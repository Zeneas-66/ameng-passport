#include "ameng_ai.h"
#include "ameng_audio.h"
#include "ameng_dialogue.h"
#include "ameng_pet.h"
#include "ameng_store.h"
#include "ameng_ui.h"

#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static const char *TAG = "ameng_v2";

#define INPUT_QUEUE_DEPTH 12
#define AI_QUEUE_DEPTH 2
#define SAVE_INTERVAL_S 300ULL
#define SCREEN_BRIGHTNESS_DEFAULT 90
#define AUDIO_VOLUME_DEFAULT 70

typedef struct {
    bsp_btn_t btn;
    bsp_btn_ev_t ev;
} input_event_t;

typedef struct {
    ameng_state_t state;
    ameng_room_t room;
    uint64_t now_s;
    uint8_t hour;
} ai_job_t;

static ameng_state_t s_pet;
static ameng_ui_t s_ui;
static QueueHandle_t s_input_queue;
static QueueHandle_t s_ai_queue;

static uint64_t s_logical_base;
static int64_t s_boot_us;
static uint64_t s_last_save_s;
static uint64_t s_last_real_epoch_s;
static int16_t s_clock_offset_minutes;
static bool s_time_reconciled;

static ameng_room_t s_player_room = AMENG_ROOM_LIVING;
static ameng_ui_action_t s_action = AMENG_UI_CALL;
static uint8_t s_home_selected;
static uint8_t s_settings_selected;
static uint8_t s_brightness = SCREEN_BRIGHTNESS_DEFAULT;
static uint8_t s_volume = AUDIO_VOLUME_DEFAULT;

static bool s_ready;
static bool s_screen_off;
static bool s_time_edit;
static bool s_ai_busy;
static bool s_dirty;
static uint64_t s_dirty_since;
static bool s_ignore_wake_events;
static bsp_btn_t s_wake_button;

static uint64_t now_s(void)
{
    int64_t elapsed = esp_timer_get_time() - s_boot_us;
    if (elapsed < 0) elapsed = 0;
    return s_logical_base + (uint64_t)(elapsed / 1000000LL);
}

static uint16_t local_minute_of_day(uint64_t t)
{
    uint64_t real = ameng_ai_real_epoch_s();
    uint64_t source = real ? real : t;
    int64_t total = (int64_t)(source / 60ULL) + s_clock_offset_minutes;
    total %= 1440;
    if (total < 0) total += 1440;
    return (uint16_t)total;
}

static uint8_t local_hour(uint64_t t)
{
    return (uint8_t)(local_minute_of_day(t) / 60U);
}

static uint8_t local_minute(uint64_t t)
{
    return (uint8_t)(local_minute_of_day(t) % 60U);
}

static ameng_behavior_t behavior_now(void)
{
    uint64_t n = now_s();
    return ameng_state_behavior(&s_pet, n, local_hour(n));
}

static void save_state(void)
{
    ameng_saved_t saved = {
        .pet = s_pet,
        .logical_now_s = now_s(),
        .last_real_epoch_s = ameng_ai_real_epoch_s() ? ameng_ai_real_epoch_s()
                                                       : s_last_real_epoch_s,
        .clock_offset_minutes = s_clock_offset_minutes,
        .brightness = s_brightness,
        .volume = s_volume,
        .player_room = s_player_room,
    };
    esp_err_t err = ameng_store_save(&saved);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "save failed: %s", esp_err_to_name(err));
    } else {
        s_last_save_s = saved.logical_now_s;
        s_dirty = false;
    }
}

static void mark_dirty(uint64_t n)
{
    s_dirty = true;
    s_dirty_since = n;
}

static void reconcile_real_time_if_ready(void)
{
    if (s_time_reconciled) return;

    uint64_t real = ameng_ai_real_epoch_s();
    if (!real) return;

    uint64_t boot_elapsed = 0;
    int64_t elapsed_us = esp_timer_get_time() - s_boot_us;
    if (elapsed_us > 0) boot_elapsed = (uint64_t)(elapsed_us / 1000000LL);

    if (s_last_real_epoch_s > 0 && real > s_last_real_epoch_s) {
        uint64_t wall_delta = real - s_last_real_epoch_s;
        uint64_t offline_delta = wall_delta > boot_elapsed
                               ? wall_delta - boot_elapsed : 0;
        s_logical_base += offline_delta;
        ESP_LOGI(TAG, "real time restored, offline elapsed=%llu s",
                 (unsigned long long)offline_delta);
    } else {
        ESP_LOGI(TAG, "real time synchronized for the first time");
    }

    s_last_real_epoch_s = real;
    s_time_reconciled = true;

    uint64_t n = now_s();
    ameng_state_advance(&s_pet, n, local_hour(n));
    mark_dirty(n);
}

static void ui_pet_refresh(void)
{
    if (s_ui.page != AMENG_PAGE_PET) return;
    uint64_t n = now_s();
    ameng_ui_render_pet(&s_ui, &s_pet, s_player_room,
                        local_hour(n), local_minute(n),
                        bsp_battery_soc(), ameng_ai_online());
}

static void show_plain_message(const char *text)
{
    if (!bsp_lvgl_lock(250)) return;
    ameng_ui_set_dialogue(&s_ui, text, true);
    bsp_lvgl_unlock();
}

static void show_cat_message(ameng_line_reason_t reason, uint8_t sound_variant)
{
    char line[160];
    uint64_t n = now_s();
    ameng_dialogue_local(&s_pet, reason, n, local_hour(n), line, sizeof(line));
    if (bsp_lvgl_lock(300)) {
        ameng_ui_set_dialogue(&s_ui, line, true);
        bsp_lvgl_unlock();
    }
    ameng_audio_meow(sound_variant);
}

static bool require_ameng_here(const char *missing_text)
{
    if (ameng_state_is_present(&s_pet, s_player_room)) return true;
    show_plain_message(missing_text);
    return false;
}

static void queue_ai_talk(uint64_t n)
{
    ai_job_t job = {
        .state = s_pet,
        .room = s_player_room,
        .now_s = n,
        .hour = local_hour(n),
    };
    if (xQueueSend(s_ai_queue, &job, 0) == pdTRUE) {
        s_ai_busy = true;
        show_plain_message("MEOW... (LET ME THINK)");
    } else {
        show_cat_message(AMENG_LINE_TALK, 2);
    }
}

static void do_pet_action(ameng_ui_action_t action)
{
    uint64_t n = now_s();
    uint8_t hour = local_hour(n);

    switch (action) {
    case AMENG_UI_CALL: {
        ameng_call_result_t result =
            ameng_state_call(&s_pet, s_player_room, n, hour);
        if (result == AMENG_CALL_SAME_ROOM) {
            show_cat_message(AMENG_LINE_CALL_SAME, (uint8_t)(s_pet.daily_calls % 6));
        } else if (result == AMENG_CALL_COMES) {
            show_cat_message(AMENG_LINE_CALL_COMES, (uint8_t)(s_pet.daily_calls % 6));
            s_pet.cat_depth = 1;
            if (bsp_lvgl_lock(250)) {
                ameng_ui_start_animation(&s_ui, AMENG_ANIM_ENTER);
                bsp_lvgl_unlock();
            }
        } else {
            show_cat_message(AMENG_LINE_CALL_FAR, (uint8_t)(s_pet.daily_calls % 6));
        }
        break;
    }

    case AMENG_UI_FEED:
        if (!require_ameng_here("AMENG ISN'T HERE. CALL HIM FIRST.")) break;
        if (ameng_state_interact(&s_pet, AMENG_ACTION_FEED, s_player_room, n, hour)) {
            show_cat_message(AMENG_LINE_FEED, 1);
        }
        break;

    case AMENG_UI_PET:
        if (!require_ameng_here("AMENG ISN'T HERE. CALL HIM FIRST.")) break;
        if (ameng_state_interact(&s_pet, AMENG_ACTION_PET, s_player_room, n, hour)) {
            show_cat_message(AMENG_LINE_PET, 4);
            bool hug = ameng_relationship_percent(s_pet.affection_x10) >= 60 &&
                       ((s_pet.daily_pets + s_pet.mood) % 3U != 0);
            if (bsp_lvgl_lock(250)) {
                ameng_ui_start_animation(&s_ui,
                    hug ? AMENG_ANIM_LEG_HUG : AMENG_ANIM_RUB);
                bsp_lvgl_unlock();
            }
        }
        break;

    case AMENG_UI_PLAY:
        if (!require_ameng_here("AMENG ISN'T HERE. CALL HIM FIRST.")) break;
        if (ameng_state_interact(&s_pet, AMENG_ACTION_PLAY, s_player_room, n, hour)) {
            s_pet.cat_depth = 1;
            show_cat_message(AMENG_LINE_PLAY, 5);
            if (bsp_lvgl_lock(250)) {
                ameng_ui_start_animation(&s_ui, AMENG_ANIM_POUNCE);
                bsp_lvgl_unlock();
            }
        } else {
            show_cat_message(AMENG_LINE_TIRED, 3);
        }
        break;

    case AMENG_UI_TALK:
        if (!require_ameng_here("GO TO AMENG FIRST.")) break;
        if (!ameng_state_interact(&s_pet, AMENG_ACTION_TALK,
                                  s_player_room, n, hour)) {
            break;
        }
        if (ameng_ai_enabled() && ameng_ai_online() && !s_ai_busy) {
            queue_ai_talk(n);
        } else {
            show_cat_message(AMENG_LINE_TALK, 0);
        }
        break;

    default:
        break;
    }

    /* Delay NVS writes until the short meow has finished. Flash/cache stalls
     * during active PCM playback are a known source of audio glitches. */
    mark_dirty(n);
    if (bsp_lvgl_lock(250)) {
        ui_pet_refresh();
        bsp_lvgl_unlock();
    }
}

static void go_home(void)
{
    s_time_edit = false;
    if (!bsp_lvgl_lock(250)) return;
    ameng_ui_set_status_visible(&s_ui, false);
    ameng_ui_show_page(&s_ui, AMENG_PAGE_HOME);
    ameng_ui_set_home_selected(&s_ui, s_home_selected);
    bsp_lvgl_unlock();
}

static void enter_selected_home(void)
{
    ameng_page_t page = s_home_selected == 0 ? AMENG_PAGE_PET : AMENG_PAGE_SETTINGS;

    if (!bsp_lvgl_lock(300)) return;
    ameng_ui_show_page(&s_ui, page);
    if (page == AMENG_PAGE_PET) {
        ui_pet_refresh();
    } else if (page == AMENG_PAGE_SETTINGS) {
        ameng_ui_render_settings(&s_ui, s_brightness, s_volume,
                                 local_hour(now_s()));
    }
    bsp_lvgl_unlock();
}

static void screen_off(void)
{
    if (s_screen_off) return;
    s_screen_off = true;
    if (bsp_lvgl_lock(150)) {
        ameng_ui_set_status_visible(&s_ui, false);
        bsp_lvgl_unlock();
    }
    bsp_display_backlight(0);
}

static void screen_wake(bsp_btn_t btn)
{
    s_screen_off = false;
    s_ignore_wake_events = true;
    s_wake_button = btn;
    bsp_display_backlight(s_brightness);
}

static void settings_apply_click(void)
{
    if (s_settings_selected == 0) {
        static const uint8_t levels[] = {35, 60, 90, 100};
        int index = 0;
        for (int i = 0; i < 4; ++i) if (levels[i] == s_brightness) index = i;
        s_brightness = levels[(index + 1) % 4];
        bsp_display_backlight(s_brightness);
    } else if (s_settings_selected == 1) {
        static const uint8_t levels[] = {0, 35, 70, 90};
        int index = 0;
        for (int i = 0; i < 4; ++i) if (levels[i] == s_volume) index = i;
        s_volume = levels[(index + 1) % 4];
        ameng_audio_set_volume(s_volume);
    } else if (s_settings_selected == 2) {
        s_time_edit = !s_time_edit;
        if (bsp_lvgl_lock(200)) {
            lv_label_set_text(s_ui.settings_hint,
                s_time_edit ? "CLOCK: UP/DOWN HOUR  OK DONE"
                            : "UP/DOWN SELECT  OK CHANGE  DOUBLE OK BACK");
            bsp_lvgl_unlock();
        }
    }

    if (bsp_lvgl_lock(200)) {
        ameng_ui_render_settings(&s_ui, s_brightness, s_volume,
                                 local_hour(now_s()));
        bsp_lvgl_unlock();
    }
    save_state();
}

static void adjust_clock(int delta_hours)
{
    s_clock_offset_minutes += (int16_t)(delta_hours * 60);
    while (s_clock_offset_minutes > 1439) s_clock_offset_minutes -= 1440;
    while (s_clock_offset_minutes < -1439) s_clock_offset_minutes += 1440;
    if (bsp_lvgl_lock(200)) {
        ameng_ui_render_settings(&s_ui, s_brightness, s_volume,
                                 local_hour(now_s()));
        bsp_lvgl_unlock();
    }
}

static void handle_pet_key(const input_event_t *e)
{
    if (e->ev == BSP_BTN_LONG && e->btn == BSP_BTN_UP) {
        if (bsp_lvgl_lock(200)) {
            bool next = !s_ui.status_visible;
            ameng_ui_render_status(&s_ui, &s_pet);
            ameng_ui_set_status_visible(&s_ui, next);
            bsp_lvgl_unlock();
        }
        return;
    }

    if (e->ev == BSP_BTN_LONG && e->btn == BSP_BTN_DOWN) {
        s_player_room = (ameng_room_t)((s_player_room + 1) % AMENG_ROOM_COUNT);
        if (bsp_lvgl_lock(250)) {
            ameng_ui_set_status_visible(&s_ui, false);
            ui_pet_refresh();
            bsp_lvgl_unlock();
        }
        save_state();
        return;
    }

    if (e->ev != BSP_BTN_CLICK) return;

    if (e->btn == BSP_BTN_UP) {
        s_action = (ameng_ui_action_t)(
            (s_action + AMENG_UI_ACTION_COUNT - 1) % AMENG_UI_ACTION_COUNT);
    } else if (e->btn == BSP_BTN_DOWN) {
        s_action = (ameng_ui_action_t)((s_action + 1) % AMENG_UI_ACTION_COUNT);
    } else if (e->btn == BSP_BTN_OK) {
        do_pet_action(s_action);
    }

    if (bsp_lvgl_lock(200)) {
        ameng_ui_set_action(&s_ui, s_action);
        bsp_lvgl_unlock();
    }
}

static void handle_settings_key(const input_event_t *e)
{
    if (e->ev != BSP_BTN_CLICK) return;

    if (s_time_edit) {
        if (e->btn == BSP_BTN_UP) adjust_clock(1);
        else if (e->btn == BSP_BTN_DOWN) adjust_clock(-1);
        else if (e->btn == BSP_BTN_OK) settings_apply_click();
        return;
    }

    if (e->btn == BSP_BTN_UP) {
        s_settings_selected = (uint8_t)((s_settings_selected + 3) % 4);
    } else if (e->btn == BSP_BTN_DOWN) {
        s_settings_selected = (uint8_t)((s_settings_selected + 1) % 4);
    } else if (e->btn == BSP_BTN_OK) {
        settings_apply_click();
    }

    if (bsp_lvgl_lock(200)) {
        ameng_ui_set_settings_selected(&s_ui, s_settings_selected);
        bsp_lvgl_unlock();
    }
}

static void handle_home_key(const input_event_t *e)
{
    if (e->ev != BSP_BTN_CLICK) return;
    if (e->btn == BSP_BTN_UP) {
        s_home_selected = (uint8_t)((s_home_selected + 1) % 2);
    } else if (e->btn == BSP_BTN_DOWN) {
        s_home_selected = (uint8_t)((s_home_selected + 1) % 2);
    } else if (e->btn == BSP_BTN_OK) {
        enter_selected_home();
        return;
    }

    if (bsp_lvgl_lock(200)) {
        ameng_ui_set_home_selected(&s_ui, s_home_selected);
        bsp_lvgl_unlock();
    }
}

static void input_task(void *arg)
{
    (void)arg;
    input_event_t e;
    uint64_t last_second = now_s();
    uint64_t last_auto_anim = 0;

    for (;;) {
        bool got = xQueueReceive(s_input_queue, &e, pdMS_TO_TICKS(250)) == pdTRUE;

        if (got) {
            if (s_screen_off) {
                if (e.ev == BSP_BTN_PRESS) screen_wake(e.btn);
                continue;
            }

            if (s_ignore_wake_events && e.btn == s_wake_button) {
                if (e.ev == BSP_BTN_CLICK || e.ev == BSP_BTN_LONG) {
                    s_ignore_wake_events = false;
                }
                continue;
            }

            if (e.ev == BSP_BTN_LONG && e.btn == BSP_BTN_OK) {
                screen_off();
                continue;
            }

            if (e.ev == BSP_BTN_DOUBLE && e.btn == BSP_BTN_OK &&
                s_ui.page != AMENG_PAGE_HOME) {
                go_home();
                continue;
            }

            if (s_ui.page == AMENG_PAGE_HOME) handle_home_key(&e);
            else if (s_ui.page == AMENG_PAGE_PET) handle_pet_key(&e);
            else if (s_ui.page == AMENG_PAGE_SETTINGS) handle_settings_key(&e);
        }

        reconcile_real_time_if_ready();
        uint64_t n = now_s();
        if (n != last_second) {
            last_second = n;
            ameng_state_advance(&s_pet, n, local_hour(n));

            if (s_ui.page == AMENG_PAGE_PET && !s_screen_off) {
                ameng_behavior_t b = behavior_now();

                if (n - last_auto_anim >= 45 &&
                    ameng_state_is_present(&s_pet, s_player_room) &&
                    s_ui.anim == AMENG_ANIM_NONE) {
                    if (b == AMENG_BEHAVIOR_LEG_HUG) {
                        if (bsp_lvgl_lock(150)) {
                            ameng_ui_start_animation(&s_ui, AMENG_ANIM_LEG_HUG);
                            bsp_lvgl_unlock();
                        }
                        last_auto_anim = n;
                    } else if (b == AMENG_BEHAVIOR_POUNCE) {
                        if (bsp_lvgl_lock(150)) {
                            ameng_ui_start_animation(&s_ui, AMENG_ANIM_POUNCE);
                            bsp_lvgl_unlock();
                        }
                        last_auto_anim = n;
                    }
                }

                if (bsp_lvgl_lock(250)) {
                    ui_pet_refresh();
                    bsp_lvgl_unlock();
                }
            }

            if (s_dirty && n >= s_dirty_since + 2) {
                save_state();
            } else if (n - s_last_save_s >= SAVE_INTERVAL_S) {
                save_state();
            }
        }

        if (s_ui.page == AMENG_PAGE_PET && !s_screen_off) {
            if (bsp_lvgl_lock(120)) {
                ameng_ui_tick(&s_ui, &s_pet, s_player_room, behavior_now());
                bsp_lvgl_unlock();
            }
        }
    }
}

static void ai_task(void *arg)
{
    (void)arg;
    ai_job_t job;
    char meaning[96];
    char line[160];

    for (;;) {
        if (xQueueReceive(s_ai_queue, &job, portMAX_DELAY) != pdTRUE) continue;

        esp_err_t err = ameng_ai_generate(&job.state, job.room, job.now_s,
                                          job.hour, meaning, sizeof(meaning));
        if (err != ESP_OK) {
            ameng_dialogue_local_meaning(&job.state, AMENG_LINE_TALK,
                                         job.now_s, job.hour,
                                         meaning, sizeof(meaning));
        }
        ameng_dialogue_wrap(&job.state, AMENG_LINE_TALK, job.now_s,
                            meaning, line, sizeof(line));

        if (bsp_lvgl_lock(500)) {
            if (s_ui.page == AMENG_PAGE_PET) {
                ameng_ui_set_dialogue(&s_ui, line, true);
            }
            bsp_lvgl_unlock();
        }
        ameng_audio_meow((uint8_t)(job.state.daily_talks % 6));
        s_ai_busy = false;
    }
}

static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    if (!s_ready || !s_input_queue) return;
    input_event_t e = {.btn = btn, .ev = ev};
    (void)xQueueSend(s_input_queue, &e, 0);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Ameng V2 starting");

    s_boot_us = esp_timer_get_time();
    esp_err_t nvs = ameng_store_init();
    if (nvs != ESP_OK) ESP_LOGW(TAG, "NVS unavailable: %s", esp_err_to_name(nvs));

    ameng_saved_t saved = {0};
    bool found = false;
    esp_err_t load = ameng_store_load(&saved, &found);
    if (load == ESP_OK && found) {
        s_pet = saved.pet;
        s_logical_base = saved.logical_now_s;
        s_last_real_epoch_s = saved.last_real_epoch_s;
        s_clock_offset_minutes = saved.clock_offset_minutes;
        s_brightness = saved.brightness ? saved.brightness : SCREEN_BRIGHTNESS_DEFAULT;
        s_volume = saved.volume;
        s_player_room = saved.player_room < AMENG_ROOM_COUNT
                      ? saved.player_room : AMENG_ROOM_LIVING;
    } else {
        s_logical_base = 12ULL * 3600ULL;
        s_last_real_epoch_s = 0;
        s_clock_offset_minutes = 8 * 60;
        s_brightness = SCREEN_BRIGHTNESS_DEFAULT;
        s_volume = AUDIO_VOLUME_DEFAULT;
        s_player_room = AMENG_ROOM_LIVING;
        ameng_state_init(&s_pet, s_logical_base);
    }
    s_last_save_s = s_logical_base;

    (void)bsp_i2c_init();
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "display/LVGL init failed");
        return;
    }
    bsp_display_backlight(s_brightness);
    (void)bsp_battery_init();

    esp_err_t audio = ameng_audio_init();
    if (audio != ESP_OK) ESP_LOGW(TAG, "audio unavailable: %s", esp_err_to_name(audio));
    ameng_audio_set_volume(s_volume);

    if (!bsp_lvgl_lock(1000)) return;
    ameng_ui_create(&s_ui);
    ameng_ui_set_room(&s_ui, s_player_room);
    ameng_ui_render_settings(&s_ui, s_brightness, s_volume, local_hour(now_s()));
    bsp_lvgl_unlock();

    s_input_queue = xQueueCreate(INPUT_QUEUE_DEPTH, sizeof(input_event_t));
    s_ai_queue = xQueueCreate(AI_QUEUE_DEPTH, sizeof(ai_job_t));
    if (!s_input_queue || !s_ai_queue) {
        ESP_LOGE(TAG, "queue allocation failed");
        return;
    }

    if (xTaskCreate(input_task, "ameng_input", 4608, NULL, 5, NULL) != pdPASS ||
        xTaskCreate(ai_task, "ameng_ai", 6144, NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "task creation failed");
        return;
    }

    esp_err_t btn = bsp_button_init(on_key, NULL);
    if (btn != ESP_OK) ESP_LOGE(TAG, "button init failed: %s", esp_err_to_name(btn));

    esp_err_t ai = ameng_ai_init();
    if (ai != ESP_OK) ESP_LOGW(TAG, "AI init failed: %s", esp_err_to_name(ai));

    s_ready = (btn == ESP_OK);
    ESP_LOGI(TAG, "Ameng V2 ready, AI=%d, flash baseline=8MB", ameng_ai_enabled());
}

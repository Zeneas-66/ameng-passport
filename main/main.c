#include "ameng_ai.h"
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

static const char *TAG = "ameng";
#define INPUT_QUEUE_DEPTH 8
#define AI_QUEUE_DEPTH 2
#define SAVE_INTERVAL_S 300ULL

typedef struct { bsp_btn_t btn; bsp_btn_ev_t ev; } input_event_t;
typedef struct { ameng_state_t state; uint64_t now_s; uint8_t hour; } ai_job_t;

static ameng_state_t s_pet;
static ameng_ui_t s_ui;
static ameng_ui_action_t s_selected = AMENG_UI_CALL;
static QueueHandle_t s_input_queue;
static QueueHandle_t s_ai_queue;
static uint64_t s_logical_base;
static int64_t s_boot_us;
static uint64_t s_last_save_s;
static bool s_ready;
static volatile bool s_ai_busy;

static uint64_t now_s(void)
{
    int64_t elapsed_us = esp_timer_get_time() - s_boot_us;
    if (elapsed_us < 0) elapsed_us = 0;
    return s_logical_base + (uint64_t)(elapsed_us / 1000000LL);
}

static uint8_t local_hour(uint64_t t)
{
    return (uint8_t)((t / 3600ULL) % 24ULL);
}

static ameng_behavior_t current_behavior(void)
{
    uint64_t n = now_s();
    return ameng_state_behavior(&s_pet, n, local_hour(n));
}

static void save_state(void)
{
    ameng_saved_t saved = {.pet = s_pet, .logical_now_s = now_s()};
    esp_err_t err = ameng_store_save(&saved);
    if (err != ESP_OK) ESP_LOGW(TAG, "save failed: %s", esp_err_to_name(err));
    else s_last_save_s = saved.logical_now_s;
}

static void show_local(ameng_line_reason_t reason)
{
    char line[96];
    uint64_t n = now_s();
    ameng_dialogue_local(&s_pet, reason, n, local_hour(n), line, sizeof(line));
    if (bsp_lvgl_lock(300)) {
        ameng_ui_set_dialogue(&s_ui, line);
        ameng_ui_render(&s_ui, &s_pet, current_behavior(), bsp_battery_soc(), ameng_ai_online());
        bsp_lvgl_unlock();
    }
}

static void do_action(ameng_ui_action_t action)
{
    uint64_t n = now_s();
    switch (action) {
    case AMENG_UI_CALL:
        ameng_state_interact(&s_pet, AMENG_ACTION_CALL, n);
        show_local(AMENG_LINE_CALL);
        break;
    case AMENG_UI_FEED:
        ameng_state_interact(&s_pet, AMENG_ACTION_FEED, n);
        show_local(AMENG_LINE_FEED);
        break;
    case AMENG_UI_CHIN:
        ameng_state_interact(&s_pet, AMENG_ACTION_CHIN_SCRATCH, n);
        show_local(AMENG_LINE_CHIN);
        break;
    case AMENG_UI_PLAY:
        ameng_state_interact(&s_pet, AMENG_ACTION_PLAY_BIRD, n);
        show_local(AMENG_LINE_PLAY);
        break;
    case AMENG_UI_TALK:
        ameng_state_interact(&s_pet, AMENG_ACTION_PET, n);
        if (ameng_ai_enabled() && ameng_ai_online() && !s_ai_busy) {
            ai_job_t job = {.state = s_pet, .now_s = n, .hour = local_hour(n)};
            if (xQueueSend(s_ai_queue, &job, 0) == pdTRUE) {
                s_ai_busy = true;
                if (bsp_lvgl_lock(200)) {
                    ameng_ui_set_dialogue(&s_ui, "Thinking...");
                    bsp_lvgl_unlock();
                }
            } else {
                show_local(AMENG_LINE_TALK);
            }
        } else {
            show_local(AMENG_LINE_TALK);
        }
        break;
    default:
        break;
    }
    save_state();
}

static void input_task(void *arg)
{
    (void)arg;
    input_event_t e;
    while (xQueueReceive(s_input_queue, &e, portMAX_DELAY) == pdTRUE) {
        if (e.ev != BSP_BTN_CLICK) continue;
        if (e.btn == BSP_BTN_UP) {
            s_selected = (ameng_ui_action_t)((s_selected + AMENG_UI_ACTION_COUNT - 1) % AMENG_UI_ACTION_COUNT);
        } else if (e.btn == BSP_BTN_DOWN) {
            s_selected = (ameng_ui_action_t)((s_selected + 1) % AMENG_UI_ACTION_COUNT);
        } else if (e.btn == BSP_BTN_OK) {
            do_action(s_selected);
        }
        if (bsp_lvgl_lock(200)) {
            ameng_ui_set_selected(&s_ui, s_selected);
            bsp_lvgl_unlock();
        }
    }
}

static void ai_task(void *arg)
{
    (void)arg;
    ai_job_t job;
    char line[96];
    while (xQueueReceive(s_ai_queue, &job, portMAX_DELAY) == pdTRUE) {
        esp_err_t err = ameng_ai_generate(&job.state, job.now_s, job.hour, line, sizeof(line));
        if (err != ESP_OK) {
            ameng_dialogue_local(&job.state, AMENG_LINE_TALK, job.now_s, job.hour,
                                 line, sizeof(line));
        }
        if (bsp_lvgl_lock(500)) {
            ameng_ui_set_dialogue(&s_ui, line);
            ameng_ui_render(&s_ui, &s_pet, current_behavior(), bsp_battery_soc(), ameng_ai_online());
            bsp_lvgl_unlock();
        }
        s_ai_busy = false;
    }
}

static void pet_task(void *arg)
{
    (void)arg;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        uint64_t n = now_s();
        ameng_state_advance(&s_pet, n, local_hour(n));
        if (bsp_lvgl_lock(250)) {
            ameng_ui_tick(&s_ui, current_behavior());
            ameng_ui_render(&s_ui, &s_pet, current_behavior(), bsp_battery_soc(), ameng_ai_online());
            bsp_lvgl_unlock();
        }
        if (n - s_last_save_s >= SAVE_INTERVAL_S) save_state();
    }
}

static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    if (!s_ready || !s_input_queue) return;
    input_event_t e = {.btn = btn, .ev = ev};
    xQueueSend(s_input_queue, &e, 0);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Ameng companion starting");

    s_boot_us = esp_timer_get_time();
    if (ameng_store_init() != ESP_OK) ESP_LOGW(TAG, "NVS unavailable");

    ameng_saved_t saved = {0};
    bool found = false;
    if (ameng_store_load(&saved, &found) == ESP_OK && found) {
        s_pet = saved.pet;
        s_logical_base = saved.logical_now_s;
    } else {
        s_logical_base = 0;
        ameng_state_init(&s_pet, 0);
    }
    s_last_save_s = s_logical_base;

    bsp_i2c_init();
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "display/LVGL init failed");
        return;
    }
    bsp_display_backlight(90);
    (void)bsp_battery_init();

    if (!bsp_lvgl_lock(1000)) return;
    ameng_ui_create(&s_ui);
    ameng_ui_render(&s_ui, &s_pet, current_behavior(), bsp_battery_soc(), false);
    bsp_lvgl_unlock();

    s_input_queue = xQueueCreate(INPUT_QUEUE_DEPTH, sizeof(input_event_t));
    s_ai_queue = xQueueCreate(AI_QUEUE_DEPTH, sizeof(ai_job_t));
    if (!s_input_queue || !s_ai_queue) {
        ESP_LOGE(TAG, "queue allocation failed");
        return;
    }
    if (xTaskCreate(input_task, "ameng_input", 4096, NULL, 5, NULL) != pdPASS ||
        xTaskCreate(pet_task, "ameng_pet", 4096, NULL, 4, NULL) != pdPASS ||
        xTaskCreate(ai_task, "ameng_ai", 6144, NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "task creation failed");
        return;
    }

    esp_err_t btn = bsp_button_init(on_key, NULL);
    if (btn != ESP_OK) ESP_LOGE(TAG, "button init failed: %s", esp_err_to_name(btn));

    esp_err_t ai = ameng_ai_init();
    if (ai != ESP_OK) ESP_LOGW(TAG, "AI init failed: %s", esp_err_to_name(ai));

    s_ready = (btn == ESP_OK);
    show_local(AMENG_LINE_IDLE);
    ESP_LOGI(TAG, "Ameng ready, AI=%d", ameng_ai_enabled());
}

#include "ameng_ui.h"
#include <stdio.h>
#include <string.h>

#define CREAM 0xF2E6C9
#define PAPER 0xFFF7E8
#define INK 0x2B2A28
#define GRASS 0x718C56
#define GRASS_DARK 0x49613B
#define YELLOW 0xC99743
#define EYE 0x90AE89
#define SHADOW 0xD1C2A6
#define ACCENT 0xA25D42

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h,
                     uint32_t color, int radius)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_radius(o, radius, 0);
    return o;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                       uint32_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

void ameng_ui_create(ameng_ui_t *ui)
{
    memset(ui, 0, sizeof(*ui));
    ui->screen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui->screen, lv_color_hex(CREAM), 0);
    lv_obj_set_style_border_width(ui->screen, 0, 0);
    lv_obj_set_style_pad_all(ui->screen, 0, 0);

    box(ui->screen, 0, 0, 240, 42, INK, 0);
    lv_obj_t *title = label(ui->screen, "AMENG", &lv_font_montserrat_20, PAPER);
    lv_obj_set_pos(title, 14, 10);
    ui->day_label = label(ui->screen, "DAY 0", &lv_font_montserrat_14, SHADOW);
    lv_obj_set_pos(ui->day_label, 91, 13);
    ui->battery_label = label(ui->screen, "--%", &lv_font_montserrat_14, PAPER);
    lv_obj_align(ui->battery_label, LV_ALIGN_TOP_RIGHT, -12, 13);

    box(ui->screen, 10, 51, 220, 61, PAPER, 8);
    ui->speech = label(ui->screen, "You're back.", &lv_font_montserrat_14, INK);
    lv_obj_set_width(ui->speech, 196);
    lv_obj_set_pos(ui->speech, 22, 65);
    lv_label_set_long_mode(ui->speech, LV_LABEL_LONG_WRAP);

    box(ui->screen, 0, 118, 240, 137, GRASS, 0);
    for (int x = 0; x < 240; x += 28) {
        box(ui->screen, x, 120 + ((x / 28) % 2) * 5, 18, 10, GRASS_DARK, 0);
    }

    ui->cat = lv_obj_create(ui->screen);
    lv_obj_remove_flag(ui->cat, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(ui->cat, 54, 138);
    lv_obj_set_size(ui->cat, 134, 106);
    lv_obj_set_style_bg_opa(ui->cat, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui->cat, 0, 0);
    lv_obj_set_style_pad_all(ui->cat, 0, 0);

    ui->cat_tail = box(ui->cat, 91, 52, 34, 18, YELLOW, 9);
    ui->cat_body = box(ui->cat, 30, 42, 78, 54, 0xF6F1E8, 20);
    ui->cat_head = box(ui->cat, 25, 7, 68, 57, 0xF6F1E8, 18);
    box(ui->cat, 27, 2, 18, 20, 0xF6F1E8, 3);
    box(ui->cat, 72, 2, 18, 20, 0xF6F1E8, 3);
    ui->cat_patch = box(ui->cat, 54, 8, 29, 16, YELLOW, 5);
    ui->cat_eye_l = box(ui->cat, 39, 30, 8, 5, EYE, 2);
    ui->cat_eye_r = box(ui->cat, 69, 30, 8, 5, EYE, 2);
    box(ui->cat, 56, 39, 6, 4, ACCENT, 2);
    ui->cat_lip_patch = box(ui->cat, 48, 42, 10, 6, YELLOW, 2);
    ui->cat_paw_l = box(ui->cat, 38, 82, 21, 18, 0xF6F1E8, 8);
    ui->cat_paw_r = box(ui->cat, 67, 82, 21, 18, 0xF6F1E8, 8);

    ui->bird = box(ui->screen, 198, 162, 16, 11, 0x5C493B, 3);
    box(ui->bird, -4, 4, 6, 3, 0xD8A440, 0);

    box(ui->screen, 0, 255, 240, 65, INK, 0);
    static const char *names[AMENG_UI_ACTION_COUNT] = {"CALL", "FEED", "CHIN", "BIRD", "TALK"};
    for (int i = 0; i < AMENG_UI_ACTION_COUNT; ++i) {
        int x = 4 + i * 47;
        ui->action_labels[i] = label(ui->screen, names[i], &lv_font_montserrat_14, SHADOW);
        lv_obj_set_width(ui->action_labels[i], 44);
        lv_obj_set_style_text_align(ui->action_labels[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(ui->action_labels[i], x, 268);
    }
    ui->status = label(ui->screen, "UP/DOWN choose  OK do", &lv_font_montserrat_14, SHADOW);
    lv_obj_set_width(ui->status, 230);
    lv_obj_set_style_text_align(ui->status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(ui->status, 5, 298);

    ameng_ui_set_selected(ui, AMENG_UI_CALL);
    lv_screen_load(ui->screen);
}

void ameng_ui_set_selected(ameng_ui_t *ui, ameng_ui_action_t action)
{
    for (int i = 0; i < AMENG_UI_ACTION_COUNT; ++i) {
        lv_obj_set_style_text_color(ui->action_labels[i],
            lv_color_hex(i == action ? 0xFFFFFF : SHADOW), 0);
        lv_obj_set_style_text_decor(ui->action_labels[i],
            i == action ? LV_TEXT_DECOR_UNDERLINE : LV_TEXT_DECOR_NONE, 0);
    }
}

void ameng_ui_set_dialogue(ameng_ui_t *ui, const char *text)
{
    if (ui && ui->speech) lv_label_set_text(ui->speech, text ? text : "...");
}

void ameng_ui_render(ameng_ui_t *ui, const ameng_state_t *state,
                     ameng_behavior_t behavior, int battery, bool ai_online)
{
    if (!ui || !state) return;
    lv_label_set_text_fmt(ui->day_label, "DAY %lu", (unsigned long)state->days_together);
    if (battery >= 0) lv_label_set_text_fmt(ui->battery_label, "%d%%", battery);
    else lv_label_set_text(ui->battery_label, "--%");

    lv_label_set_text_fmt(ui->status, "H%u E%u B%u  AI:%s",
                          state->hunger, state->energy, state->bond,
                          ai_online ? "ON" : "OFF");

    lv_obj_set_pos(ui->cat, 54, 138);
    lv_obj_clear_flag(ui->bird, LV_OBJ_FLAG_HIDDEN);
    switch (behavior) {
    case AMENG_BEHAVIOR_NAP:
        lv_obj_set_y(ui->cat, 160);
        lv_obj_set_size(ui->cat_eye_l, 8, 2);
        lv_obj_set_size(ui->cat_eye_r, 8, 2);
        lv_obj_add_flag(ui->bird, LV_OBJ_FLAG_HIDDEN);
        break;
    case AMENG_BEHAVIOR_WAIT:
        lv_obj_set_x(ui->cat, 14);
        lv_obj_set_size(ui->cat_eye_l, 8, 5);
        lv_obj_set_size(ui->cat_eye_r, 8, 5);
        break;
    case AMENG_BEHAVIOR_LEG_HUG:
        lv_obj_set_y(ui->cat, 130);
        lv_obj_set_pos(ui->cat_paw_l, 31, 65);
        lv_obj_set_pos(ui->cat_paw_r, 75, 65);
        break;
    case AMENG_BEHAVIOR_POUNCE:
        lv_obj_set_x(ui->cat, 86);
        break;
    default:
        lv_obj_set_size(ui->cat_eye_l, 8, 5);
        lv_obj_set_size(ui->cat_eye_r, 8, 5);
        lv_obj_set_pos(ui->cat_paw_l, 38, 82);
        lv_obj_set_pos(ui->cat_paw_r, 67, 82);
        break;
    }
}

void ameng_ui_tick(ameng_ui_t *ui, ameng_behavior_t behavior)
{
    if (!ui) return;
    ui->frame = !ui->frame;
    int dy = ui->frame ? -2 : 0;
    if (behavior == AMENG_BEHAVIOR_NAP) dy = ui->frame ? 1 : 0;
    lv_obj_set_style_translate_y(ui->cat_body, dy, 0);
    lv_obj_set_style_translate_y(ui->cat_head, dy, 0);
    if (behavior == AMENG_BEHAVIOR_POUNCE) {
        lv_obj_set_x(ui->bird, ui->frame ? 194 : 202);
    }
}

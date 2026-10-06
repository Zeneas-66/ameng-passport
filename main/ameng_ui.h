#pragma once
#include "ameng_pet.h"
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    AMENG_UI_CALL = 0,
    AMENG_UI_FEED,
    AMENG_UI_CHIN,
    AMENG_UI_PLAY,
    AMENG_UI_TALK,
    AMENG_UI_ACTION_COUNT
} ameng_ui_action_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *day_label;
    lv_obj_t *battery_label;
    lv_obj_t *speech;
    lv_obj_t *status;
    lv_obj_t *action_labels[AMENG_UI_ACTION_COUNT];
    lv_obj_t *cat;
    lv_obj_t *cat_body;
    lv_obj_t *cat_head;
    lv_obj_t *cat_eye_l;
    lv_obj_t *cat_eye_r;
    lv_obj_t *cat_patch;
    lv_obj_t *cat_lip_patch;
    lv_obj_t *cat_tail;
    lv_obj_t *cat_paw_l;
    lv_obj_t *cat_paw_r;
    lv_obj_t *bird;
    bool frame;
} ameng_ui_t;

void ameng_ui_create(ameng_ui_t *ui);
void ameng_ui_set_selected(ameng_ui_t *ui, ameng_ui_action_t action);
void ameng_ui_set_dialogue(ameng_ui_t *ui, const char *text);
void ameng_ui_render(ameng_ui_t *ui, const ameng_state_t *state,
                     ameng_behavior_t behavior, int battery, bool ai_online);
void ameng_ui_tick(ameng_ui_t *ui, ameng_behavior_t behavior);

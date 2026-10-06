#pragma once
#include "ameng_pet.h"
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    AMENG_PAGE_HOME = 0,
    AMENG_PAGE_CARD,
    AMENG_PAGE_PET,
    AMENG_PAGE_SETTINGS,
} ameng_page_t;

typedef enum {
    AMENG_UI_CALL = 0,
    AMENG_UI_FEED,
    AMENG_UI_PET,
    AMENG_UI_PLAY,
    AMENG_UI_TALK,
    AMENG_UI_ACTION_COUNT,
} ameng_ui_action_t;

typedef enum {
    AMENG_ANIM_NONE = 0,
    AMENG_ANIM_ENTER,
    AMENG_ANIM_LEG_HUG,
    AMENG_ANIM_POUNCE,
    AMENG_ANIM_RUB,
} ameng_anim_t;

typedef struct {
    lv_obj_t *group;
    lv_obj_t *body;
    lv_obj_t *head;
    lv_obj_t *eye_l;
    lv_obj_t *eye_r;
    lv_obj_t *paw_l;
    lv_obj_t *paw_r;
    lv_obj_t *tail;
    lv_obj_t *patch_l;
    lv_obj_t *patch_r;
    lv_obj_t *lip_patch;
    int w;
    int h;
} ameng_cat_ui_t;

typedef struct {
    lv_obj_t *screen;
    lv_obj_t *home_panel;
    lv_obj_t *card_panel;
    lv_obj_t *pet_panel;
    lv_obj_t *settings_panel;

    lv_obj_t *home_items[3];
    lv_obj_t *home_hint;

    lv_obj_t *card_name;
    lv_obj_t *card_bio;
    lv_obj_t *card_footer;

    lv_obj_t *scene_panels[AMENG_ROOM_COUNT];
    lv_obj_t *room_label;
    lv_obj_t *time_label;
    lv_obj_t *battery_label;
    lv_obj_t *speech_panel;
    lv_obj_t *speech;
    lv_obj_t *action_labels[AMENG_UI_ACTION_COUNT];

    ameng_cat_ui_t cats[3];
    lv_obj_t *bird;
    lv_obj_t *leg;

    lv_obj_t *status_panel;
    lv_obj_t *status_title;
    lv_obj_t *status_lines[8];

    lv_obj_t *settings_items[4];
    lv_obj_t *settings_values[4];
    lv_obj_t *settings_hint;

    ameng_page_t page;
    ameng_ui_action_t selected_action;
    uint8_t home_selected;
    uint8_t settings_selected;
    ameng_room_t room;
    bool status_visible;

    ameng_anim_t anim;
    uint8_t anim_frame;
} ameng_ui_t;

void ameng_ui_create(ameng_ui_t *ui, const char *card_name, const char *card_bio);
void ameng_ui_show_page(ameng_ui_t *ui, ameng_page_t page);

void ameng_ui_set_home_selected(ameng_ui_t *ui, uint8_t selected);
void ameng_ui_set_action(ameng_ui_t *ui, ameng_ui_action_t action);
void ameng_ui_set_settings_selected(ameng_ui_t *ui, uint8_t selected);

void ameng_ui_set_room(ameng_ui_t *ui, ameng_room_t room);
void ameng_ui_set_dialogue(ameng_ui_t *ui, const char *text, bool visible);
void ameng_ui_set_status_visible(ameng_ui_t *ui, bool visible);

void ameng_ui_render_pet(ameng_ui_t *ui, const ameng_state_t *state,
                         ameng_room_t player_room, uint8_t hour, uint8_t minute,
                         int battery, bool ai_online);
void ameng_ui_render_status(ameng_ui_t *ui, const ameng_state_t *state);
void ameng_ui_render_settings(ameng_ui_t *ui, uint8_t brightness,
                              uint8_t volume, uint8_t hour);

void ameng_ui_start_animation(ameng_ui_t *ui, ameng_anim_t anim);
void ameng_ui_tick(ameng_ui_t *ui, const ameng_state_t *state,
                   ameng_room_t player_room, ameng_behavior_t behavior);

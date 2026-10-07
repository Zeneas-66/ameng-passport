#pragma once
#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#define AMENG_PHOTO_COUNT 9

void ameng_photo_create(lv_obj_t *screen);
void ameng_photo_show(uint8_t index);
void ameng_photo_hide(void);
bool ameng_photo_visible(void);
uint8_t ameng_photo_index(void);
uint8_t ameng_photo_move(int delta);

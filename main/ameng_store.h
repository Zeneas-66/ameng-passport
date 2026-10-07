#pragma once
#include "ameng_pet.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    ameng_state_t pet;
    uint64_t logical_now_s;
    uint64_t last_real_epoch_s;
    int16_t clock_offset_minutes;
    uint8_t brightness;
    uint8_t volume;
    ameng_room_t player_room;
} ameng_saved_t;

esp_err_t ameng_store_init(void);
esp_err_t ameng_store_load(ameng_saved_t *out, bool *found);
esp_err_t ameng_store_save(const ameng_saved_t *saved);

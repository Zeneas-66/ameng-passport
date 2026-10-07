#pragma once
#include "esp_err.h"
#include <stdint.h>

typedef enum {
    AMENG_MEOW_SOFT = 0,
    AMENG_MEOW_SOCIAL,
    AMENG_MEOW_LOW,
    AMENG_MEOW_QUICK,
    AMENG_MEOW_PLEADING,
    AMENG_MEOW_COUNT,
} ameng_meow_t;

esp_err_t ameng_audio_init(void);
void ameng_audio_set_volume(uint8_t volume);
void ameng_audio_meow(ameng_meow_t voice);

#pragma once
#include "esp_err.h"
#include <stdint.h>

esp_err_t ameng_audio_init(void);
void ameng_audio_set_volume(uint8_t volume);
void ameng_audio_meow(uint8_t variant);

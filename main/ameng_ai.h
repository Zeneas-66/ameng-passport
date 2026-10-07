#pragma once
#include "ameng_pet.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

esp_err_t ameng_ai_init(void);
bool ameng_ai_enabled(void);
bool ameng_ai_online(void);
bool ameng_ai_time_synced(void);
uint64_t ameng_ai_real_epoch_s(void);

esp_err_t ameng_ai_generate(const ameng_state_t *state,
                            ameng_room_t player_room,
                            uint64_t now_s,
                            uint8_t local_hour,
                            char *out,
                            size_t out_size);

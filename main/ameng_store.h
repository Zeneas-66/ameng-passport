#pragma once
#include "ameng_pet.h"
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    ameng_state_t pet;
    uint64_t logical_now_s;
} ameng_saved_t;

esp_err_t ameng_store_init(void);
esp_err_t ameng_store_load(ameng_saved_t *out, bool *found);
esp_err_t ameng_store_save(const ameng_saved_t *saved);

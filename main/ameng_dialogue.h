#pragma once
#include "ameng_pet.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    AMENG_LINE_IDLE = 0,
    AMENG_LINE_CALL,
    AMENG_LINE_FEED,
    AMENG_LINE_PET,
    AMENG_LINE_CHIN,
    AMENG_LINE_PLAY,
    AMENG_LINE_TALK,
} ameng_line_reason_t;

void ameng_dialogue_local(const ameng_state_t *state, ameng_line_reason_t reason,
                          uint64_t now_s, uint8_t local_hour,
                          char *out, size_t out_size);

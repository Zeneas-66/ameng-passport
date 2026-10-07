#pragma once
#include "ameng_pet.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    AMENG_LINE_IDLE = 0,
    AMENG_LINE_CALL_SAME,
    AMENG_LINE_CALL_COMES,
    AMENG_LINE_CALL_FAR,
    AMENG_LINE_FEED,
    AMENG_LINE_PET,
    AMENG_LINE_PLAY,
    AMENG_LINE_TALK,
    AMENG_LINE_TIRED,
} ameng_line_reason_t;

/* Returns only the meaning inside parentheses. */
void ameng_dialogue_local_meaning(const ameng_state_t *state,
                                  ameng_line_reason_t reason,
                                  uint64_t now_s, uint8_t local_hour,
                                  char *out, size_t out_size);

/* Wrap a meaning as varying cat vocalization + Chinese parenthesized meaning. */
void ameng_dialogue_wrap(const ameng_state_t *state,
                         ameng_line_reason_t reason,
                         uint64_t now_s,
                         const char *meaning,
                         char *out, size_t out_size);

void ameng_dialogue_local(const ameng_state_t *state,
                          ameng_line_reason_t reason,
                          uint64_t now_s, uint8_t local_hour,
                          char *out, size_t out_size);

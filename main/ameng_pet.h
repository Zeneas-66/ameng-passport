#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    AMENG_STAGE_RETURNED = 0,
    AMENG_STAGE_SETTLING,
    AMENG_STAGE_FAMILIAR,
    AMENG_STAGE_HOME,
} ameng_stage_t;

typedef enum {
    AMENG_ACTION_CALL = 0,
    AMENG_ACTION_FEED,
    AMENG_ACTION_PET,
    AMENG_ACTION_CHIN_SCRATCH,
    AMENG_ACTION_PLAY_BIRD,
} ameng_action_t;

typedef enum {
    AMENG_BEHAVIOR_IDLE = 0,
    AMENG_BEHAVIOR_HUNGRY,
    AMENG_BEHAVIOR_NAP,
    AMENG_BEHAVIOR_WAIT,
    AMENG_BEHAVIOR_APPROACH,
    AMENG_BEHAVIOR_LEG_HUG,
    AMENG_BEHAVIOR_POUNCE,
} ameng_behavior_t;

typedef struct {
    uint8_t hunger;       /* 0 = full, 100 = very hungry */
    uint8_t energy;       /* 0..100 */
    uint8_t mood;         /* 0..100 */
    uint8_t trust;        /* 0..100 */
    uint8_t bond;         /* 0..100 */
    uint8_t playfulness;  /* learned tendency, 0..100 */
    uint8_t routine;      /* learned routine strength, 0..100 */

    uint32_t days_together;
    uint32_t interactions;
    uint32_t feeds;
    uint32_t pets;
    uint32_t calls;
    uint32_t bird_games;

    uint64_t born_at_s;
    uint64_t last_update_s;
    uint64_t last_interaction_s;
    uint64_t last_feed_s;
} ameng_state_t;

void ameng_state_init(ameng_state_t *state, uint64_t now_s);
void ameng_state_advance(ameng_state_t *state, uint64_t now_s, uint8_t local_hour);
void ameng_state_interact(ameng_state_t *state, ameng_action_t action, uint64_t now_s);
ameng_stage_t ameng_state_stage(const ameng_state_t *state);
ameng_behavior_t ameng_state_behavior(const ameng_state_t *state, uint64_t now_s,
                                      uint8_t local_hour);

/*
 * Build a compact, factual context for an optional AI dialogue layer.
 * The text intentionally contains state/facts only; the model should never
 * invent biographical memories that are not present in the curated archive.
 */
int ameng_state_ai_context(const ameng_state_t *state, uint64_t now_s,
                           uint8_t local_hour, char *out, size_t out_size);

const char *ameng_stage_name(ameng_stage_t stage);
const char *ameng_behavior_name(ameng_behavior_t behavior);

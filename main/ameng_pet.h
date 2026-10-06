#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    AMENG_ROOM_BEDROOM = 0,
    AMENG_ROOM_LIVING,
    AMENG_ROOM_STUDY,
    AMENG_ROOM_COUNT,
} ameng_room_t;

typedef enum {
    AMENG_ACTION_CALL = 0,
    AMENG_ACTION_FEED,
    AMENG_ACTION_PET,
    AMENG_ACTION_PLAY,
    AMENG_ACTION_TALK,
} ameng_action_t;

typedef enum {
    AMENG_BEHAVIOR_IDLE = 0,
    AMENG_BEHAVIOR_HUNGRY,
    AMENG_BEHAVIOR_NAP,
    AMENG_BEHAVIOR_WAIT,
    AMENG_BEHAVIOR_APPROACH,
    AMENG_BEHAVIOR_LEG_HUG,
    AMENG_BEHAVIOR_POUNCE,
    AMENG_BEHAVIOR_SUNBATHE,
} ameng_behavior_t;

typedef enum {
    AMENG_CALL_SAME_ROOM = 0,
    AMENG_CALL_VOICE_ONLY,
    AMENG_CALL_COMES,
} ameng_call_result_t;

typedef struct {
    /* Current / short-term state, 0..100. */
    uint8_t mood;
    uint8_t hunger;
    uint8_t thirst;
    uint8_t energy;
    uint8_t play_drive;
    uint8_t comfort;

    /* Long-term relationship values in tenths: 0..1000 == 0.0..100.0. */
    uint16_t affection_x10;
    uint16_t trust_x10;
    uint16_t familiarity_x10;
    uint16_t attachment_x10;
    uint16_t safety_x10;

    /* Hidden daily counters. */
    uint16_t daily_calls;
    uint16_t daily_feeds;
    uint16_t daily_pets;
    uint16_t daily_games;
    uint16_t daily_talks;
    uint16_t daily_satisfaction;
    uint16_t daily_overstimulation;

    uint32_t days_together;
    uint32_t day_index;
    uint32_t interactions;

    uint64_t born_at_s;
    uint64_t last_update_s;
    uint64_t last_interaction_s;
    uint64_t last_feed_s;
    uint64_t last_room_change_s;

    ameng_room_t cat_room;
    uint8_t cat_depth; /* 0 far, 1 middle, 2 near */
    uint8_t cat_x;     /* 0..100 logical horizontal position */
    bool sleeping;
} ameng_state_t;

void ameng_state_init(ameng_state_t *state, uint64_t now_s);
void ameng_state_advance(ameng_state_t *state, uint64_t now_s, uint8_t local_hour);

bool ameng_state_is_present(const ameng_state_t *state, ameng_room_t player_room);

ameng_call_result_t ameng_state_call(ameng_state_t *state, ameng_room_t player_room,
                                     uint64_t now_s, uint8_t local_hour);

bool ameng_state_interact(ameng_state_t *state, ameng_action_t action,
                          ameng_room_t player_room, uint64_t now_s,
                          uint8_t local_hour);

ameng_behavior_t ameng_state_behavior(const ameng_state_t *state, uint64_t now_s,
                                      uint8_t local_hour);

int ameng_state_ai_context(const ameng_state_t *state, ameng_room_t player_room,
                           uint64_t now_s, uint8_t local_hour,
                           char *out, size_t out_size);

const char *ameng_behavior_name(ameng_behavior_t behavior);
const char *ameng_room_name_cn(ameng_room_t room);
uint8_t ameng_relationship_percent(uint16_t x10);

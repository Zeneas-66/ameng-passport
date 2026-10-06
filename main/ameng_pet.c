#include "ameng_pet.h"

#include <stdio.h>
#include <string.h>

#define SEC_PER_HOUR 3600ULL
#define SEC_PER_DAY 86400ULL

static uint8_t clamp_u8(int value)
{
    if (value < 0) return 0;
    if (value > 100) return 100;
    return (uint8_t)value;
}

static void add_u8(uint8_t *value, int delta)
{
    *value = clamp_u8((int)*value + delta);
}

void ameng_state_init(ameng_state_t *state, uint64_t now_s)
{
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->hunger = 24;
    state->energy = 72;
    state->mood = 62;
    state->trust = 42;
    state->bond = 35;
    state->playfulness = 36;
    state->routine = 10;
    state->born_at_s = now_s;
    state->last_update_s = now_s;
    state->last_interaction_s = now_s;
    state->last_feed_s = now_s;
}

void ameng_state_advance(ameng_state_t *state, uint64_t now_s, uint8_t local_hour)
{
    if (!state || now_s <= state->last_update_s) return;

    uint64_t elapsed_s = now_s - state->last_update_s;
    uint32_t hours = (uint32_t)(elapsed_s / SEC_PER_HOUR);
    if (hours == 0) return;
    if (hours > 168) hours = 168; /* cap one catch-up pass to a week */

    for (uint32_t i = 0; i < hours; ++i) {
        add_u8(&state->hunger, 3);

        bool sleep_hours = (local_hour >= 23 || local_hour < 7);
        if (sleep_hours) {
            add_u8(&state->energy, 5);
            add_u8(&state->mood, 1);
        } else {
            add_u8(&state->energy, -2);
        }

        if (state->hunger > 78) add_u8(&state->mood, -3);
        if (state->energy < 20) add_u8(&state->mood, -2);

        uint64_t absent = now_s > state->last_interaction_s
                        ? now_s - state->last_interaction_s : 0;
        if (absent > 2 * SEC_PER_DAY) add_u8(&state->bond, -1);
    }

    state->days_together = (uint32_t)((now_s - state->born_at_s) / SEC_PER_DAY);
    state->last_update_s += (uint64_t)hours * SEC_PER_HOUR;
}

void ameng_state_interact(ameng_state_t *state, ameng_action_t action, uint64_t now_s)
{
    if (!state) return;

    state->interactions++;
    state->last_interaction_s = now_s;

    switch (action) {
    case AMENG_ACTION_CALL:
        state->calls++;
        add_u8(&state->mood, 3);
        add_u8(&state->trust, 1);
        add_u8(&state->bond, 1);
        break;
    case AMENG_ACTION_FEED:
        state->feeds++;
        add_u8(&state->hunger, -38);
        add_u8(&state->mood, 7);
        add_u8(&state->trust, 2);
        add_u8(&state->bond, 1);
        state->last_feed_s = now_s;
        break;
    case AMENG_ACTION_PET:
        state->pets++;
        add_u8(&state->mood, 5);
        add_u8(&state->trust, 2);
        add_u8(&state->bond, 2);
        break;
    case AMENG_ACTION_CHIN_SCRATCH:
        state->pets++;
        add_u8(&state->mood, 9);
        add_u8(&state->trust, 3);
        add_u8(&state->bond, 2);
        break;
    case AMENG_ACTION_PLAY_BIRD:
        state->bird_games++;
        add_u8(&state->energy, -14);
        add_u8(&state->hunger, 6);
        add_u8(&state->mood, 10);
        add_u8(&state->playfulness, 2);
        add_u8(&state->bond, 2);
        break;
    }

    if (state->interactions % 8 == 0) add_u8(&state->routine, 1);
}

ameng_stage_t ameng_state_stage(const ameng_state_t *state)
{
    if (!state) return AMENG_STAGE_RETURNED;
    if (state->days_together < 3 || state->bond < 42) return AMENG_STAGE_RETURNED;
    if (state->days_together < 14 || state->bond < 58) return AMENG_STAGE_SETTLING;
    if (state->days_together < 45 || state->bond < 76) return AMENG_STAGE_FAMILIAR;
    return AMENG_STAGE_HOME;
}

ameng_behavior_t ameng_state_behavior(const ameng_state_t *state, uint64_t now_s,
                                      uint8_t local_hour)
{
    if (!state) return AMENG_BEHAVIOR_IDLE;
    if (state->hunger >= 82) return AMENG_BEHAVIOR_HUNGRY;
    if (state->energy <= 18 || local_hour >= 23 || local_hour < 6) return AMENG_BEHAVIOR_NAP;

    uint64_t absent = now_s > state->last_interaction_s
                    ? now_s - state->last_interaction_s : 0;

    if (state->bond >= 78 && absent >= 4 * SEC_PER_HOUR) return AMENG_BEHAVIOR_WAIT;
    if (state->bond >= 65 && state->mood >= 70) return AMENG_BEHAVIOR_LEG_HUG;
    if (state->playfulness >= 58 && state->energy >= 45) return AMENG_BEHAVIOR_POUNCE;
    if (state->trust >= 55) return AMENG_BEHAVIOR_APPROACH;
    return AMENG_BEHAVIOR_IDLE;
}

int ameng_state_ai_context(const ameng_state_t *state, uint64_t now_s,
                           uint8_t local_hour, char *out, size_t out_size)
{
    if (!state || !out || out_size == 0) return -1;

    uint64_t absent_s = now_s > state->last_interaction_s
                      ? now_s - state->last_interaction_s : 0;
    uint64_t since_feed_s = now_s > state->last_feed_s
                          ? now_s - state->last_feed_s : 0;

    int written = snprintf(
        out, out_size,
        "AMENG_FACTS\n"
        "hour=%u\n"
        "days_together=%lu\n"
        "stage=%s\n"
        "hunger=%u energy=%u mood=%u trust=%u bond=%u\n"
        "playfulness=%u routine=%u\n"
        "interactions=%lu feeds=%lu pets=%lu calls=%lu bird_games=%lu\n"
        "hours_since_interaction=%llu\n"
        "hours_since_feed=%llu\n"
        "current_behavior=%s\n"
        "persona=quiet,proud,food-loving,aloof-looking-but-affectionate\n"
        "truth_rule=never invent pre-existing memories; use only curated archive facts\n",
        (unsigned)local_hour,
        (unsigned long)state->days_together,
        ameng_stage_name(ameng_state_stage(state)),
        state->hunger, state->energy, state->mood, state->trust, state->bond,
        state->playfulness, state->routine,
        (unsigned long)state->interactions, (unsigned long)state->feeds,
        (unsigned long)state->pets, (unsigned long)state->calls,
        (unsigned long)state->bird_games,
        (unsigned long long)(absent_s / SEC_PER_HOUR),
        (unsigned long long)(since_feed_s / SEC_PER_HOUR),
        ameng_behavior_name(ameng_state_behavior(state, now_s, local_hour)));

    return (written < 0 || (size_t)written >= out_size) ? -1 : written;
}

const char *ameng_stage_name(ameng_stage_t stage)
{
    switch (stage) {
    case AMENG_STAGE_RETURNED: return "returned";
    case AMENG_STAGE_SETTLING: return "settling";
    case AMENG_STAGE_FAMILIAR: return "familiar";
    case AMENG_STAGE_HOME: return "home";
    default: return "unknown";
    }
}

const char *ameng_behavior_name(ameng_behavior_t behavior)
{
    switch (behavior) {
    case AMENG_BEHAVIOR_IDLE: return "idle";
    case AMENG_BEHAVIOR_HUNGRY: return "hungry";
    case AMENG_BEHAVIOR_NAP: return "nap";
    case AMENG_BEHAVIOR_WAIT: return "wait";
    case AMENG_BEHAVIOR_APPROACH: return "approach";
    case AMENG_BEHAVIOR_LEG_HUG: return "leg_hug";
    case AMENG_BEHAVIOR_POUNCE: return "pounce";
    default: return "unknown";
    }
}

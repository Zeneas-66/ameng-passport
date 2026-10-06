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

static uint16_t clamp_rel(int value)
{
    if (value < 0) return 0;
    if (value > 1000) return 1000;
    return (uint16_t)value;
}

static void add_u8(uint8_t *value, int delta)
{
    *value = clamp_u8((int)*value + delta);
}

static void add_rel(uint16_t *value, int delta)
{
    *value = clamp_rel((int)*value + delta);
}

static uint32_t hash32(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

uint8_t ameng_relationship_percent(uint16_t x10)
{
    if (x10 > 1000) x10 = 1000;
    return (uint8_t)((x10 + 5) / 10);
}

static void close_day(ameng_state_t *s)
{
    /* Clicking is deliberately not the main source of relationship growth.
     * A balanced day can move long-term values by roughly 0.1..0.8 points. */
    if (s->daily_satisfaction >= 55) {
        add_rel(&s->affection_x10, 4);
        add_rel(&s->trust_x10, 2);
        add_rel(&s->familiarity_x10, 4);
    } else if (s->daily_satisfaction >= 30) {
        add_rel(&s->affection_x10, 2);
        add_rel(&s->familiarity_x10, 2);
    }

    if (s->daily_calls > 0 && s->daily_pets > 0 && s->daily_games > 0) {
        add_rel(&s->attachment_x10, 3);
    }
    if (s->daily_overstimulation == 0 && s->daily_satisfaction >= 45) {
        add_rel(&s->safety_x10, 2);
    }
    if (s->daily_overstimulation >= 6) {
        add_rel(&s->trust_x10, -2);
    }

    s->daily_calls = 0;
    s->daily_feeds = 0;
    s->daily_pets = 0;
    s->daily_games = 0;
    s->daily_talks = 0;
    s->daily_satisfaction = 0;
    s->daily_overstimulation = 0;
}

static ameng_room_t scheduled_room(const ameng_state_t *s, uint8_t hour,
                                   uint32_t day_index)
{
    uint32_t roll = hash32(day_index * 97U + (uint32_t)hour * 131U +
                           (uint32_t)s->familiarity_x10) % 100U;

    if (hour < 7 || hour >= 23) return AMENG_ROOM_BEDROOM;

    if (hour >= 9 && hour <= 15) {
        if (roll < 64) return AMENG_ROOM_LIVING; /* window / sun time */
        if (roll < 88) return AMENG_ROOM_STUDY;
        return AMENG_ROOM_BEDROOM;
    }

    if (hour >= 16 && hour <= 20) {
        if (roll < 46) return AMENG_ROOM_STUDY;
        if (roll < 86) return AMENG_ROOM_LIVING;
        return AMENG_ROOM_BEDROOM;
    }

    if (roll < 48) return AMENG_ROOM_LIVING;
    if (roll < 72) return AMENG_ROOM_STUDY;
    return AMENG_ROOM_BEDROOM;
}

static void maybe_move_room(ameng_state_t *s, uint64_t now_s, uint8_t hour)
{
    /* Do not teleport away during a fresh interaction. */
    if (now_s > s->last_interaction_s &&
        now_s - s->last_interaction_s < 30ULL * 60ULL) {
        return;
    }
    if (now_s > s->last_room_change_s &&
        now_s - s->last_room_change_s < SEC_PER_HOUR) {
        return;
    }

    ameng_room_t target = scheduled_room(s, hour, s->day_index);
    if (target != s->cat_room) {
        s->cat_room = target;
        s->cat_depth = 0;
        s->cat_x = (uint8_t)(18 + hash32((uint32_t)now_s) % 65U);
        s->last_room_change_s = now_s;
    }
}

void ameng_state_init(ameng_state_t *s, uint64_t now_s)
{
    if (!s) return;
    memset(s, 0, sizeof(*s));

    s->mood = 72;
    s->hunger = 28;
    s->thirst = 24;
    s->energy = 76;
    s->play_drive = 52;
    s->comfort = 74;

    /* Ameng is not a blank pet: the relationship begins recognizable but has
     * room to deepen over months of this new digital life. */
    s->affection_x10 = 560;
    s->trust_x10 = 550;
    s->familiarity_x10 = 500;
    s->attachment_x10 = 420;
    s->safety_x10 = 520;

    s->born_at_s = now_s;
    s->last_update_s = now_s;
    s->last_interaction_s = now_s;
    s->last_feed_s = now_s;
    s->last_room_change_s = now_s;
    s->day_index = (uint32_t)(now_s / SEC_PER_DAY);
    s->cat_room = AMENG_ROOM_LIVING;
    s->cat_depth = 1;
    s->cat_x = 54;
}

void ameng_state_advance(ameng_state_t *s, uint64_t now_s, uint8_t local_hour)
{
    if (!s || now_s <= s->last_update_s) return;

    uint32_t new_day = (uint32_t)(now_s / SEC_PER_DAY);
    while (s->day_index < new_day) {
        close_day(s);
        s->day_index++;
    }
    s->days_together = (uint32_t)((now_s - s->born_at_s) / SEC_PER_DAY);

    uint64_t elapsed_s = now_s - s->last_update_s;
    uint32_t hours = (uint32_t)(elapsed_s / SEC_PER_HOUR);
    if (hours > 168) hours = 168;

    for (uint32_t i = 0; i < hours; ++i) {
        add_u8(&s->hunger, 3);
        add_u8(&s->thirst, 4);
        add_u8(&s->play_drive, 3);

        bool night = (local_hour >= 23 || local_hour < 7);
        if (night) {
            add_u8(&s->energy, 7);
            add_u8(&s->comfort, 2);
        } else {
            add_u8(&s->energy, -2);
        }

        if (s->hunger > 78) add_u8(&s->mood, -3);
        if (s->thirst > 80) add_u8(&s->mood, -3);
        if (s->energy < 20) add_u8(&s->mood, -2);
        if (s->comfort < 30) add_u8(&s->mood, -2);

        /* Ameng can find water by himself. Thirst is a living-state signal,
         * not a button-spam chore. */
        if (s->thirst >= 86) {
            add_u8(&s->thirst, -58);
            add_u8(&s->comfort, 2);
        }
    }

    if (hours > 0) {
        s->last_update_s += (uint64_t)hours * SEC_PER_HOUR;
    }

    s->sleeping = (local_hour >= 23 || local_hour < 6 || s->energy < 16);
    maybe_move_room(s, now_s, local_hour);
}

bool ameng_state_is_present(const ameng_state_t *s, ameng_room_t player_room)
{
    return s && s->cat_room == player_room;
}

ameng_call_result_t ameng_state_call(ameng_state_t *s, ameng_room_t player_room,
                                     uint64_t now_s, uint8_t local_hour)
{
    if (!s) return AMENG_CALL_VOICE_ONLY;

    s->interactions++;
    s->daily_calls++;
    s->last_interaction_s = now_s;

    if (s->daily_calls <= 3) {
        add_rel(&s->familiarity_x10, 2);
        s->daily_satisfaction = clamp_u8((int)s->daily_satisfaction + 2);
    } else if (s->daily_calls >= 8) {
        s->daily_overstimulation++;
    }

    if (s->cat_room == player_room) {
        s->cat_depth = s->cat_depth < 1 ? 1 : s->cat_depth;
        return AMENG_CALL_SAME_ROOM;
    }

    int chance = 18;
    chance += ameng_relationship_percent(s->affection_x10) / 5;
    chance += ameng_relationship_percent(s->trust_x10) / 5;
    chance += s->mood / 8;
    chance += s->energy / 10;
    if (s->hunger >= 68) chance += 18;
    if (s->sleeping || local_hour >= 23 || local_hour < 6) chance -= 28;
    if (s->daily_calls > 5) chance -= (int)(s->daily_calls - 5) * 4;
    if (chance < 8) chance = 8;
    if (chance > 92) chance = 92;

    uint32_t roll = hash32((uint32_t)now_s ^ ((uint32_t)s->daily_calls << 17) ^
                           ((uint32_t)s->mood << 8)) % 100U;

    if ((int)roll < chance) {
        s->cat_room = player_room;
        s->cat_depth = 0;
        s->cat_x = (roll & 1U) ? 5 : 92;
        s->last_room_change_s = now_s;
        add_u8(&s->energy, -1);
        return AMENG_CALL_COMES;
    }

    return AMENG_CALL_VOICE_ONLY;
}

static int diminishing_gain(uint16_t count, int first, int second)
{
    if (count <= 2) return first;
    if (count <= 4) return second;
    return 0;
}

bool ameng_state_interact(ameng_state_t *s, ameng_action_t action,
                          ameng_room_t player_room, uint64_t now_s,
                          uint8_t local_hour)
{
    (void)local_hour;
    if (!s || action == AMENG_ACTION_CALL) return false;
    if (!ameng_state_is_present(s, player_room)) return false;

    s->interactions++;
    s->last_interaction_s = now_s;
    s->cat_depth = 2;

    switch (action) {
    case AMENG_ACTION_FEED: {
        uint8_t before = s->hunger;
        s->daily_feeds++;
        add_u8(&s->hunger, -42);
        add_u8(&s->thirst, -6);
        add_u8(&s->mood, before >= 55 ? 8 : 2);
        add_u8(&s->comfort, 4);
        s->last_feed_s = now_s;
        if (s->daily_feeds <= 2 && before >= 45) {
            add_rel(&s->trust_x10, 2);
            s->daily_satisfaction = clamp_u8((int)s->daily_satisfaction + 10);
        } else if (before < 25 && s->daily_feeds > 2) {
            s->daily_overstimulation++;
        }
        break;
    }
    case AMENG_ACTION_PET: {
        s->daily_pets++;
        int gain = diminishing_gain(s->daily_pets, 3, 1);
        add_u8(&s->mood, 7);
        add_u8(&s->comfort, 10);
        add_rel(&s->affection_x10, gain);
        add_rel(&s->trust_x10, gain > 0 ? 2 : 0);
        if (s->daily_pets <= 4) {
            s->daily_satisfaction = clamp_u8((int)s->daily_satisfaction + 8);
        } else if (s->daily_pets >= 7) {
            s->daily_overstimulation++;
            add_u8(&s->comfort, -5);
            if (s->daily_pets >= 10) add_u8(&s->mood, -2);
        }
        break;
    }
    case AMENG_ACTION_PLAY: {
        if (s->energy < 14) return false;
        s->daily_games++;
        int gain = diminishing_gain(s->daily_games, 2, 1);
        add_u8(&s->energy, -15);
        add_u8(&s->hunger, 6);
        add_u8(&s->thirst, 5);
        add_u8(&s->play_drive, -34);
        add_u8(&s->mood, 10);
        add_rel(&s->affection_x10, gain);
        add_rel(&s->familiarity_x10, gain);
        if (s->daily_games <= 3) {
            s->daily_satisfaction = clamp_u8((int)s->daily_satisfaction + 12);
        } else {
            s->daily_overstimulation++;
        }
        break;
    }
    case AMENG_ACTION_TALK: {
        s->daily_talks++;
        add_u8(&s->mood, 2);
        int gain = diminishing_gain(s->daily_talks, 2, 1);
        add_rel(&s->familiarity_x10, gain);
        if (s->daily_talks <= 3) {
            s->daily_satisfaction = clamp_u8((int)s->daily_satisfaction + 4);
        }
        break;
    }
    default:
        return false;
    }

    return true;
}

ameng_behavior_t ameng_state_behavior(const ameng_state_t *s, uint64_t now_s,
                                      uint8_t local_hour)
{
    if (!s) return AMENG_BEHAVIOR_IDLE;
    if (s->hunger >= 82) return AMENG_BEHAVIOR_HUNGRY;
    if (s->sleeping || s->energy <= 16) return AMENG_BEHAVIOR_NAP;
    if (s->cat_room == AMENG_ROOM_LIVING && local_hour >= 9 && local_hour <= 15 &&
        s->comfort >= 55) {
        return AMENG_BEHAVIOR_SUNBATHE;
    }

    uint64_t absent = now_s > s->last_interaction_s
                    ? now_s - s->last_interaction_s : 0;
    if (ameng_relationship_percent(s->attachment_x10) >= 62 &&
        absent >= 4ULL * SEC_PER_HOUR) {
        return AMENG_BEHAVIOR_WAIT;
    }
    if (ameng_relationship_percent(s->affection_x10) >= 64 &&
        s->mood >= 72 && s->cat_depth == 2) {
        return AMENG_BEHAVIOR_LEG_HUG;
    }
    if (s->play_drive >= 72 && s->energy >= 48) return AMENG_BEHAVIOR_POUNCE;
    if (ameng_relationship_percent(s->trust_x10) >= 55) return AMENG_BEHAVIOR_APPROACH;
    return AMENG_BEHAVIOR_IDLE;
}

int ameng_state_ai_context(const ameng_state_t *s, ameng_room_t player_room,
                           uint64_t now_s, uint8_t local_hour,
                           char *out, size_t out_size)
{
    if (!s || !out || out_size == 0) return -1;

    uint64_t absent_s = now_s > s->last_interaction_s
                      ? now_s - s->last_interaction_s : 0;

    int written = snprintf(
        out, out_size,
        "AMENG_FACTS\n"
        "hour=%u\n"
        "player_room=%u cat_room=%u together=%u\n"
        "mood=%u hunger=%u thirst=%u energy=%u play=%u comfort=%u\n"
        "affection=%u trust=%u familiarity=%u attachment=%u safety=%u\n"
        "days=%lu hours_since_interaction=%llu\n"
        "behavior=%s\n"
        "persona=quiet,proud,food-loving,aloof-looking-but-affectionate\n"
        "truth_rule=never invent pre-existing memories; use only supplied facts\n",
        (unsigned)local_hour,
        (unsigned)player_room, (unsigned)s->cat_room,
        ameng_state_is_present(s, player_room) ? 1U : 0U,
        s->mood, s->hunger, s->thirst, s->energy, s->play_drive, s->comfort,
        ameng_relationship_percent(s->affection_x10),
        ameng_relationship_percent(s->trust_x10),
        ameng_relationship_percent(s->familiarity_x10),
        ameng_relationship_percent(s->attachment_x10),
        ameng_relationship_percent(s->safety_x10),
        (unsigned long)s->days_together,
        (unsigned long long)(absent_s / SEC_PER_HOUR),
        ameng_behavior_name(ameng_state_behavior(s, now_s, local_hour)));

    return (written < 0 || (size_t)written >= out_size) ? -1 : written;
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
    case AMENG_BEHAVIOR_SUNBATHE: return "sunbathe";
    default: return "unknown";
    }
}

const char *ameng_room_name_cn(ameng_room_t room)
{
    switch (room) {
    case AMENG_ROOM_BEDROOM: return "卧室";
    case AMENG_ROOM_LIVING: return "客厅";
    case AMENG_ROOM_STUDY: return "书房";
    default: return "家里";
    }
}

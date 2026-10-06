#include <assert.h>
#include <string.h>

#include "ameng_pet.h"

#define HOUR 3600ULL
#define DAY 86400ULL

int main(void)
{
    ameng_state_t s;
    ameng_state_init(&s, 12 * HOUR);

    assert(s.hunger == 28);
    assert(s.thirst == 24);
    assert(s.energy == 76);
    assert(s.cat_room == AMENG_ROOM_LIVING);
    assert(ameng_relationship_percent(s.affection_x10) == 56);

    ameng_state_advance(&s, 16 * HOUR, 16);
    assert(s.hunger == 40);
    assert(s.thirst == 40);
    assert(s.energy == 68);

    /* Only CALL works remotely. Other interactions require the same room. */
    s.cat_room = AMENG_ROOM_LIVING;
    assert(!ameng_state_interact(&s, AMENG_ACTION_TALK, AMENG_ROOM_BEDROOM,
                                 16 * HOUR + 1, 16));

    uint8_t before = s.hunger;
    assert(ameng_state_interact(&s, AMENG_ACTION_FEED, AMENG_ROOM_LIVING,
                                16 * HOUR + 2, 16));
    assert(s.hunger < before);

    /* Repeating touch in one day has sharply diminishing long-term gain. */
    uint16_t affection_before = s.affection_x10;
    for (int i = 0; i < 10; ++i) {
        assert(ameng_state_interact(&s, AMENG_ACTION_PET, AMENG_ROOM_LIVING,
                                    16 * HOUR + 10 + i, 16));
    }
    assert(s.affection_x10 > affection_before);
    assert(s.affection_x10 - affection_before <= 10);
    assert(s.daily_overstimulation > 0);

    /* Calling in the same room always answers without teleporting. */
    assert(ameng_state_call(&s, AMENG_ROOM_LIVING, 17 * HOUR, 17)
           == AMENG_CALL_SAME_ROOM);

    /* A completed good day changes relationship slowly, not by tens of points. */
    s.daily_satisfaction = 70;
    uint16_t rel_before = s.affection_x10;
    uint32_t next_day = s.day_index + 1;
    ameng_state_advance(&s, (uint64_t)next_day * DAY + HOUR, 1);
    assert(s.affection_x10 >= rel_before);
    assert(s.affection_x10 - rel_before <= 8);

    char context[896];
    int n = ameng_state_ai_context(&s, AMENG_ROOM_BEDROOM,
                                   (uint64_t)next_day * DAY + HOUR, 1,
                                   context, sizeof(context));
    assert(n > 0);
    assert(strstr(context, "AMENG_FACTS") != 0);
    assert(strstr(context, "truth_rule=never invent") != 0);

    return 0;
}

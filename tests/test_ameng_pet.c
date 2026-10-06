#include <assert.h>
#include <string.h>

#include "ameng_pet.h"

#define HOUR 3600ULL
#define DAY 86400ULL

int main(void)
{
    ameng_state_t s;
    ameng_state_init(&s, 1000);

    assert(s.hunger == 24);
    assert(ameng_state_stage(&s) == AMENG_STAGE_RETURNED);

    ameng_state_advance(&s, 1000 + 4 * HOUR, 12);
    assert(s.hunger == 36);
    assert(s.energy == 64);

    uint8_t before_hunger = s.hunger;
    ameng_state_interact(&s, AMENG_ACTION_FEED, 1000 + 4 * HOUR);
    assert(s.hunger < before_hunger);
    assert(s.feeds == 1);

    for (int i = 0; i < 20; ++i) {
        ameng_state_interact(&s, AMENG_ACTION_CHIN_SCRATCH, 2000 + i);
    }
    assert(s.bond > 65);
    assert(s.trust > 55);

    s.days_together = 50;
    s.bond = 85;
    assert(ameng_state_stage(&s) == AMENG_STAGE_HOME);

    s.last_interaction_s = 1000;
    s.hunger = 40;
    s.energy = 70;
    s.mood = 80;
    assert(ameng_state_behavior(&s, 1000 + 5 * HOUR, 20) == AMENG_BEHAVIOR_WAIT);

    char context[768];
    int n = ameng_state_ai_context(&s, 1000 + 5 * HOUR, 20, context, sizeof(context));
    assert(n > 0);
    assert(strstr(context, "AMENG_FACTS") != 0);
    assert(strstr(context, "truth_rule=never invent") != 0);

    ameng_state_t tired = s;
    tired.energy = 10;
    tired.hunger = 20;
    assert(ameng_state_behavior(&tired, 1000 + DAY, 14) == AMENG_BEHAVIOR_NAP);

    return 0;
}

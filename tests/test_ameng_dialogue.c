#include <assert.h>
#include <string.h>

#include "ameng_dialogue.h"
#include "ameng_pet.h"

int main(void)
{
    ameng_state_t s;
    char out[96];

    ameng_state_init(&s, 0);
    s.hunger = 90;
    ameng_dialogue_local(&s, AMENG_LINE_TALK, 100, 12, out, sizeof(out));
    assert(strlen(out) > 0);

    s.hunger = 20;
    s.energy = 80;
    s.bond = 85;
    s.days_together = 50;
    s.last_interaction_s = 0;
    ameng_dialogue_local(&s, AMENG_LINE_TALK, 5ULL * 3600ULL, 12, out, sizeof(out));
    assert(strcmp(out, "You're late.") == 0);

    return 0;
}

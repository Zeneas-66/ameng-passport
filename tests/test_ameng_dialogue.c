#include <assert.h>
#include <string.h>

#include "ameng_dialogue.h"
#include "ameng_pet.h"

int main(void)
{
    ameng_state_t s;
    char out[192];
    char meaning[96];

    ameng_state_init(&s, 0);
    s.hunger = 90;

    ameng_dialogue_local(&s, AMENG_LINE_TALK, 100, 12, out, sizeof(out));
    assert(strstr(out, "MEOW") != 0 || strstr(out, "MROW") != 0 || strstr(out, "MEE") != 0);
    assert(strstr(out, "(") != 0);
    assert(strstr(out, ")") != 0);

    s.hunger = 20;
    s.energy = 80;
    s.sleeping = false;
    ameng_dialogue_local_meaning(&s, AMENG_LINE_CALL_COMES,
                                 200, 12, meaning, sizeof(meaning));
    assert(strlen(meaning) > 0);

    ameng_dialogue_wrap(&s, AMENG_LINE_CALL_COMES, 200,
                        "COMING", out, sizeof(out));
    assert(strstr(out, "COMING") != 0);
    assert(strstr(out, "MEOW") != 0 || strstr(out, "MROW") != 0 || strstr(out, "MEE") != 0);

    return 0;
}

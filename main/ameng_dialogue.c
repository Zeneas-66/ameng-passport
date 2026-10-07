#include "ameng_dialogue.h"
#include <stdio.h>

static const char *pick(uint32_t key, const char *const *lines, size_t count)
{
    return lines[key % count];
}

void ameng_dialogue_local_meaning(const ameng_state_t *s,
                                  ameng_line_reason_t reason,
                                  uint64_t now_s, uint8_t local_hour,
                                  char *out, size_t out_size)
{
    if (!out || out_size == 0) return;
    if (!s) {
        snprintf(out, out_size, "HUH?");
        return;
    }

    uint32_t key = (uint32_t)(now_s / 7ULL) + s->interactions +
                   (uint32_t)local_hour * 13U;
    const char *line = "HUH?";

    if (s->hunger >= 84) {
        static const char *const hungry[] = {
            "FEED ME FIRST", "WHERE'S MY BOWL", "I'M HUNGRY", "FORGOT SOMETHING?"
        };
        line = pick(key, hungry, 4);
    } else if (s->sleeping || s->energy <= 16) {
        static const char *const sleepy[] = {
            "I'M SLEEPING", "FIVE MORE MINUTES", "I HEARD YOU", "QUIETER"
        };
        line = pick(key, sleepy, 4);
    } else {
        switch (reason) {
        case AMENG_LINE_CALL_SAME: {
            static const char *const lines[] = {
                "I'M RIGHT HERE", "I HEARD YOU", "WHAT", "I SEE YOU"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_CALL_COMES: {
            static const char *const lines[] = {
                "COMING", "WAIT A SECOND", "ON MY WAY", "I KNOW"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_CALL_FAR: {
            static const char *const lines[] = {
                "I HEARD YOU. NOT COMING", "I'M OVER HERE", "I KNOW", "MAYBE LATER"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_FEED: {
            static const char *const lines[] = {
                "THAT'S BETTER", "FOOD FIRST", "A LITTLE MORE", "NOT BAD"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_PET: {
            static const char *const lines[] = {
                "UNDER THE CHIN", "DON'T STOP", "NOT BAD", "A LITTLE LONGER"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_PLAY: {
            static const char *const lines[] = {
                "GOT IT", "IT'S MINE", "DON'T TAKE IT", "AGAIN"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_TALK:
        case AMENG_LINE_IDLE:
        default: {
            if (ameng_relationship_percent(s->attachment_x10) >= 68) {
                static const char *const close[] = {
                    "YOU'RE BACK", "SIT A WHILE", "I'M STILL HERE", "ON TIME TODAY"
                };
                line = pick(key, close, 4);
            } else {
                static const char *const idle[] = {
                    "HUH?", "WHAT IS IT", "I'M LISTENING", "ONE LOOK"
                };
                line = pick(key, idle, 4);
            }
            break;
        }
        case AMENG_LINE_TIRED:
            line = "NOT PLAYING NOW";
            break;
        }
    }

    snprintf(out, out_size, "%s", line);
}

void ameng_dialogue_wrap(const ameng_state_t *s,
                         ameng_line_reason_t reason,
                         uint64_t now_s,
                         const char *meaning,
                         char *out, size_t out_size)
{
    if (!out || out_size == 0) return;

    static const char *const meows[] = {
        "MEOW.", "MEEOW~", "MEEEOOW--", "MEOW, MEOW.", "MRROW...", "MEOW!",
        "MROW...", "MEOW MEOW~"
    };
    uint32_t key = (uint32_t)(now_s / 3ULL) + (uint32_t)reason * 17U;
    if (s) key += s->interactions * 7U + s->mood;

    const char *voice = meows[key % (sizeof(meows) / sizeof(meows[0]))];
    snprintf(out, out_size, "%s (%s)", voice, meaning ? meaning : "HUH?");
}

void ameng_dialogue_local(const ameng_state_t *s,
                          ameng_line_reason_t reason,
                          uint64_t now_s, uint8_t local_hour,
                          char *out, size_t out_size)
{
    char meaning[96];
    ameng_dialogue_local_meaning(s, reason, now_s, local_hour,
                                 meaning, sizeof(meaning));
    ameng_dialogue_wrap(s, reason, now_s, meaning, out, out_size);
}

#include "ameng_dialogue.h"
#include <stdio.h>

static const char *pick(uint32_t key, const char *const *lines, size_t count)
{
    return lines[key % count];
}

void ameng_dialogue_local(const ameng_state_t *s, ameng_line_reason_t reason,
                          uint64_t now_s, uint8_t local_hour,
                          char *out, size_t out_size)
{
    if (!out || out_size == 0) return;
    if (!s) {
        snprintf(out, out_size, "...");
        return;
    }

    uint32_t key = s->interactions + s->days_together + (uint32_t)local_hour;
    const char *line = "...";

    if (s->hunger >= 82) {
        static const char *const hungry[] = {"Food.", "You forgot something.", "My bowl."};
        line = pick(key, hungry, 3);
    } else if (s->energy <= 18 || local_hour >= 23 || local_hour < 6) {
        static const char *const sleepy[] = {"I'm sleeping.", "Stay. Quietly.", "Mrrr..."};
        line = pick(key, sleepy, 3);
    } else {
        switch (reason) {
        case AMENG_LINE_CALL: {
            static const char *const lines[] = {"Mraow...", "I heard you.", "Coming."};
            line = pick(key, lines, 3);
            break;
        }
        case AMENG_LINE_FEED: {
            static const char *const lines[] = {"Good.", "More would be fine.", "Food first."};
            line = pick(key, lines, 3);
            break;
        }
        case AMENG_LINE_PET: {
            static const char *const lines[] = {"Acceptable.", "Don't stop yet.", "Mrrr."};
            line = pick(key, lines, 3);
            break;
        }
        case AMENG_LINE_CHIN: {
            static const char *const lines[] = {"Yes. There.", "That spot.", "...keep going."};
            line = pick(key, lines, 3);
            break;
        }
        case AMENG_LINE_PLAY: {
            static const char *const lines[] = {"Mine.", "Got it.", "Don't take the bird."};
            line = pick(key, lines, 3);
            break;
        }
        case AMENG_LINE_TALK:
        case AMENG_LINE_IDLE:
        default: {
            uint64_t absent = now_s > s->last_interaction_s ? now_s - s->last_interaction_s : 0;
            if (s->bond >= 78 && absent >= 4ULL * 3600ULL) {
                line = "You're late.";
            } else if (ameng_state_stage(s) == AMENG_STAGE_HOME) {
                static const char *const home[] = {"I was here.", "Sit with me.", "You're back."};
                line = pick(key, home, 3);
            } else {
                static const char *const idle[] = {"Hm.", "...", "What?"};
                line = pick(key, idle, 3);
            }
            break;
        }
        }
    }
    snprintf(out, out_size, "%s", line);
}

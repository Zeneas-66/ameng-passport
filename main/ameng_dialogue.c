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
        snprintf(out, out_size, "嗯？");
        return;
    }

    uint32_t key = (uint32_t)(now_s / 7ULL) + s->interactions +
                   (uint32_t)local_hour * 13U;
    const char *line = "嗯？";

    if (s->hunger >= 84) {
        static const char *const hungry[] = {
            "先喂我", "碗呢", "饿了", "你是不是忘了什么"
        };
        line = pick(key, hungry, 4);
    } else if (s->sleeping || s->energy <= 16) {
        static const char *const sleepy[] = {
            "困着呢", "让我再睡会儿", "听见了，不想动", "小声点"
        };
        line = pick(key, sleepy, 4);
    } else {
        switch (reason) {
        case AMENG_LINE_CALL_SAME: {
            static const char *const lines[] = {
                "我就在这", "听见了", "干嘛", "看见你了"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_CALL_COMES: {
            static const char *const lines[] = {
                "来了", "等我一下", "这就过来", "知道了"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_CALL_FAR: {
            static const char *const lines[] = {
                "听见了，不过去", "我在这边", "知道了", "等会儿"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_FEED: {
            static const char *const lines[] = {
                "这还差不多", "先吃饭", "再来一点也行", "嗯，不错"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_PET: {
            static const char *const lines[] = {
                "下巴这里", "别停", "还可以", "再摸会儿"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_PLAY: {
            static const char *const lines[] = {
                "抓到了", "是我的", "别拿走", "再来一次"
            };
            line = pick(key, lines, 4);
            break;
        }
        case AMENG_LINE_TALK:
        case AMENG_LINE_IDLE:
        default: {
            if (ameng_relationship_percent(s->attachment_x10) >= 68) {
                static const char *const close[] = {
                    "你回来啦", "坐一会儿", "我一直在", "今天还挺准时"
                };
                line = pick(key, close, 4);
            } else {
                static const char *const idle[] = {
                    "嗯？", "什么事", "我听着", "看你一眼"
                };
                line = pick(key, idle, 4);
            }
            break;
        }
        case AMENG_LINE_TIRED:
            line = "今天不想玩了";
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
        "喵。", "喵~", "喵——", "喵、喵。", "喵呜……", "喵！",
        "喵……", "喵喵~"
    };
    uint32_t key = (uint32_t)(now_s / 3ULL) + (uint32_t)reason * 17U;
    if (s) key += s->interactions * 7U + s->mood;

    const char *voice = meows[key % (sizeof(meows) / sizeof(meows[0]))];
    snprintf(out, out_size, "%s（%s）", voice, meaning ? meaning : "嗯？");
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

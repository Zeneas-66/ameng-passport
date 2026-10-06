#include "ameng_ui.h"

#include <stdio.h>
#include <string.h>

#define C_BG        0xF1E7D5
#define C_PAPER     0xFFF9EF
#define C_INK       0x302B28
#define C_MUTED     0x8B8178
#define C_ACCENT    0xB36F50
#define C_GOLD      0xC89B55
#define C_FUR       0xF8F4EC
#define C_FUR_SH    0xD9D0C3
#define C_EYE       0x91AF8B
#define C_WINDOW    0xA9CED8
#define C_WOOD      0xA97C59
#define C_WOOD_D    0x7B5742
#define C_SOFA      0xAFA391
#define C_GREEN     0x748861
#define C_BLUE      0x8099A9
#define C_DARK      0x252321

static const lv_font_t *font_cn(void)
{
    return &lv_font_source_han_sans_sc_16_cjk;
}

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h,
                     uint32_t color, int radius)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_radius(o, radius, 0);
    return o;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text,
                       const lv_font_t *font, uint32_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

static void hide(lv_obj_t *o, bool yes)
{
    if (!o) return;
    if (yes) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
}

static int sc(int v, int size)
{
    int r = v * size / 80;
    return r < 1 ? 1 : r;
}

static ameng_cat_ui_t cat_create(lv_obj_t *parent, int size)
{
    ameng_cat_ui_t c = {0};
    c.w = size;
    c.h = sc(68, size);

    c.group = lv_obj_create(parent);
    lv_obj_remove_flag(c.group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(c.group, c.w, c.h);
    lv_obj_set_style_bg_opa(c.group, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(c.group, 0, 0);
    lv_obj_set_style_pad_all(c.group, 0, 0);

    /* Tail behind the body. */
    c.tail = box(c.group, sc(58,size), sc(39,size), sc(19,size), sc(12,size),
                 C_GOLD, sc(6,size));

    /* Fluffy body and chest. */
    box(c.group, sc(16,size), sc(32,size), sc(50,size), sc(31,size),
        C_FUR_SH, sc(15,size));
    c.body = box(c.group, sc(13,size), sc(29,size), sc(51,size), sc(31,size),
                 C_FUR, sc(16,size));
    box(c.group, sc(27,size), sc(40,size), sc(21,size), sc(20,size),
        0xFFFDF8, sc(9,size));

    /* Broad lion-like mane + head. */
    box(c.group, sc(12,size), sc(4,size), sc(52,size), sc(42,size),
        C_FUR_SH, sc(17,size));
    c.head = box(c.group, sc(15,size), sc(5,size), sc(47,size), sc(38,size),
                 C_FUR, sc(15,size));

    /* Ears. */
    box(c.group, sc(17,size), sc(1,size), sc(13,size), sc(15,size),
        C_FUR, sc(3,size));
    box(c.group, sc(48,size), sc(1,size), sc(13,size), sc(15,size),
        C_FUR, sc(3,size));

    /* IMPORTANT: Ameng has pale yellow fur on BOTH sides of the crown. */
    c.patch_l = box(c.group, sc(19,size), sc(7,size), sc(14,size), sc(10,size),
                    C_GOLD, sc(4,size));
    c.patch_r = box(c.group, sc(45,size), sc(6,size), sc(13,size), sc(11,size),
                    C_GOLD, sc(4,size));

    c.eye_l = box(c.group, sc(27,size), sc(23,size), sc(6,size), sc(4,size),
                  C_EYE, sc(2,size));
    c.eye_r = box(c.group, sc(45,size), sc(23,size), sc(6,size), sc(4,size),
                  C_EYE, sc(2,size));
    box(c.group, sc(36,size), sc(29,size), sc(5,size), sc(3,size),
        0xC78378, sc(2,size));

    /* The lip mark is fixed to the SCREEN-LEFT side. We do not mirror this
     * sprite when Ameng walks, so the asymmetric identity mark never flips. */
    c.lip_patch = box(c.group, sc(28,size), sc(31,size), sc(8,size), sc(5,size),
                      C_GOLD, sc(2,size));

    c.paw_l = box(c.group, sc(21,size), sc(53,size), sc(15,size), sc(12,size),
                  C_FUR, sc(6,size));
    c.paw_r = box(c.group, sc(42,size), sc(53,size), sc(15,size), sc(12,size),
                  C_FUR, sc(6,size));

    hide(c.group, true);
    return c;
}

static void create_living(lv_obj_t *p)
{
    box(p, 0, 0, 240, 125, 0xEDE0C7, 0);
    box(p, 0, 125, 240, 81, 0xC79C73, 0);

    box(p, 13, 12, 76, 73, 0xF7F0E5, 3);
    box(p, 18, 17, 66, 63, C_WINDOW, 1);
    box(p, 49, 17, 3, 63, 0xEEE6D7, 0);
    box(p, 18, 47, 66, 3, 0xEEE6D7, 0);
    box(p, 64, 25, 13, 13, 0xF4D981, 7);

    box(p, 129, 89, 101, 65, C_SOFA, 12);
    box(p, 137, 78, 84, 28, 0xC3B7A4, 10);
    box(p, 145, 103, 33, 8, 0x9F9383, 4);
    box(p, 184, 103, 28, 8, 0x9F9383, 4);
    box(p, 84, 157, 76, 32, 0xD7C3A2, 16);
    box(p, 5, 104, 24, 52, 0x7C8D63, 10);
    box(p, 10, 145, 16, 18, 0x9A7355, 4);
}

static void create_bedroom(lv_obj_t *p)
{
    box(p, 0, 0, 240, 126, 0xE9DED0, 0);
    box(p, 0, 126, 240, 80, 0xB98E6A, 0);

    box(p, 12, 16, 58, 67, 0xF7F1E8, 4);
    box(p, 17, 21, 48, 57, 0xA6C5D0, 2);
    box(p, 12, 14, 8, 76, 0xD1B08A, 4);
    box(p, 63, 14, 8, 76, 0xD1B08A, 4);

    box(p, 80, 92, 150, 72, 0xD9C4AB, 8);
    box(p, 87, 76, 136, 31, 0xF2ECE4, 10);
    box(p, 91, 81, 52, 20, 0xFFF9F0, 9);
    box(p, 80, 153, 150, 11, C_WOOD_D, 2);
    box(p, 6, 129, 53, 34, C_WOOD, 4);
    box(p, 16, 114, 15, 15, 0xD8BE83, 8);
}

static void create_study(lv_obj_t *p)
{
    box(p, 0, 0, 240, 124, 0xDFE2D7, 0);
    box(p, 0, 124, 240, 82, 0xAF845F, 0);

    box(p, 12, 13, 55, 100, 0x8D6B51, 3);
    for (int y = 24; y < 105; y += 24) {
        box(p, 17, y, 45, 4, 0x5F493A, 1);
    }
    box(p, 21, 18, 7, 18, 0xA85B4A, 1);
    box(p, 31, 17, 9, 19, C_BLUE, 1);
    box(p, 43, 20, 6, 16, C_GREEN, 1);

    box(p, 82, 94, 148, 12, C_WOOD_D, 2);
    box(p, 92, 106, 9, 67, C_WOOD_D, 2);
    box(p, 211, 106, 9, 67, C_WOOD_D, 2);
    box(p, 127, 51, 69, 43, C_DARK, 5);
    box(p, 133, 57, 57, 31, 0x8AAAB5, 2);
    box(p, 151, 95, 20, 5, 0x5E5149, 2);
    box(p, 110, 130, 58, 39, 0x8C8175, 10);
    box(p, 128, 164, 10, 30, 0x5D554E, 3);
}

static void create_header(ameng_ui_t *ui)
{
    box(ui->pet_panel, 0, 0, 240, 36, C_DARK, 0);
    ui->room_label = label(ui->pet_panel, "客厅", font_cn(), C_PAPER);
    lv_obj_set_pos(ui->room_label, 12, 8);
    ui->time_label = label(ui->pet_panel, "12:00", &lv_font_montserrat_14, C_PAPER);
    lv_obj_set_pos(ui->time_label, 88, 10);
    ui->battery_label = label(ui->pet_panel, "--%", &lv_font_montserrat_14, C_PAPER);
    lv_obj_align(ui->battery_label, LV_ALIGN_TOP_RIGHT, -12, 10);
}

static void create_status(ameng_ui_t *ui)
{
    ui->status_panel = box(ui->pet_panel, 18, 43, 204, 202, C_PAPER, 10);
    lv_obj_set_style_border_width(ui->status_panel, 2, 0);
    lv_obj_set_style_border_color(ui->status_panel, lv_color_hex(C_GOLD), 0);

    ui->status_title = label(ui->status_panel, "阿猛状态", font_cn(), C_INK);
    lv_obj_set_pos(ui->status_title, 14, 10);

    for (int i = 0; i < 8; ++i) {
        ui->status_lines[i] = label(ui->status_panel, "", font_cn(), C_INK);
        lv_obj_set_pos(ui->status_lines[i], 15, 38 + i * 19);
    }
    hide(ui->status_panel, true);
}

void ameng_ui_create(ameng_ui_t *ui, const char *card_name, const char *card_bio)
{
    memset(ui, 0, sizeof(*ui));

    ui->screen = lv_obj_create(NULL);
    lv_obj_remove_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui->screen, lv_color_hex(C_BG), 0);
    lv_obj_set_style_border_width(ui->screen, 0, 0);
    lv_obj_set_style_pad_all(ui->screen, 0, 0);

    ui->home_panel = box(ui->screen, 0, 0, 240, 320, C_BG, 0);
    ui->card_panel = box(ui->screen, 0, 0, 240, 320, C_BG, 0);
    ui->pet_panel = box(ui->screen, 0, 0, 240, 320, C_BG, 0);
    ui->settings_panel = box(ui->screen, 0, 0, 240, 320, C_BG, 0);

    /* Home / shell: keeps the useful badge + settings structure around Ameng. */
    lv_obj_t *brand = label(ui->home_panel, "AI PASSPORT", &lv_font_montserrat_20, C_INK);
    lv_obj_set_pos(brand, 18, 25);
    lv_obj_t *sub = label(ui->home_panel, "阿猛陪伴版", font_cn(), C_MUTED);
    lv_obj_set_pos(sub, 19, 56);
    static const char *home_names[3] = {"名片", "阿猛", "设置"};
    for (int i = 0; i < 3; ++i) {
        lv_obj_t *card = box(ui->home_panel, 18, 93 + i * 59, 204, 48,
                             i == 1 ? 0xF3DFC9 : C_PAPER, 9);
        ui->home_items[i] = label(card, home_names[i], font_cn(), C_INK);
        lv_obj_center(ui->home_items[i]);
    }
    ui->home_hint = label(ui->home_panel, "上下选择  确认进入", font_cn(), C_MUTED);
    lv_obj_align(ui->home_hint, LV_ALIGN_BOTTOM_MID, 0, -18);

    /* Local badge. Exact factory-mini-program sync is not part of the public
     * firmware, but badge and settings remain first-class local pages. */
    lv_obj_t *ct = label(ui->card_panel, "我的名片", font_cn(), C_INK);
    lv_obj_set_pos(ct, 17, 19);
    lv_obj_t *avatar = box(ui->card_panel, 17, 61, 76, 76, 0xDED7CB, 15);
    lv_obj_t *a = label(avatar, "A", &lv_font_montserrat_20, C_MUTED);
    lv_obj_center(a);
    ui->card_name = label(ui->card_panel, card_name && card_name[0] ? card_name : "AI PASSPORT",
                          font_cn(), C_INK);
    lv_obj_set_width(ui->card_name, 125);
    lv_obj_set_pos(ui->card_name, 106, 72);
    ui->card_bio = label(ui->card_panel, card_bio && card_bio[0] ? card_bio : "阿猛陪伴版",
                         font_cn(), C_MUTED);
    lv_obj_set_width(ui->card_bio, 120);
    lv_label_set_long_mode(ui->card_bio, LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(ui->card_bio, 106, 101);
    box(ui->card_panel, 17, 157, 206, 92, 0xF7F0E5, 10);
    ui->card_footer = label(ui->card_panel,
                            "双击确认键返回\n资料可在编译设置中修改",
                            font_cn(), C_MUTED);
    lv_obj_set_style_text_align(ui->card_footer, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui->card_footer, LV_ALIGN_BOTTOM_MID, 0, -26);

    /* Pet page with three distinct rooms. */
    create_header(ui);
    for (int i = 0; i < AMENG_ROOM_COUNT; ++i) {
        ui->scene_panels[i] = box(ui->pet_panel, 0, 36, 240, 206, C_BG, 0);
    }
    create_bedroom(ui->scene_panels[AMENG_ROOM_BEDROOM]);
    create_living(ui->scene_panels[AMENG_ROOM_LIVING]);
    create_study(ui->scene_panels[AMENG_ROOM_STUDY]);

    ui->cats[0] = cat_create(ui->pet_panel, 40);
    ui->cats[1] = cat_create(ui->pet_panel, 58);
    ui->cats[2] = cat_create(ui->pet_panel, 78);

    ui->bird = box(ui->pet_panel, 190, 164, 18, 11, 0x59473B, 3);
    box(ui->bird, -5, 4, 7, 3, 0xD6A23D, 1);
    ui->leg = box(ui->pet_panel, 173, 114, 33, 120, 0x6D7783, 9);
    hide(ui->bird, true);
    hide(ui->leg, true);

    ui->speech_panel = box(ui->pet_panel, 12, 45, 216, 60, 0xFFFDF8, 8);
    ui->speech = label(ui->speech_panel, "", font_cn(), C_INK);
    lv_obj_set_width(ui->speech, 192);
    lv_label_set_long_mode(ui->speech, LV_LABEL_LONG_WRAP);
    lv_obj_center(ui->speech);
    hide(ui->speech_panel, true);

    box(ui->pet_panel, 0, 242, 240, 78, C_DARK, 0);
    static const char *actions[AMENG_UI_ACTION_COUNT] = {
        "阿猛", "喂食", "抚摸", "游戏", "对话"
    };
    for (int i = 0; i < AMENG_UI_ACTION_COUNT; ++i) {
        ui->action_labels[i] = label(ui->pet_panel, actions[i], font_cn(), 0xBFB5AA);
        lv_obj_set_width(ui->action_labels[i], 47);
        lv_obj_set_style_text_align(ui->action_labels[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(ui->action_labels[i], 2 + i * 47, 255);
    }
    lv_obj_t *pet_hint = label(ui->pet_panel, "长上:状态  长下:换房  长确认:息屏",
                               font_cn(), 0xBFB5AA);
    lv_obj_set_width(pet_hint, 236);
    lv_obj_set_style_text_align(pet_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(pet_hint, 2, 294);
    create_status(ui);

    /* Settings page. */
    lv_obj_t *st = label(ui->settings_panel, "设置", font_cn(), C_INK);
    lv_obj_set_pos(st, 18, 20);
    static const char *setting_names[4] = {"亮度", "音量", "时间校准", "关于"};
    for (int i = 0; i < 4; ++i) {
        lv_obj_t *row = box(ui->settings_panel, 16, 62 + i * 48, 208, 40, C_PAPER, 7);
        ui->settings_items[i] = label(row, setting_names[i], font_cn(), C_INK);
        lv_obj_set_pos(ui->settings_items[i], 11, 10);
        ui->settings_values[i] = label(row, "", font_cn(), C_MUTED);
        lv_obj_align(ui->settings_values[i], LV_ALIGN_RIGHT_MID, -10, 0);
    }
    ui->settings_hint = label(ui->settings_panel,
                              "上下选择  确认修改  双击确认返回",
                              font_cn(), C_MUTED);
    lv_obj_align(ui->settings_hint, LV_ALIGN_BOTTOM_MID, 0, -23);

    ui->page = AMENG_PAGE_HOME;
    ui->selected_action = AMENG_UI_CALL;
    ui->room = AMENG_ROOM_LIVING;
    ameng_ui_set_home_selected(ui, 0);
    ameng_ui_set_action(ui, AMENG_UI_CALL);
    ameng_ui_set_settings_selected(ui, 0);
    ameng_ui_show_page(ui, AMENG_PAGE_HOME);
    lv_screen_load(ui->screen);
}

void ameng_ui_show_page(ameng_ui_t *ui, ameng_page_t page)
{
    if (!ui) return;
    ui->page = page;
    hide(ui->home_panel, page != AMENG_PAGE_HOME);
    hide(ui->card_panel, page != AMENG_PAGE_CARD);
    hide(ui->pet_panel, page != AMENG_PAGE_PET);
    hide(ui->settings_panel, page != AMENG_PAGE_SETTINGS);
}

void ameng_ui_set_home_selected(ameng_ui_t *ui, uint8_t selected)
{
    if (!ui) return;
    selected %= 3;
    ui->home_selected = selected;
    for (int i = 0; i < 3; ++i) {
        lv_obj_t *parent = lv_obj_get_parent(ui->home_items[i]);
        lv_obj_set_style_border_width(parent, i == selected ? 2 : 0, 0);
        lv_obj_set_style_border_color(parent, lv_color_hex(C_ACCENT), 0);
    }
}

void ameng_ui_set_action(ameng_ui_t *ui, ameng_ui_action_t action)
{
    if (!ui) return;
    ui->selected_action = action;
    for (int i = 0; i < AMENG_UI_ACTION_COUNT; ++i) {
        lv_obj_set_style_text_color(ui->action_labels[i],
            lv_color_hex(i == action ? 0xFFFFFF : 0xBFB5AA), 0);
        lv_obj_set_style_text_decor(ui->action_labels[i],
            i == action ? LV_TEXT_DECOR_UNDERLINE : LV_TEXT_DECOR_NONE, 0);
    }
}

void ameng_ui_set_settings_selected(ameng_ui_t *ui, uint8_t selected)
{
    if (!ui) return;
    selected %= 4;
    ui->settings_selected = selected;
    for (int i = 0; i < 4; ++i) {
        lv_obj_t *parent = lv_obj_get_parent(ui->settings_items[i]);
        lv_obj_set_style_border_width(parent, i == selected ? 2 : 0, 0);
        lv_obj_set_style_border_color(parent, lv_color_hex(C_ACCENT), 0);
    }
}

void ameng_ui_set_room(ameng_ui_t *ui, ameng_room_t room)
{
    if (!ui) return;
    ui->room = room;
    for (int i = 0; i < AMENG_ROOM_COUNT; ++i) {
        hide(ui->scene_panels[i], i != room);
    }
    lv_label_set_text(ui->room_label, ameng_room_name_cn(room));
}

void ameng_ui_set_dialogue(ameng_ui_t *ui, const char *text, bool visible)
{
    if (!ui) return;
    lv_label_set_text(ui->speech, text ? text : "");
    hide(ui->speech_panel, !visible);
}

void ameng_ui_set_status_visible(ameng_ui_t *ui, bool visible)
{
    if (!ui) return;
    ui->status_visible = visible;
    hide(ui->status_panel, !visible);
}

static const char *mood_word(uint8_t v)
{
    if (v >= 82) return "很好";
    if (v >= 62) return "不错";
    if (v >= 42) return "一般";
    if (v >= 22) return "低落";
    return "很差";
}

static const char *need_word(uint8_t v, const char *high)
{
    if (v >= 82) return high;
    if (v >= 58) return "有一点";
    if (v >= 30) return "正常";
    return "满足";
}

static const char *energy_word(uint8_t v)
{
    if (v >= 78) return "充足";
    if (v >= 50) return "正常";
    if (v >= 25) return "有点累";
    return "很困";
}

static const char *play_word(uint8_t v)
{
    if (v >= 75) return "很想玩";
    if (v >= 50) return "想玩";
    if (v >= 25) return "一般";
    return "不太想";
}

void ameng_ui_render_status(ameng_ui_t *ui, const ameng_state_t *s)
{
    if (!ui || !s) return;
    lv_label_set_text_fmt(ui->status_lines[0], "心情  %s", mood_word(s->mood));
    lv_label_set_text_fmt(ui->status_lines[1], "饥饿  %s", need_word(s->hunger, "很饿"));
    lv_label_set_text_fmt(ui->status_lines[2], "口渴  %s", need_word(s->thirst, "很渴"));
    lv_label_set_text_fmt(ui->status_lines[3], "精力  %s", energy_word(s->energy));
    lv_label_set_text_fmt(ui->status_lines[4], "玩心  %s", play_word(s->play_drive));
    lv_label_set_text_fmt(ui->status_lines[5], "好感  %u",
                          ameng_relationship_percent(s->affection_x10));
    lv_label_set_text_fmt(ui->status_lines[6], "信任  %u",
                          ameng_relationship_percent(s->trust_x10));
    lv_label_set_text_fmt(ui->status_lines[7], "相伴  %lu天",
                          (unsigned long)(s->days_together + 1));
}

static void reset_cat_pose(ameng_cat_ui_t *cat)
{
    if (!cat || !cat->group) return;
    lv_obj_set_style_translate_y(cat->body, 0, 0);
    lv_obj_set_style_translate_y(cat->head, 0, 0);
    lv_obj_set_style_translate_y(cat->paw_l, 0, 0);
    lv_obj_set_style_translate_y(cat->paw_r, 0, 0);
    lv_obj_set_size(cat->eye_l, sc(6,cat->w), sc(4,cat->w));
    lv_obj_set_size(cat->eye_r, sc(6,cat->w), sc(4,cat->w));
}

static ameng_cat_ui_t *show_depth(ameng_ui_t *ui, int depth)
{
    if (depth < 0) depth = 0;
    if (depth > 2) depth = 2;
    for (int i = 0; i < 3; ++i) hide(ui->cats[i].group, i != depth);
    return &ui->cats[depth];
}

static void place_cat(ameng_ui_t *ui, const ameng_state_t *s,
                      ameng_room_t player_room, ameng_behavior_t behavior)
{
    for (int i = 0; i < 3; ++i) {
        hide(ui->cats[i].group, true);
        reset_cat_pose(&ui->cats[i]);
    }
    hide(ui->bird, true);
    hide(ui->leg, true);

    if (!s || s->cat_room != player_room) return;

    int depth = s->cat_depth > 2 ? 2 : s->cat_depth;
    ameng_cat_ui_t *cat = show_depth(ui, depth);
    int max_x = 240 - cat->w - 6;
    int x = 4 + (int)s->cat_x * max_x / 100;
    int y = depth == 0 ? 181 : (depth == 1 ? 166 : 157);

    if (behavior == AMENG_BEHAVIOR_SUNBATHE && player_room == AMENG_ROOM_LIVING) {
        x = 27;
        y = depth == 0 ? 184 : 171;
        lv_obj_set_size(cat->eye_l, sc(6,cat->w), sc(2,cat->w));
        lv_obj_set_size(cat->eye_r, sc(6,cat->w), sc(2,cat->w));
    } else if (behavior == AMENG_BEHAVIOR_NAP) {
        y += 8;
        lv_obj_set_size(cat->eye_l, sc(6,cat->w), sc(2,cat->w));
        lv_obj_set_size(cat->eye_r, sc(6,cat->w), sc(2,cat->w));
    } else if (behavior == AMENG_BEHAVIOR_WAIT) {
        x = 12;
    }

    lv_obj_set_pos(cat->group, x, y - cat->h);
}

void ameng_ui_render_pet(ameng_ui_t *ui, const ameng_state_t *s,
                         ameng_room_t player_room, uint8_t hour, uint8_t minute,
                         int battery, bool ai_online)
{
    if (!ui || !s) return;
    ameng_ui_set_room(ui, player_room);
    lv_label_set_text_fmt(ui->time_label, "%02u:%02u", hour, minute);
    if (battery >= 0) lv_label_set_text_fmt(ui->battery_label, "%d%%", battery);
    else lv_label_set_text(ui->battery_label, "--%");

    if (ui->anim == AMENG_ANIM_NONE) {
        place_cat(ui, s, player_room,
                  ameng_state_behavior(s, 0, hour));
    }

    if (ui->status_visible) ameng_ui_render_status(ui, s);

    (void)ai_online;
}

void ameng_ui_render_settings(ameng_ui_t *ui, uint8_t brightness,
                              uint8_t volume, uint8_t hour)
{
    if (!ui) return;
    lv_label_set_text_fmt(ui->settings_values[0], "%u%%", brightness);
    lv_label_set_text_fmt(ui->settings_values[1], "%u%%", volume);
    lv_label_set_text_fmt(ui->settings_values[2], "%02u:00", hour);
    lv_label_set_text(ui->settings_values[3], "阿猛 V2");
}

void ameng_ui_start_animation(ameng_ui_t *ui, ameng_anim_t anim)
{
    if (!ui) return;
    ui->anim = anim;
    ui->anim_frame = 0;
}

void ameng_ui_tick(ameng_ui_t *ui, const ameng_state_t *s,
                   ameng_room_t player_room, ameng_behavior_t behavior)
{
    if (!ui || !s || ui->page != AMENG_PAGE_PET) return;

    if (ui->anim == AMENG_ANIM_NONE) {
        place_cat(ui, s, player_room, behavior);
        ameng_cat_ui_t *cat = NULL;
        if (s->cat_room == player_room) {
            int d = s->cat_depth > 2 ? 2 : s->cat_depth;
            cat = &ui->cats[d];
        }
        if (cat) {
            int dy = (ui->anim_frame & 1U) ? -1 : 0;
            lv_obj_set_style_translate_y(cat->body, dy, 0);
            lv_obj_set_style_translate_y(cat->head, dy, 0);
        }
        ui->anim_frame++;
        return;
    }

    ui->anim_frame++;
    for (int i = 0; i < 3; ++i) hide(ui->cats[i].group, true);
    hide(ui->bird, true);
    hide(ui->leg, true);

    if (ui->anim == AMENG_ANIM_ENTER) {
        int d = ui->anim_frame < 4 ? 0 : (ui->anim_frame < 8 ? 1 : 2);
        ameng_cat_ui_t *cat = show_depth(ui, d);
        int from_left = s->cat_x < 50;
        int target = d == 0 ? 104 : (d == 1 ? 87 : 73);
        int x;
        if (from_left) x = -cat->w + (int)ui->anim_frame * 18;
        else x = 240 - (int)ui->anim_frame * 17;
        if ((from_left && x > target) || (!from_left && x < target)) x = target;
        lv_obj_set_pos(cat->group, x, 174 - cat->h);
        if (ui->anim_frame >= 11) ui->anim = AMENG_ANIM_NONE;
    } else if (ui->anim == AMENG_ANIM_LEG_HUG) {
        ameng_cat_ui_t *cat = show_depth(ui, 2);
        hide(ui->leg, false);
        lv_obj_set_pos(cat->group, 86, 165 - cat->h);
        lv_obj_set_style_translate_y(cat->paw_l, -sc(16,cat->w), 0);
        lv_obj_set_style_translate_y(cat->paw_r, -sc(16,cat->w), 0);
        if (ui->anim_frame < 4) lv_obj_set_style_translate_y(cat->body, -4, 0);
        if (ui->anim_frame >= 13) ui->anim = AMENG_ANIM_NONE;
    } else if (ui->anim == AMENG_ANIM_POUNCE) {
        ameng_cat_ui_t *cat = show_depth(ui, 1);
        hide(ui->bird, false);
        int x;
        if (ui->anim_frame <= 3) {
            x = 38;
            lv_obj_set_style_translate_y(cat->body, 7, 0);
            lv_obj_set_style_translate_y(cat->head, 7, 0);
        } else if (ui->anim_frame <= 8) {
            x = 38 + (ui->anim_frame - 3) * 22;
        } else {
            x = 145;
        }
        lv_obj_set_pos(cat->group, x, 176 - cat->h);
        lv_obj_set_pos(ui->bird, ui->anim_frame < 8 ? 190 : 167, 165);
        if (ui->anim_frame >= 13) ui->anim = AMENG_ANIM_NONE;
    } else if (ui->anim == AMENG_ANIM_RUB) {
        ameng_cat_ui_t *cat = show_depth(ui, 2);
        int x = 76 + ((ui->anim_frame % 6) < 3 ? -5 : 5);
        lv_obj_set_pos(cat->group, x, 166 - cat->h);
        if (ui->anim_frame >= 10) ui->anim = AMENG_ANIM_NONE;
    }
}

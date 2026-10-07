#include "ameng_photo.h"

#include <stddef.h>
#include <string.h>

#define PHOTO_W 240
#define PHOTO_H 206
#define PHOTO_BYTES (PHOTO_W * PHOTO_H * 2)
#define C_DARK 0x252321
#define C_PAPER 0xFFF9EF
#define C_MUTED 0xBFB5AA

extern const uint8_t _binary_assets_photos_living_1_rgb565_start[] asm("_binary_assets_photos_living_1_rgb565_start");
extern const uint8_t _binary_assets_photos_living_2_rgb565_start[] asm("_binary_assets_photos_living_2_rgb565_start");
extern const uint8_t _binary_assets_photos_living_3_rgb565_start[] asm("_binary_assets_photos_living_3_rgb565_start");
extern const uint8_t _binary_assets_photos_workspace_1_rgb565_start[] asm("_binary_assets_photos_workspace_1_rgb565_start");
extern const uint8_t _binary_assets_photos_workspace_2_rgb565_start[] asm("_binary_assets_photos_workspace_2_rgb565_start");
extern const uint8_t _binary_assets_photos_workspace_3_rgb565_start[] asm("_binary_assets_photos_workspace_3_rgb565_start");
extern const uint8_t _binary_assets_photos_dining_1_rgb565_start[] asm("_binary_assets_photos_dining_1_rgb565_start");
extern const uint8_t _binary_assets_photos_dining_2_rgb565_start[] asm("_binary_assets_photos_dining_2_rgb565_start");
extern const uint8_t _binary_assets_photos_dining_3_rgb565_start[] asm("_binary_assets_photos_dining_3_rgb565_start");

static const uint8_t *const s_photo_data[AMENG_PHOTO_COUNT] = {
    _binary_assets_photos_living_1_rgb565_start,
    _binary_assets_photos_living_2_rgb565_start,
    _binary_assets_photos_living_3_rgb565_start,
    _binary_assets_photos_workspace_1_rgb565_start,
    _binary_assets_photos_workspace_2_rgb565_start,
    _binary_assets_photos_workspace_3_rgb565_start,
    _binary_assets_photos_dining_1_rgb565_start,
    _binary_assets_photos_dining_2_rgb565_start,
    _binary_assets_photos_dining_3_rgb565_start,
};

static const char *const s_photo_titles[AMENG_PHOTO_COUNT] = {
    "LIVING 1/3", "LIVING 2/3", "LIVING 3/3",
    "WORKSPACE 1/3", "WORKSPACE 2/3", "WORKSPACE 3/3",
    "DINING 1/3", "DINING 2/3", "DINING 3/3",
};

static lv_obj_t *s_panel;
static lv_obj_t *s_image;
static lv_obj_t *s_title;
static lv_image_dsc_t s_desc[AMENG_PHOTO_COUNT];
static uint8_t s_index;

static lv_obj_t *plain_box(lv_obj_t *parent, int x, int y, int w, int h,
                           uint32_t color)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_set_style_radius(o, 0, 0);
    return o;
}

static lv_obj_t *plain_label(lv_obj_t *parent, const char *text, uint32_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

static void init_descriptors(void)
{
    for (int i = 0; i < AMENG_PHOTO_COUNT; ++i) {
        memset(&s_desc[i], 0, sizeof(s_desc[i]));
        s_desc[i].header.magic = LV_IMAGE_HEADER_MAGIC;
        s_desc[i].header.cf = LV_COLOR_FORMAT_RGB565;
        s_desc[i].header.flags = 0;
        s_desc[i].header.w = PHOTO_W;
        s_desc[i].header.h = PHOTO_H;
        s_desc[i].header.stride = PHOTO_W * 2;
        s_desc[i].data_size = PHOTO_BYTES;
        s_desc[i].data = s_photo_data[i];
    }
}

void ameng_photo_create(lv_obj_t *screen)
{
    if (!screen || s_panel) return;

    init_descriptors();

    s_panel = plain_box(screen, 0, 0, 240, 320, C_DARK);

    plain_box(s_panel, 0, 0, 240, 36, C_DARK);
    lv_obj_t *brand = plain_label(s_panel, "PHOTOS", C_PAPER);
    lv_obj_set_pos(brand, 8, 10);
    lv_obj_set_size(brand, 56, 16);
    lv_obj_set_style_text_align(brand, LV_TEXT_ALIGN_LEFT, 0);

    s_title = plain_label(s_panel, s_photo_titles[0], C_PAPER);
    lv_obj_set_pos(s_title, 66, 10);
    lv_obj_set_size(s_title, 166, 16);
    lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_RIGHT, 0);

    s_image = lv_image_create(s_panel);
    lv_obj_set_pos(s_image, 0, 36);
    lv_image_set_src(s_image, &s_desc[0]);

    plain_box(s_panel, 0, 242, 240, 78, C_DARK);
    lv_obj_t *hint = plain_label(s_panel, "UP PREV   DOWN NEXT   OK BACK", C_PAPER);
    lv_obj_set_pos(hint, 2, 254);
    lv_obj_set_width(hint, 236);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *sub = plain_label(s_panel, "3 SPACES / 9 PHOTOS", C_MUTED);
    lv_obj_set_pos(sub, 2, 278);
    lv_obj_set_width(sub, 236);
    lv_obj_set_style_text_align(sub, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
}

void ameng_photo_show(uint8_t index)
{
    if (!s_panel) return;
    s_index = (uint8_t)(index % AMENG_PHOTO_COUNT);
    lv_image_set_src(s_image, &s_desc[s_index]);
    lv_label_set_text(s_title, s_photo_titles[s_index]);
    lv_obj_clear_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_panel);
}

void ameng_photo_hide(void)
{
    if (s_panel) lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
}

bool ameng_photo_visible(void)
{
    return s_panel && !lv_obj_has_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
}

uint8_t ameng_photo_index(void)
{
    return s_index;
}

uint8_t ameng_photo_move(int delta)
{
    int next = (int)s_index + delta;
    while (next < 0) next += AMENG_PHOTO_COUNT;
    while (next >= AMENG_PHOTO_COUNT) next -= AMENG_PHOTO_COUNT;
    ameng_photo_show((uint8_t)next);
    return s_index;
}

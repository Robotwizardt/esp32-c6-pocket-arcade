#pragma once
#include "lvgl.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static unsigned char framebuffer[480 * 480 * 3];
static unsigned char draw_buffer[480 * 40 * 2];
static lv_indev_t *pointer_device;
static lv_point_t pointer_position;
static bool pointer_pressed;
static void pointer_read(lv_indev_t *, lv_indev_data_t *data) {
    data->point = pointer_position;
    data->state = pointer_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels) {
    for(int y = area->y1; y <= area->y2; ++y) {
        for(int x = area->x1; x <= area->x2; ++x) {
            unsigned value = pixels[0] | (pixels[1] << 8);
            pixels += 2;
            if(x < 0 || y < 0 || x >= 480 || y >= 480) std::abort();
            auto p = framebuffer + (y * 480 + x) * 3;
            p[0] = ((value >> 11) & 31) * 255 / 31;
            p[1] = ((value >> 5) & 63) * 255 / 63;
            p[2] = (value & 31) * 255 / 31;
        }
    }
    lv_display_flush_ready(display);
}
static void settle() {
    for(int i = 0; i < 12; ++i) {
        lv_tick_inc(20);
        lv_timer_handler();
    }
    lv_refr_now(nullptr);
}
static void screenshot(const char *path) {
    settle();
    FILE *f = std::fopen(path, "wb");
    if(!f) std::abort();
    std::fprintf(f, "P6\n480 480\n255\n");
    std::fwrite(framebuffer, 1, sizeof(framebuffer), f);
    std::fclose(f);
}
static void tap(int x, int y) {
    pointer_position = {x, y};
    pointer_pressed = true;
    lv_indev_read(pointer_device);
    lv_tick_inc(40);
    pointer_pressed = false;
    lv_indev_read(pointer_device);
    settle();
}
static lv_obj_t *find_text(lv_obj_t *root, const char *text) {
    if(lv_obj_check_type(root, &lv_label_class) && std::strcmp(lv_label_get_text(root), text) == 0) return root;
    for(uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
        if(auto found = find_text(lv_obj_get_child(root, i), text)) return found;
    }
    return nullptr;
}
static void require(bool condition, const char *message) {
    if(!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(2); }
}
static void click_label(const char *text) {
    auto object = find_text(lv_screen_active(), text);
    require(object != nullptr, text);
    while(object && !lv_obj_has_flag(object, LV_OBJ_FLAG_CLICKABLE)) object = lv_obj_get_parent(object);
    require(object != nullptr, "clickable ancestor");
    require(!lv_obj_has_state(object, LV_STATE_DISABLED), "target must be enabled");
    lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
    settle();
}
static void dump_tree(lv_obj_t *root, int depth = 0) {
    lv_area_t a; lv_obj_get_coords(root, &a);
    const char *text = lv_obj_check_type(root, &lv_label_class) ? lv_label_get_text(root) : "";
    std::printf("%*s%d,%d..%d,%d %s\n", depth * 2, "", (int)a.x1, (int)a.y1, (int)a.x2, (int)a.y2, text);
    for(uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) dump_tree(lv_obj_get_child(root, i), depth + 1);
}
static void init_display() {
    lv_init();
    auto display = lv_display_create(480, 480);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, draw_buffer, nullptr, sizeof(draw_buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    pointer_device = lv_indev_create();
    lv_indev_set_type(pointer_device, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(pointer_device, pointer_read);
}

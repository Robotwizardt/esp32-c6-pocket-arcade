#include "game_lights_ui.h"

#include "fonts/lobby_fonts.h"

#include <cstdint>
#include <cstdio>

namespace gameLights {

namespace {

constexpr int32_t kScreen = 480;
constexpr int32_t kCell = 60;
constexpr int32_t kCellPitch = 72;
constexpr int32_t kFirstCellX = 66;
constexpr int32_t kFirstCellY = 114;

constexpr uint32_t kBackground = 0x070A12;
constexpr uint32_t kBoard = 0x0F1D2F;
constexpr uint32_t kOff = 0x1A2B42;
constexpr uint32_t kOffBorder = 0x2D4663;
constexpr uint32_t kOn = 0xF6C453;
constexpr uint32_t kOnBorder = 0xFFE49A;
constexpr uint32_t kText = 0xF8FAFC;
constexpr uint32_t kMuted = 0xA8B3C7;
constexpr uint32_t kSurface = 0x172A42;
constexpr uint32_t kPressed = 0x365474;

enum class DialogAction : intptr_t {
    Cancel = 0,
    Change,
    Next,
    Return,
};

Game g_game;
Exit g_exit = nullptr;
lv_obj_t *g_screen = nullptr;
lv_obj_t *g_board = nullptr;
lv_obj_t *g_step_label = nullptr;
lv_obj_t *g_level_label = nullptr;
lv_obj_t *g_cells[kCellCount]{};
lv_obj_t *g_dialog = nullptr;

static lv_color_t color(uint32_t hex)
{
    return lv_color_hex(hex);
}

static lv_obj_t *box(lv_obj_t *parent, int32_t x, int32_t y, int32_t width,
                     int32_t height, uint32_t fill, int32_t radius = 12)
{
    lv_obj_t *object = lv_obj_create(parent);
    lv_obj_remove_style_all(object);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, height);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(object, color(fill), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(object, radius, 0);
    return object;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                       uint32_t text_color = kText)
{
    lv_obj_t *object = lv_label_create(parent);
    lv_label_set_text(object, text);
    lv_obj_set_style_text_font(object, font, 0);
    lv_obj_set_style_text_color(object, color(text_color), 0);
    lv_obj_set_style_text_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(object, 0, 0);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_CLICKABLE);
    return object;
}

static lv_obj_t *button(lv_obj_t *parent, int32_t x, int32_t y, int32_t width,
                        int32_t height, const char *text, lv_event_cb_t callback,
                        void *user_data = nullptr, uint32_t fill = kSurface)
{
    lv_obj_t *object = box(parent, x, y, width, height, fill, 12);
    lv_obj_add_flag(object, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(object, color(kPressed), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(object, 1, 0);
    lv_obj_set_style_border_color(object, color(0x29405C), 0);
    lv_obj_set_style_border_opa(object, LV_OPA_COVER, 0);
    lv_obj_t *title = label(object, text, &lobby_zh_16);
    lv_obj_center(title);
    if (callback != nullptr) lv_obj_add_event_cb(object, callback, LV_EVENT_CLICKED, user_data);
    return object;
}

static void close_dialog()
{
    if (g_dialog != nullptr) {
        lv_obj_t *old = g_dialog;
        g_dialog = nullptr;
        lv_obj_delete_async(old);
    }
}

static void render()
{
    if (g_screen == nullptr) return;

    char text[32];
    std::snprintf(text, sizeof(text), "步数 %lu", static_cast<unsigned long>(g_game.state().steps));
    if (g_step_label != nullptr) lv_label_set_text(g_step_label, text);
    std::snprintf(text, sizeof(text), "第 %lu 关", static_cast<unsigned long>(g_game.state().level));
    if (g_level_label != nullptr) lv_label_set_text(g_level_label, text);

    for (uint8_t index = 0; index < kCellCount; ++index) {
        lv_obj_t *cell = g_cells[index];
        if (cell == nullptr) continue;
        const bool on = g_game.state().cells[index] != 0;
        lv_obj_set_style_bg_color(cell, color(on ? kOn : kOff), 0);
        lv_obj_set_style_border_color(cell, color(on ? kOnBorder : kOffBorder), 0);
        lv_obj_set_style_shadow_width(cell, on ? 14 : 0, 0);
        lv_obj_set_style_shadow_color(cell, color(kOn), 0);
        lv_obj_set_style_shadow_opa(cell, on ? LV_OPA_40 : LV_OPA_TRANSP, 0);
    }


}

static void dialog_action(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    const auto action = static_cast<DialogAction>(
        reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    close_dialog();

    switch (action) {
    case DialogAction::Cancel:
        break;
    case DialogAction::Change:
        g_game.change_level();
        render();
        break;
    case DialogAction::Next:
        if (g_game.next_level()) render();
        break;
    case DialogAction::Return:
        if (g_exit != nullptr) g_exit();
        break;
    }
}

static void show_dialog(const char *title_text, const char *body_text,
                        const char *left_text, DialogAction left_action,
                        const char *right_text, DialogAction right_action)
{
    if (g_screen == nullptr || g_dialog != nullptr) return;

    g_dialog = box(g_screen, 0, 0, kScreen, kScreen, 0x000000, 0);
    lv_obj_set_style_bg_opa(g_dialog, 210, 0);
    lv_obj_add_flag(g_dialog, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *panel = box(g_dialog, 38, 150, 404, 186, 0x15283D, 20);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, color(0x3B5875), 0);
    lv_obj_t *heading = label(panel, title_text, &lobby_zh_24);
    lv_obj_set_pos(heading, 24, 18);
    lv_obj_t *body = label(panel, body_text, &lobby_zh_16, kMuted);
    lv_obj_set_pos(body, 24, 57);
    lv_obj_set_width(body, 356);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);

    button(panel, 24, 122, 164, 44, left_text, dialog_action,
           reinterpret_cast<void *>(static_cast<intptr_t>(left_action)));
    button(panel, 216, 122, 164, 44, right_text, dialog_action,
           reinterpret_cast<void *>(static_cast<intptr_t>(right_action)), 0x1E4660);
}

static void show_victory()
{
    show_dialog("通关！", "所有灯都熄灭了。", "下一关", DialogAction::Next,
                "返回", DialogAction::Return);
}

static void show_change_confirmation()
{
    show_dialog("换一关？", "当前关卡进度会被清除。", "取消", DialogAction::Cancel,
                "确认换关", DialogAction::Change);
}

static void cell_clicked(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || g_dialog != nullptr) return;
    const uintptr_t index = reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
    if (index >= kCellCount) return;
    if (g_game.click(static_cast<uint8_t>(index))) {
        render();
        if (g_game.state().phase == Phase::Won) show_victory();
    }
}

static void reset_clicked(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || g_dialog != nullptr) return;
    g_game.reset();
    render();
}

static void change_clicked(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || g_dialog != nullptr) return;
    show_change_confirmation();
}

static void return_clicked(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED || g_exit == nullptr) return;
    g_exit();
}

static void screen_deleted(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_DELETE ||
        lv_event_get_target_obj(event) != g_screen) return;

    g_screen = nullptr;
    g_board = nullptr;
    g_step_label = nullptr;
    g_level_label = nullptr;
    g_dialog = nullptr;
    for (auto &cell : g_cells) cell = nullptr;
}

} // namespace

void configure(uint32_t seed)
{
    g_game = Game(seed);
}

const State &state()
{
    return g_game.state();
}

lv_obj_t *create_screen(Exit exit)
{
    g_exit = exit;
    g_screen = box(nullptr, 0, 0, kScreen, kScreen, kBackground, 0);
    lv_obj_add_flag(g_screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_screen, screen_deleted, LV_EVENT_DELETE, nullptr);

    lv_obj_t *title = label(g_screen, "灯泡全灭", &lobby_zh_24);
    lv_obj_set_pos(title, 128, 24);
    button(g_screen, 24, 18, 80, 44, "返回", return_clicked);
    button(g_screen, 252, 18, 80, 44, "重置", reset_clicked);
    button(g_screen, 344, 18, 112, 44, "换一关", change_clicked);

    g_level_label = label(g_screen, "第 1 关", &lobby_zh_16, kMuted);
    lv_obj_set_pos(g_level_label, 68, 70);
    g_step_label = label(g_screen, "步数 0", &lobby_zh_16, kMuted);
    lv_obj_set_pos(g_step_label, 350, 70);

    g_board = box(g_screen, 60, 108, 360, 360, kBoard, 20);
    for (uint8_t index = 0; index < kCellCount; ++index) {
        const int32_t column = index % kBoardSide;
        const int32_t row = index / kBoardSide;
        lv_obj_t *cell = box(g_board, kFirstCellX - 60 + column * kCellPitch,
                             kFirstCellY - 108 + row * kCellPitch,
                             kCell, kCell, kOff, 16);
        lv_obj_add_flag(cell, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_border_width(cell, 2, 0);
        lv_obj_set_style_border_color(cell, color(kOffBorder), 0);
        lv_obj_set_style_border_opa(cell, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(cell, color(kPressed), LV_STATE_PRESSED);
        lv_obj_set_style_pad_all(cell, 0, 0);
        lv_obj_add_event_cb(cell, cell_clicked, LV_EVENT_CLICKED,
                            reinterpret_cast<void *>(static_cast<uintptr_t>(index)));
        g_cells[index] = cell;
    }

    render();
    if (g_game.state().phase == Phase::Won) show_victory();
    return g_screen;
}

} // namespace gameLights

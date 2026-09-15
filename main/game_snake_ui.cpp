#include "game_snake_ui.h"

#include "fonts/lobby_fonts.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gameSnake {

namespace {

constexpr int32_t kScreen = 480;
constexpr int32_t kBoardX = 48;
constexpr int32_t kBoardY = 88;
constexpr int32_t kBoardPx = 384;
constexpr int32_t kCellPx = 24;
constexpr int32_t kSnakePx = 22;
constexpr int32_t kSwipeMin = 22;

constexpr uint32_t kBackground = 0x070A12;
constexpr uint32_t kBoardBackground = 0x102337;
constexpr uint32_t kGridA = 0x112A3D;
constexpr uint32_t kGridB = 0x0F2436;
constexpr uint32_t kSnake = 0x35D07F;
constexpr uint32_t kSnakeHead = 0x8AF0A8;
constexpr uint32_t kFood = 0xFFB45B;
constexpr uint32_t kText = 0xF8FAFC;
constexpr uint32_t kTextMuted = 0xA8B3C7;
constexpr uint32_t kSurface = 0x172A42;
constexpr uint32_t kSurfacePressed = 0x365474;
constexpr uint32_t kLine = 0x29405C;

enum class OverlayAction : intptr_t {
    Cancel = 0,
    Continue,
    ConfirmRestart,
    Restart,
    Return,
};

static Game g_game;
static Exit g_exit = nullptr;
static lv_obj_t *g_screen = nullptr;
static lv_obj_t *g_board = nullptr;
static lv_obj_t *g_score_label = nullptr;
static lv_obj_t *g_best_label = nullptr;
static lv_obj_t *g_hint_label = nullptr;
static lv_obj_t *g_pause_button = nullptr;
static lv_obj_t *g_restart_button = nullptr;
static lv_obj_t *g_overlay = nullptr;
static lv_timer_t *g_timer = nullptr;
static uint32_t g_last_tick = 0;
static lv_indev_t *g_input_indev = nullptr;
static lv_point_t g_press_point{};
static bool g_tracking = false;
static bool g_swipe_consumed = false;
static uint16_t g_best_score = 0;
static uint16_t g_rendered_score = 0xFFFF;
static Phase g_rendered_phase = Phase::Ready;

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

static void set_button_text(lv_obj_t *button, const char *text)
{
    if(button == nullptr) return;
    lv_obj_t *title = lv_obj_get_child(button, 0);
    if(title != nullptr) lv_label_set_text(title, text);
}

static lv_obj_t *button(lv_obj_t *parent, int32_t x, int32_t y, int32_t width,
                        int32_t height, const char *text, lv_event_cb_t callback,
                        void *user_data = nullptr, uint32_t fill = kSurface)
{
    lv_obj_t *object = box(parent, x, y, width, height, fill, 12);
    lv_obj_add_flag(object, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_EVENT_BUBBLE |
                             LV_OBJ_FLAG_GESTURE_BUBBLE));
    lv_obj_set_style_bg_color(object, color(kSurfacePressed), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(object, 1, 0);
    lv_obj_set_style_border_color(object, color(kLine), 0);
    lv_obj_set_style_border_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(object, 0, 0);
    lv_obj_t *title = label(object, text, &lobby_zh_16);
    lv_obj_center(title);
    if(callback != nullptr) lv_obj_add_event_cb(object, callback, LV_EVENT_CLICKED, user_data);
    return object;
}

static void draw_rect(lv_layer_t *layer, const lv_area_t &area, uint32_t fill,
                      int32_t radius, lv_opa_t opacity = LV_OPA_COVER)
{
    lv_draw_rect_dsc_t draw;
    lv_draw_rect_dsc_init(&draw);
    draw.base.layer = layer;
    draw.bg_color = color(fill);
    draw.bg_opa = opacity;
    draw.radius = radius;
    lv_draw_rect(layer, &draw, &area);
}

static void draw_board(lv_event_t *event)
{
    if(lv_event_get_code(event) != LV_EVENT_DRAW_MAIN || g_board == nullptr) return;
    lv_layer_t *layer = lv_event_get_layer(event);
    lv_area_t board_area;
    lv_obj_get_coords(g_board, &board_area);

    for(uint8_t y = 0; y < kBoardHeight; ++y) {
        for(uint8_t x = 0; x < kBoardWidth; ++x) {
            const int32_t left = board_area.x1 + static_cast<int32_t>(x) * kCellPx + 1;
            const int32_t top = board_area.y1 + static_cast<int32_t>(y) * kCellPx + 1;
            const lv_area_t cell{left, top, left + kCellPx - 3, top + kCellPx - 3};
            draw_rect(layer, cell, ((x + y) & 1) ? kGridA : kGridB, 5);
        }
    }

    const State &current = g_game.state();
    for(uint16_t index = current.length; index > 0; --index) {
        const Cell segment = current.body[index - 1];
        const int32_t left = board_area.x1 + static_cast<int32_t>(segment.x) * kCellPx + 1;
        const int32_t top = board_area.y1 + static_cast<int32_t>(segment.y) * kCellPx + 1;
        const lv_area_t area{left, top, left + kSnakePx - 1, top + kSnakePx - 1};
        draw_rect(layer, area, index == 1 ? kSnakeHead : kSnake, 7);
    }

    if(current.food.x < kBoardWidth && current.food.y < kBoardHeight) {
        const int32_t left = board_area.x1 + static_cast<int32_t>(current.food.x) * kCellPx + 5;
        const int32_t top = board_area.y1 + static_cast<int32_t>(current.food.y) * kCellPx + 5;
        const lv_area_t area{left, top, left + 13, top + 13};
        draw_rect(layer, area, kFood, LV_RADIUS_CIRCLE);
    }
}

static void update_hud(bool force = false)
{
    if(g_screen == nullptr) return;
    const State &current = g_game.state();
    if(current.score > g_best_score) g_best_score = current.score;

    if(force || g_rendered_score != current.score) {
        char text[32];
        std::snprintf(text, sizeof(text), "得分 %u", static_cast<unsigned>(current.score));
        if(g_score_label != nullptr) lv_label_set_text(g_score_label, text);
        std::snprintf(text, sizeof(text), "最高 %u", static_cast<unsigned>(g_best_score));
        if(g_best_label != nullptr) lv_label_set_text(g_best_label, text);
        g_rendered_score = current.score;
    }

    if(g_hint_label != nullptr && (force || g_rendered_phase != current.phase)) {
        const char *hint = "滑动开始";
        switch(current.phase) {
        case Phase::Running: hint = "全屏滑动转向"; break;
        case Phase::Paused: hint = "游戏已暂停"; break;
        case Phase::Dead: hint = "撞到障碍了"; break;
        case Phase::Won: hint = "棋盘已填满"; break;
        case Phase::Ready: break;
        }
        lv_label_set_text(g_hint_label, hint);
    }
    if(g_pause_button != nullptr) {
        set_button_text(g_pause_button, current.phase == Phase::Paused ? "继续" : "暂停");
    }
    g_rendered_phase = current.phase;
}

static void invalidate_board()
{
    if(g_board != nullptr) lv_obj_invalidate(g_board);
}

static void close_overlay()
{
    if(g_overlay != nullptr) {
        lv_obj_t *old = g_overlay;
        g_overlay = nullptr;
        g_last_tick = lv_tick_get();
        lv_obj_delete_async(old);
    }
}

static void show_overlay(const char *title_text, const char *body_text,
                         const char *left_text, OverlayAction left_action,
                         const char *right_text, OverlayAction right_action)
{
    if(g_screen == nullptr || g_overlay != nullptr) return;

    g_overlay = box(g_screen, 0, 0, kScreen, kScreen, 0x000000, 0);
    lv_obj_set_style_bg_opa(g_overlay, 205, 0);
    lv_obj_add_flag(g_overlay, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_GESTURE_BUBBLE));

    lv_obj_t *panel = box(g_overlay, 36, 142, 408, 210, 0x15283D, 22);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_border_color(panel, color(0x3B5875), 0);
    lv_obj_t *heading = label(panel, title_text, &lobby_zh_28);
    lv_obj_set_pos(heading, 24, 20);
    lv_obj_t *body = label(panel, body_text, &lobby_zh_16, kTextMuted);
    lv_obj_set_pos(body, 24, 65);
    lv_obj_set_width(body, 360);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);

    button(panel, 24, 136, 164, 48, left_text, [](lv_event_t *event) {
        if(lv_event_get_code(event) != LV_EVENT_CLICKED) return;
        const OverlayAction action = static_cast<OverlayAction>(
            reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
        close_overlay();
        if(action == OverlayAction::Continue) {
            g_game.resume();
        } else if(action == OverlayAction::ConfirmRestart || action == OverlayAction::Restart) {
            g_game.restart();
        } else if(action == OverlayAction::Return) {
            if(g_exit != nullptr) g_exit();
            return;
        }
        update_hud(true);
        invalidate_board();
    }, reinterpret_cast<void *>(static_cast<intptr_t>(left_action)));
    button(panel, 220, 136, 164, 48, right_text, [](lv_event_t *event) {
        if(lv_event_get_code(event) != LV_EVENT_CLICKED) return;
        const OverlayAction action = static_cast<OverlayAction>(
            reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
        close_overlay();
        if(action == OverlayAction::Continue) {
            g_game.resume();
        } else if(action == OverlayAction::ConfirmRestart || action == OverlayAction::Restart) {
            g_game.restart();
        } else if(action == OverlayAction::Return) {
            if(g_exit != nullptr) g_exit();
            return;
        }
        update_hud(true);
        invalidate_board();
    }, reinterpret_cast<void *>(static_cast<intptr_t>(right_action)), 0x1E4660);
}

static void state_overlay()
{
    switch(g_game.phase()) {
    case Phase::Paused:
        show_overlay("暂停游戏", "准备好后点击继续，滑动不会解除暂停。", "返回", OverlayAction::Return,
                     "继续", OverlayAction::Continue);
        break;
    case Phase::Dead:
        show_overlay("游戏结束", "小蛇撞到了边界或自己的身体。", "返回", OverlayAction::Return,
                     "重新开始", OverlayAction::Restart);
        break;
    case Phase::Won:
        show_overlay("通关！", "你填满了整张棋盘。", "返回", OverlayAction::Return,
                     "重新开始", OverlayAction::Restart);
        break;
    case Phase::Ready:
    case Phase::Running:
        break;
    }
}

static void pause_clicked(lv_event_t *event)
{
    if(lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    if(g_game.phase() == Phase::Running) {
        g_game.pause();
        update_hud(true);
        state_overlay();
    } else if(g_game.phase() == Phase::Paused) {
        g_game.resume();
        close_overlay();
        update_hud(true);
    }
}

static void restart_clicked(lv_event_t *event)
{
    if(lv_event_get_code(event) != LV_EVENT_CLICKED || g_overlay != nullptr) return;
    show_overlay("确认重开？", "本局进度会清空，但最高分会保留。", "取消", OverlayAction::Cancel,
                 "确认重开", OverlayAction::ConfirmRestart);
}

static void return_clicked(lv_event_t *event)
{
    if(lv_event_get_code(event) != LV_EVENT_CLICKED || g_exit == nullptr) return;
    g_exit();
}

static Direction direction_from_delta(int32_t dx, int32_t dy)
{
    if(std::abs(dx) >= std::abs(dy)) return dx > 0 ? Direction::Right : Direction::Left;
    return dy > 0 ? Direction::Down : Direction::Up;
}

static void input_event(lv_event_t *event)
{
    if(g_screen == nullptr) return;
    lv_indev_t *indev = static_cast<lv_indev_t *>(lv_event_get_current_target(event));
    if(indev == nullptr) indev = g_input_indev;
    if(indev == nullptr) return;

    const lv_event_code_t code = lv_event_get_code(event);
    if(code == LV_EVENT_PRESSED) {
        g_tracking = true;
        g_swipe_consumed = false;
        lv_indev_get_point(indev, &g_press_point);
        return;
    }
    if(code == LV_EVENT_RELEASED && g_tracking) {
        lv_point_t end{};
        lv_indev_get_point(indev, &end);
        const int32_t dx = end.x - g_press_point.x;
        const int32_t dy = end.y - g_press_point.y;
        g_tracking = false;
        if(std::max(std::abs(dx), std::abs(dy)) >= kSwipeMin) {
            g_swipe_consumed = true;
            /* A swipe over a button is still a game gesture, never a click. */
            if(g_overlay == nullptr &&
               (g_game.phase() == Phase::Ready || g_game.phase() == Phase::Running)) {
                const bool was_ready = g_game.phase() == Phase::Ready;
                g_game.set_direction(direction_from_delta(dx, dy));
                if(was_ready) g_last_tick = lv_tick_get();
                update_hud(true);
                invalidate_board();
            }
            // Deliver RELEASED so the pressed visual state is cleared.
            // Suppress CLICKED below when this release belongs to a swipe.
        }
        return;
    }
    if(code == LV_EVENT_CLICKED && g_swipe_consumed) {
        g_swipe_consumed = false;
        lv_indev_stop_processing(indev);
    }
}

static void timer_tick(lv_timer_t *)
{
    if(g_screen == nullptr) return;
    const Phase before = g_game.phase();
    const uint32_t now = lv_tick_get();
    const uint32_t elapsed = now - g_last_tick;
    g_last_tick = now;
    if(g_overlay != nullptr) return;
    const StepResult result = g_game.advance(elapsed);
    const Phase after = g_game.phase();
    if(result != StepResult::None || before != after) {
        update_hud(true);
        invalidate_board();
    }
    if(before != after && (after == Phase::Paused || after == Phase::Dead || after == Phase::Won)) {
        state_overlay();
    }
}

static void screen_deleted(lv_event_t *event)
{
    if(lv_event_get_code(event) != LV_EVENT_DELETE ||
       lv_event_get_target_obj(event) != g_screen) return;

    g_screen = nullptr;
    g_board = nullptr;
    g_score_label = nullptr;
    g_best_label = nullptr;
    g_hint_label = nullptr;
    g_pause_button = nullptr;
    g_restart_button = nullptr;
    g_overlay = nullptr;
    if(g_timer != nullptr) {
        lv_timer_delete(g_timer);
        g_timer = nullptr;
    }
    if(g_input_indev != nullptr) {
        lv_indev_remove_event_cb_with_user_data(g_input_indev, input_event, nullptr);
        g_input_indev = nullptr;
    }
}

} // namespace

void configure(uint32_t seed)
{
    g_game = Game(seed);
    g_rendered_score = 0xFFFF;
    g_rendered_phase = Phase::Ready;
}

const Game &game()
{
    return g_game;
}

const State &state()
{
    return g_game.state();
}

lv_obj_t *create_screen(Exit exit)
{
    g_exit = exit;
    g_game.restart();
    g_tracking = false;
    g_swipe_consumed = false;
    g_screen = box(nullptr, 0, 0, kScreen, kScreen, kBackground, 0);
    lv_obj_add_flag(g_screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_screen, screen_deleted, LV_EVENT_DELETE, nullptr);

    lv_obj_t *title = label(g_screen, "贪吃蛇", &lobby_zh_24);
    lv_obj_set_pos(title, 142, 18);

    button(g_screen, 24, 18, 92, 44, "返回", return_clicked);
    g_pause_button = button(g_screen, 264, 18, 88, 44, "暂停", pause_clicked);
    g_restart_button = button(g_screen, 364, 18, 92, 44, "重开", restart_clicked);

    g_score_label = label(g_screen, "得分 0", &lobby_zh_16);
    lv_obj_set_pos(g_score_label, 48, 65);
    g_best_label = label(g_screen, "最高 0", &lobby_zh_16, 0xFBBF24);
    lv_obj_set_pos(g_best_label, 170, 65);
    g_hint_label = label(g_screen, "滑动开始", &lobby_zh_16, kTextMuted);
    lv_obj_set_pos(g_hint_label, 324, 65);

    g_board = box(g_screen, kBoardX, kBoardY, kBoardPx, kBoardPx, kBoardBackground, 24);
    lv_obj_add_event_cb(g_board, draw_board, LV_EVENT_DRAW_MAIN, nullptr);
    lv_obj_set_style_border_width(g_board, 1, 0);
    lv_obj_set_style_border_color(g_board, color(0x27445B), 0);
    lv_obj_set_style_border_opa(g_board, LV_OPA_COVER, 0);

    g_rendered_score = 0xFFFF;
    g_rendered_phase = Phase::Ready;
    update_hud(true);

    g_last_tick = lv_tick_get();
    g_timer = lv_timer_create(timer_tick, 20, nullptr);
    if(g_input_indev != nullptr) {
        lv_indev_remove_event_cb_with_user_data(g_input_indev, input_event, nullptr);
        g_input_indev = nullptr;
    }
    g_input_indev = lv_indev_get_next(nullptr);
    while(g_input_indev && lv_indev_get_type(g_input_indev) != LV_INDEV_TYPE_POINTER)
        g_input_indev = lv_indev_get_next(g_input_indev);
    if(g_input_indev != nullptr) {
        lv_indev_add_event_cb(g_input_indev, input_event, LV_EVENT_ALL, nullptr);
    }
    invalidate_board();
    return g_screen;
}

} // namespace gameSnake

#include "game_2048_ui.h"
#include "fonts/lobby_fonts.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace game2048 {
static Game game;
static Save save_fn;
static Exit exit_fn;
static lv_obj_t *screen, *overlay, *score_label, *best_label, *undo_button;
static lv_obj_t *tiles[16], *numbers[16];
static lv_timer_t *save_timer;
static bool dirty, save_failed, tracking;
static lv_point_t press_point;

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h, uint32_t fill) {
    auto object = lv_obj_create(parent);
    lv_obj_remove_style_all(object);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, w, h);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(object, lv_color_hex(fill), 0);
    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(object, 12, 0);
    return object;
}
static lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color = 0xF8FAFC) {
    auto object = lv_label_create(parent);
    lv_label_set_text(object, text);
    lv_obj_set_style_text_font(object, font, 0);
    lv_obj_set_style_text_color(object, lv_color_hex(color), 0);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_CLICKABLE);
    return object;
}
static lv_obj_t *button(lv_obj_t *parent, int x, int y, int w, const char *text, lv_event_cb_t cb) {
    auto object = box(parent, x, y, w, 48, 0x172A42);
    lv_obj_add_flag(object, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(object, lv_color_hex(0x365474), LV_STATE_PRESSED);
    auto title = label(object, text, &lobby_zh_20);
    lv_obj_center(title);
    lv_obj_add_event_cb(object, cb, LV_EVENT_CLICKED, nullptr);
    return object;
}
static void update_scores() {
    if(!screen || !score_label || !best_label) return;
    char text[48];
    std::snprintf(text, sizeof(text), "得分 %lu", static_cast<unsigned long>(game.archive().current.score));
    lv_label_set_text(score_label, text);
    if(save_failed) std::snprintf(text, sizeof(text), "存档失败");
    else std::snprintf(text, sizeof(text), "最高 %lu", static_cast<unsigned long>(game.archive().best));
    lv_label_set_text(best_label, text);
    lv_obj_set_style_text_color(best_label, lv_color_hex(save_failed ? 0xFB7185 : 0xFBBF24), 0);
}
static void flush_save() {
    if(!dirty) return;
    if(save_fn && save_fn(game.archive())) {
        save_failed = false;
        dirty = false;
    } else {
        // Keep the in-memory game intact when NVS is unavailable or a write
        // fails. The next debounce or an explicit exit will retry the save.
        save_failed = true;
    }
    if(save_timer) lv_timer_pause(save_timer);
    if(screen) update_scores();
}
static void changed() {
    dirty = true;
    if(save_timer) {
        lv_timer_reset(save_timer);
        lv_timer_resume(save_timer);
    }
}
static uint32_t tile_color(uint32_t value) {
    switch(value) {
    case 0: return 0x243044;
    case 2: return 0xE6E0D4;
    case 4: return 0xEBD2A1;
    case 8: return 0xED9D53;
    case 16: return 0xF07842;
    case 32: return 0xE8544D;
    case 64: return 0xC83755;
    case 128: return 0xB7791F;
    case 256: return 0x986914;
    case 512: return 0x775412;
    case 1024: return 0x875BC1;
    default: return 0x60419A;
    }
}
static void close_overlay() {
    if(overlay) lv_obj_delete_async(overlay);
    overlay = nullptr;
}
static void render();
static void state_dialog();
enum class Action { Cancel, Restart, Continue, Undo };
static void action(lv_event_t *event) {
    auto value = static_cast<Action>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    close_overlay();
    if(value == Action::Cancel) {
        // Cancel is also used for the game-over "查看棋盘" action. Do not
        // immediately call state_dialog(), or that dialog would reopen.
        render();
        return;
    }
    if(value == Action::Restart) { game.restart(); changed(); }
    if(value == Action::Continue) { game.continue_playing(); changed(); }
    if(value == Action::Undo && game.undo()) changed();
    render();
    state_dialog();
}
static void dialog(const char *title, const char *body, const char *left, Action left_action,
                   const char *right, Action right_action) {
    if(overlay) return;
    overlay = box(screen, 0, 0, 480, 480, 0x000000);
    lv_obj_set_style_bg_opa(overlay, 200, 0);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    auto panel = box(overlay, 40, 124, 400, 232, 0x152238);
    auto heading = label(panel, title, &lobby_zh_28);
    lv_obj_set_pos(heading, 24, 20);
    auto description = label(panel, body, &lobby_zh_20, 0xCBD5E1);
    lv_obj_set_pos(description, 24, 68);
    lv_obj_set_width(description, 352);
    auto a = button(panel, 24, 160, 164, left, action);
    lv_obj_remove_event_cb(a, action);
    lv_obj_add_event_cb(a, action, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(left_action)));
    auto b = button(panel, 212, 160, 164, right, action);
    lv_obj_remove_event_cb(b, action);
    lv_obj_add_event_cb(b, action, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<intptr_t>(right_action)));
    lv_obj_set_style_bg_color(b, lv_color_hex(0x315C83), 0);
}
static void state_dialog() {
    if(game.win_pending()) dialog("达成 2048！", "继续挑战更大的数字吧。", "重新开始", Action::Restart, "继续挑战", Action::Continue);
    else if(!game.can_move()) {
        if(game.archive().undo_available) dialog("游戏结束", "没有可以移动的数字了。", "撤销一步", Action::Undo, "重新开始", Action::Restart);
        else dialog("游戏结束", "没有可以移动的数字了。", "查看棋盘", Action::Cancel, "重新开始", Action::Restart);
    }
}
static void render() {
    if(!screen || !score_label || !best_label || !undo_button) return;
    update_scores();
    for(unsigned i = 0; i < 16; ++i) {
        if(!tiles[i] || !numbers[i]) continue;
        uint32_t value = game.archive().current.cells[i];
        char text[16] = {};
        if(value) std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(value));
        lv_label_set_text(numbers[i], text);
        lv_obj_set_style_bg_color(tiles[i], lv_color_hex(tile_color(value)), 0);
        lv_obj_set_style_text_color(numbers[i], lv_color_hex(value <= 8 ? 0x302C29 : 0xFFFFFF), 0);
        const lv_font_t *number_font = value >= 1000000
                                           ? &lv_font_montserrat_12
                                           : (value >= 100000 ? &lobby_zh_16
                                                              : (value >= 10000 ? &lobby_zh_20
                                                                                : &lobby_zh_28));
        lv_obj_set_style_text_font(numbers[i], number_font, 0);
        lv_obj_center(numbers[i]);
    }
    bool enabled = game.archive().undo_available != 0;
    if(enabled) lv_obj_remove_state(undo_button, LV_STATE_DISABLED);
    else lv_obj_add_state(undo_button, LV_STATE_DISABLED);
    auto undo_label = lv_obj_get_child(undo_button, 0);
    if(undo_label) lv_obj_set_style_text_color(undo_label, lv_color_hex(enabled ? 0xF8FAFC : 0x64748B), 0);
}
static void board_event(lv_event_t *event) {
    auto input = lv_indev_active();
    if(!input) return;
    auto code = lv_event_get_code(event);
    if(code == LV_EVENT_PRESSED) {
        tracking = !overlay;
        lv_indev_get_point(input, &press_point);
    } else if(code == LV_EVENT_PRESS_LOST) tracking = false;
    else if(code == LV_EVENT_RELEASED && tracking) {
        tracking = false;
        if(overlay) return;
        lv_point_t end;
        lv_indev_get_point(input, &end);
        int dx = end.x - press_point.x, dy = end.y - press_point.y;
        if(std::max(std::abs(dx), std::abs(dy)) < 22) return;
        auto direction = std::abs(dx) >= std::abs(dy) ? (dx > 0 ? Direction::Right : Direction::Left)
                                                     : (dy > 0 ? Direction::Down : Direction::Up);
        if(game.move(direction)) { changed(); render(); }
        state_dialog();
    }
}
void configure(uint32_t seed, Load load, Save save) {
    game = Game(seed);
    save_fn = save;
    Archive archive;
    dirty = !(load && load(archive) && game.restore(archive));
    save_failed = false;
}
lv_obj_t *create_screen(Exit exit) {
    exit_fn = exit;
    screen = box(nullptr, 0, 0, 480, 480, 0x070A12);
    lv_obj_set_style_radius(screen, 0, 0);
    overlay = nullptr;
    tracking = false;
    auto title = label(screen, "2048", &lobby_zh_24);
    lv_obj_set_pos(title, 124, 27);
    button(screen, 24, 20, 80, "返回", [](lv_event_t *) { flush_save(); if(exit_fn) exit_fn(); });
    undo_button = button(screen, 264, 20, 88, "撤销", [](lv_event_t *) {
        if(game.undo()) { changed(); render(); state_dialog(); }
    });
    button(screen, 368, 20, 88, "重开", [](lv_event_t *) {
        dialog("重新开始？", "本局进度将被清空。", "取消", Action::Cancel, "确认重开", Action::Restart);
    });
    score_label = label(screen, "", &lobby_zh_16);
    lv_obj_set_pos(score_label, 64, 78);
    best_label = label(screen, "", &lobby_zh_16);
    lv_obj_set_pos(best_label, 250, 78);
    auto board = box(screen, 64, 108, 352, 352, 0x142033);
    lv_obj_add_flag(board, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(board, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_add_event_cb(board, board_event, LV_EVENT_ALL, nullptr);
    for(unsigned i = 0; i < 16; ++i) {
        tiles[i] = box(board, 12 + (i % 4) * 84, 12 + (i / 4) * 84, 76, 76, 0x243044);
        numbers[i] = label(tiles[i], "", &lobby_zh_28);
    }
    save_timer = lv_timer_create([](lv_timer_t *) { flush_save(); }, 900, nullptr);
    lv_timer_pause(save_timer);
    lv_obj_add_event_cb(screen, [](lv_event_t *event) {
        if(lv_event_get_target_obj(event) != screen) return;
        // LVGL may dispatch DELETE after child labels have already gone away.
        // Clear all UI handles first so flush_save() cannot update dead
        // objects, while still writing the latest archive to NVS.
        screen = nullptr;
        overlay = nullptr;
        score_label = nullptr;
        best_label = nullptr;
        undo_button = nullptr;
        for(auto &tile : tiles) tile = nullptr;
        for(auto &number : numbers) number = nullptr;
        flush_save();
        if(save_timer) lv_timer_delete(save_timer);
        save_timer = nullptr;
    }, LV_EVENT_DELETE, nullptr);
    render();
    state_dialog();
    if(dirty) changed();
    return screen;
}
}

#include "capture.h"
#include "../../main/game_2048_ui.h"
#include <algorithm>
static game2048::Archive saved;
static bool exited;
static unsigned saves;
static bool load_game(game2048::Archive &value) { value = saved; return true; }
static bool save_game(const game2048::Archive &value) { saved = value; ++saves; return true; }
static void exit_game() { exited = true; }
static void advance_save() { lv_tick_inc(1000); lv_timer_handler(); settle(); }
static void show() {
    game2048::configure(123, load_game, save_game);
    exited = false;
    auto previous = lv_screen_active();
    auto next = game2048::create_screen(exit_game);
    lv_screen_load(next);
    lv_obj_delete(previous);
    settle();
}
static void leave() {
    click_label("返回");
    require(exited, "return callback");
    auto previous = lv_screen_active();
    auto blank = lv_obj_create(nullptr);
    lv_screen_load(blank);
    lv_obj_delete(previous);
    settle();
}
static void swipe(int x1, int y1, int x2, int y2) {
    pointer_position = {x1,y1}; pointer_pressed = true;
    lv_indev_read(pointer_device);
    lv_tick_inc(40);
    pointer_position = {x2,y2};
    lv_indev_read(pointer_device);
    pointer_pressed = false;
    lv_indev_read(pointer_device);
    settle();
}
static void fixture(std::array<uint32_t,16> cells) {
    saved = {};
    saved.current.cells = cells;
    saved.current.random = 123;
}
int main() {
    init_display();
    fixture({2,2});
    show();
    screenshot("2048-start.ppm");
    tap(200,150);
    advance_save();
    require(saved.current.cells[0] == 2 && saved.current.cells[1] == 2 && saves == 0, "tap is not a swipe");
    swipe(300,150,100,150);
    advance_save();
    screenshot("2048-play.ppm");
    require(saved.current.cells[0] == 4 && saved.current.score == 4 && saved.undo_available, "swipe merges and saves");
    click_label("撤销");
    advance_save();
    require(saved.current.cells[0] == 2 && saved.current.cells[1] == 2 && saved.current.score == 0 && saved.best == 4,
            "undo via button preserves high score");
    auto current = saved.current.cells;
    click_label("重开");
    screenshot("2048-restart.ppm");
    click_label("取消");
    require(!find_text(lv_screen_active(), "重新开始？"), "cancel closes confirmation");
    require(saved.current.cells == current, "cancel preserves game");
    click_label("重开"); click_label("确认重开"); advance_save();
    unsigned count = 0; for(auto value:saved.current.cells) if(value)++count;
    require(count==2 && saved.current.score==0 && !saved.undo_available, "confirmed restart");
    leave();
    auto resumed = saved.current.cells;
    show(); advance_save();
    require(saved.current.cells == resumed, "saved game loads on reentry");
    leave();
    fixture({1024,1024}); show();
    swipe(300,150,100,150);
    require(find_text(lv_screen_active(), "达成 2048！"), "victory dialog");
    screenshot("2048-win.ppm");
    click_label("继续挑战"); advance_save();
    require(saved.current.continued == 1 && !find_text(lv_screen_active(), "达成 2048！"), "continue victory");
    leave();
    fixture({2,4,2,4,4,2,4,2,2,4,2,4,4,2,4,2}); show();
    require(find_text(lv_screen_active(), "游戏结束"), "game over dialog");
    screenshot("2048-over.ppm");
    click_label("查看棋盘");
    require(!find_text(lv_screen_active(), "游戏结束"), "view board closes terminal dialog");
    leave();
    fixture({2,2});
    for(unsigned i = 0; i < 5; ++i) { show(); swipe(300,150,100,150); click_label("撤销"); leave(); }
    lv_mem_monitor_t before, after;
    lv_mem_monitor(&before);
    for(unsigned i = 0; i < 100; ++i) { show(); swipe(300,150,100,150); click_label("撤销"); leave(); }
    lv_mem_monitor(&after);
    std::printf("HEAP before=%zu after=%zu max_used=%zu\n",before.free_size,after.free_size,after.max_used);
    require(after.free_size >= before.free_size, "reentry must not leak objects or timers");
    std::puts("PASS: 2048 swipe, undo, restart, save/resume, victory/continue, game-over, 100 reentries");
}

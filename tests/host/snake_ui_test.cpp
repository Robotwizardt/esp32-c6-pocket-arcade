#include "capture.h"
#include "../../main/game_snake_ui.h"
using namespace gameSnake;
static bool exited;
static void exit_game() { exited=true; }
static void show() {
    exited=false; auto old=lv_screen_active();
    lv_screen_load(create_screen(exit_game));lv_obj_delete(old);settle();
}
static void leave() {
    click_label("返回");require(exited,"return callback");
    auto old=lv_screen_active();lv_screen_load(lv_obj_create(nullptr));lv_obj_delete(old);settle();
}
static void swipe(int x,int y,int dx,int dy) {
    pointer_position={x,y};pointer_pressed=true;lv_indev_read(pointer_device);
    lv_tick_inc(20);pointer_position={x+dx,y+dy};lv_indev_read(pointer_device);
    pointer_pressed=false;lv_indev_read(pointer_device);lv_refr_now(nullptr);
}
int main() {
    init_display();configure(123);show();
    require(state().phase==Phase::Ready,"ready waits for first swipe");
    screenshot("snake-ready.ppm");
    tap(240,240);require(state().phase==Phase::Ready,"tap does not start");
    swipe(240,240,-60,0);require(state().phase==Phase::Ready,"reverse cannot start");
    swipe(240,76,60,0);require(state().phase==Phase::Running,"HUD swipe starts game");
    lv_tick_inc(300);lv_timer_handler();
    require(state().steps>=1,"timer moves snake");
    tap(307,40);require(state().phase==Phase::Paused,"pause button");
    auto steps=state().steps;lv_tick_inc(5000);lv_timer_handler();
    require(state().steps==steps,"paused timer does not move");
    screenshot("snake-paused.ppm");
    swipe(340,300,30,0);require(state().phase==Phase::Paused,"modal swipe does not resume");
    tap(338,303);require(state().phase==Phase::Running,"resume button");
    // Swiping a return button is a game gesture, not a navigation click.
    swipe(60,44,30,0);require(!exited,"swipe within return must not click");
    auto return_label=find_text(lv_screen_active(),"返回");
    require(!lv_obj_has_state(lv_obj_get_parent(return_label),LV_STATE_PRESSED),"swipe releases button visual state");
    click_label("重开");steps=state().steps;lv_tick_inc(2000);lv_timer_handler();require(state().steps==steps,"restart dialog suspends movement");
    screenshot("snake-restart.ppm");
    click_label("取消");
    click_label("重开");click_label("确认重开");
    require(state().phase==Phase::Ready && state().score==0,"confirmed restart");
    swipe(240,240,60,0);
    for(int i=0;i<12 && state().phase==Phase::Running;++i) {lv_tick_inc(300);lv_timer_handler();}
    require(state().phase==Phase::Dead,"wall death");screenshot("snake-dead.ppm");
    leave();
    for(int i=0;i<5;++i) {show();leave();}
    lv_mem_monitor_t before,after;lv_mem_monitor(&before);
    for(int i=0;i<100;++i) {show();swipe(240,240,0,-60);leave();}
    lv_tick_inc(5000);lv_timer_handler();lv_mem_monitor(&after);
    require(before.used_cnt==after.used_cnt && after.free_size+512>=before.free_size,"no retained screen or timer allocations");
    std::printf("HEAP before=%zu after=%zu\n",before.free_size,after.free_size);
    std::puts("PASS: snake full-screen gestures, buttons, pause, restart, death, 100 reentries");
}

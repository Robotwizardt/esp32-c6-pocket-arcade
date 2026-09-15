#include "game_ui_test_common.h"
#include "../../main/game_breakout_ui.h"
using namespace gameBreakout;
int main(){init_display();test_enter(create_screen);require(state().phase==Phase::Ready,"ready");screenshot("breakout-ready.ppm");tap(240,300);require(state().phase==Phase::Running,"tap launches");
 pointer_position={180,420};pointer_pressed=true;lv_indev_read(pointer_device);pointer_position={340,420};lv_indev_read(pointer_device);pointer_pressed=false;lv_indev_read(pointer_device);require(state().paddle>260,"drag paddle");
 tap(300,40);require(state().phase==Phase::Paused,"pause");auto x=state().x;lv_tick_inc(4000);lv_timer_handler();require(state().x==x,"pause freezes");tap(340,300);require(state().phase==Phase::Running,"continue");
 auto&s=const_cast<State&>(state());s.bricks.fill(false);s.bricks[0]=true;s.score=23;s.x=34;s.y=21;s.vx=0;s.vy=150;lv_tick_inc(30);lv_timer_handler();require(s.phase==Phase::Won,"win modal");screenshot("breakout-won.ppm");click_label("再来一局");tap(240,300);s.lives=1;s.y=333;s.vy=150;lv_tick_inc(30);lv_timer_handler();require(s.phase==Phase::Dead,"loss modal");click_label("再来一局");click_label("重开");click_label("取消");click_label("重开");click_label("确认重开");require(s.phase==Phase::Ready&&s.lives==3,"restart");test_leave();test_lifecycle(create_screen);std::puts("PASS: breakout touch launch/drag, pause, win/loss, replay, restart, 100 reentries");}

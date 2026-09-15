#include "capture.h"
#include "../../main/game_stack_ui.h"
using namespace gameStack;
static bool exited;
static void exit_game(){exited=true;}
static void show(){exited=false;auto old=lv_screen_active();lv_screen_load(create_screen(exit_game));lv_obj_delete(old);settle();}
static void leave(){click_label("返回");require(exited,"exit");auto old=lv_screen_active();lv_screen_load(lv_obj_create(nullptr));lv_obj_delete(old);settle();}
int main(){
 init_display();show();screenshot("stack-ready.ppm");require(state().phase==Phase::Ready,"ready");
 tap(230,220);require(state().phase==Phase::Running,"tap starts");
 auto&s=const_cast<State&>(state());s.moving.x=s.tower[s.count-1].x;s.velocity=0;
 tap(230,220);require(s.score==1,"tap stacks");screenshot("stack-play.ppm");
    for(int i=0;i<7;++i){s.moving.x=s.tower[s.count-1].x;s.velocity=0;tap(230,220);}
    screenshot("stack-tall.ppm");
 tap(300,40);require(s.phase==Phase::Paused,"pause");float x=s.moving.x;lv_tick_inc(4000);lv_timer_handler();require(s.moving.x==x,"paused motion");
 tap(340,285);require(s.phase==Phase::Running,"resume");
 click_label("重开");x=s.moving.x;lv_tick_inc(4000);lv_timer_handler();require(s.moving.x==x,"modal suspends");
 click_label("取消");click_label("重开");click_label("确认重开");require(s.phase==Phase::Ready&&s.score==0,"restart");
 tap(230,220);s.moving={0,10};s.tower[s.count-1]={100,10};s.velocity=0;tap(230,220);require(s.phase==Phase::Dead,"miss ends");screenshot("stack-dead.ppm");leave();
 for(int i=0;i<5;++i){show();leave();}lv_mem_monitor_t a,b;lv_mem_monitor(&a);
 for(int i=0;i<100;++i){show();tap(230,220);leave();}lv_tick_inc(5000);lv_timer_handler();lv_mem_monitor(&b);
 require(a.used_cnt==b.used_cnt&&b.free_size+512>=a.free_size,"no screen or timer leak");
 std::printf("PASS: stack touch, pause, restart, game-over, 100 reentries; heap %zu -> %zu\n",a.free_size,b.free_size);
}

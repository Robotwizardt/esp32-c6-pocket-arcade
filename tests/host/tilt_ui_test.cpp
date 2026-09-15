#include "game_ui_test_common.h"
#include "../../main/game_tilt_ui.h"
using namespace gameTilt;
static bool enabled,available=true;
static tiltInput::Vec acceleration{0,0,1};
static void active(bool value){enabled=value;}
static bool read_sensor(tiltInput::Sample&s){s={acceleration,{.095f+((lv_tick_get()/20)%2 ? .025f : -.025f),.048f,.008f},lv_tick_get()};return enabled&&available;}
static void pump(unsigned ms){for(unsigned i=0;i<ms;i+=20){lv_tick_inc(20);lv_timer_handler();}}
static void feed(tiltInput::Vec a,unsigned ms=2000){acceleration=a;pump(ms);}
int main(){
 init_display();configure(read_sensor,active);test_enter(create_screen);require(enabled,"sensor enabled on entry");screenshot("tilt-calibrate.ppm");
 click_label("开始校准");
 for(unsigned i=0;i<60;++i){acceleration={i%2?.15f:-.15f,0,.988686f};pump(20);}
 require(!find_text(lv_screen_active(),"向右倾斜"),"moving device must not calibrate");
 acceleration={0,0,1};pump(1000);require(find_text(lv_screen_active(),"向右倾斜"),"neutral calibration collects samples");
 click_label("确认方向");require(find_text(lv_screen_active(),"向右倾斜"),"flat direction rejected");
 feed({.3f,0,.953939f});click_label("确认方向");require(find_text(lv_screen_active(),"向下倾斜"),"right axis accepted");
 feed({0,.3f,.953939f});click_label("确认方向");feed({0,0,1});screenshot("tilt-ready.ppm");
 tap(240,300);require(state().phase==Phase::Running,"flat tap starts");auto x=state().x;feed({.3f,0,.953939f},1000);require(state().x>x,"sensor tilt moves ball");screenshot("tilt-play.ppm");
 click_label("暂停");auto frozen=state();pump(1000);require(state().phase==Phase::Paused&&state().elapsed_ms==frozen.elapsed_ms,"pause freezes game");click_label("继续");require(state().phase==Phase::Running,"resume");
 available=false;pump(300);require(state().phase==Phase::Paused&&find_text(lv_screen_active(),"传感器暂无数据"),"sensor loss pauses");available=true;feed({0,0,1});click_label("重试");require(state().phase==Phase::Running,"sensor recovery resumes");
 auto&s=const_cast<State&>(state());const Point goals[]={{330,284},{330,32},{335,294}};
 for(unsigned i=0;i<3;++i){if(s.phase==Phase::Ready){tap(240,300);screenshot(i==1?"tilt-level2.ppm":"tilt-level3.ppm");}s.x=goals[i].x;s.y=goals[i].y;s.vx=s.vy=0;pump(20);require(s.phase==Phase::Won,"goal reached");if(i==0)screenshot("tilt-won.ppm");click_label(i<2?"下一关":"完成挑战");}
 require(s.phase==Phase::Completed,"all levels complete");click_label("重新挑战");require(s.level==1&&s.phase==Phase::Ready,"replay");test_leave();require(!enabled,"sensor disabled on exit");test_lifecycle(create_screen);require(!enabled,"sensor stays off after 100 exits");
 std::puts("PASS: tilt calibration, orientation, sensor motion, pause, loss/recovery, three goals, replay, 100 reentries");
}

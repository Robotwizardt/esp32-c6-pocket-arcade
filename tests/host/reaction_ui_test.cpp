#include "game_ui_test_common.h"
#include "../../main/game_reaction_ui.h"
using namespace gameReaction;
int main(){init_display();configure(42);test_enter(create_screen);screenshot("reaction-ready.ppm");tap(240,280);require(state().phase==Phase::Waiting,"start waits");tap(240,280);require(state().phase==Phase::Early&&state().rounds==0,"early press rejected");
 for(unsigned i=0;i<5;++i){tap(240,280);require(state().phase==Phase::Waiting,"next waits");if(i==0)screenshot("reaction-wait.ppm");lv_tick_inc(5000);lv_timer_handler();require(state().phase==Phase::Go,"green cue");if(i==0)screenshot("reaction-go.ppm");lv_tick_inc(210);tap(240,280);require(state().rounds==i+1,"result captured");}
 require(state().phase==Phase::Summary,"five-round summary");screenshot("reaction-summary.ppm");tap(240,280);click_label("重开");click_label("取消");require(state().phase==Phase::Canceled,"timed attempt canceled fairly");click_label("重开");click_label("确认重开");require(state().phase==Phase::Idle,"restart");test_leave();test_lifecycle(create_screen);std::puts("PASS: reaction touch timing, early press, five rounds, summary, cancel, restart, 100 reentries");}

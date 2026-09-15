#include "game_ui_test_common.h"
#include "../../main/game_memory_ui.h"
using namespace gameMemory;
static void cell(unsigned i){tap(102+(i%4)*88,154+(i/4)*88);}
int main(){init_display();configure(42);test_enter(create_screen);screenshot("memory-ready.ppm");unsigned second=1;while(state().cards[0]==state().cards[second])++second;cell(0);require(state().first==0,"first reveal");cell(0);require(state().moves==0,"duplicate ignored");cell(second);require(state().phase==Phase::Resolving,"mismatch delay");cell((second+1)%16);require(state().moves==1,"third card blocked");lv_tick_inc(800);lv_timer_handler();require(state().phase==Phase::Playing&&state().first==-1,"mismatch concealed");
 for(unsigned value=1;value<=8;++value){for(unsigned i=0;i<16;++i)if(state().cards[i]==value)cell(i);if(value==3)screenshot("memory-play.ppm");}
 require(state().phase==Phase::Won&&state().pairs==8,"all pairs win");screenshot("memory-won.ppm");click_label("再来一局");require(state().pairs==0,"win replay");cell(0);click_label("重开");click_label("取消");require(state().first==0,"cancel preserves");click_label("重开");click_label("确认重开");require(state().first==-1&&state().moves==0,"restart");test_leave();test_lifecycle(create_screen);std::puts("PASS: memory touch reveal, mismatch lock, complete solve, replay, restart, 100 reentries");}

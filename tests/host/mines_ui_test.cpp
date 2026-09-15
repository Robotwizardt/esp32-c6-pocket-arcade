#include "game_ui_test_common.h"
#include "../../main/game_mines_ui.h"
using namespace gameMines;
static void cell(unsigned i){tap(90+(i%6)*60,140+(i/6)*60);}
int main(){init_display();configure(42);test_enter(create_screen);cell(0);require(state().cells[0].revealed&&!state().cells[0].mine,"safe first touch");screenshot("mines-play.ppm");unsigned safe=0;while(state().cells[safe].revealed||state().cells[safe].mine)++safe;
 click_label("切到插旗");cell(safe);require(state().cells[safe].flag,"flag mode");click_label("切到开格");cell(safe);require(!state().cells[safe].revealed,"flag protects");click_label("切到插旗");cell(safe);click_label("切到开格");
 for(unsigned i=0;i<36;++i)if(!state().cells[i].mine)cell(i);require(state().phase==Phase::Won,"open safe cells wins");screenshot("mines-won.ppm");click_label("再来一局");cell(0);unsigned mine=0;while(!state().cells[mine].mine)++mine;cell(mine);require(state().phase==Phase::Dead,"mine ends game");screenshot("mines-dead.ppm");click_label("再来一局");click_label("重开");click_label("取消");click_label("重开");click_label("确认重开");require(state().phase==Phase::Ready,"restart");test_leave();test_lifecycle(create_screen);std::puts("PASS: mines touch open/flag, flood, victory, loss, replay, restart, 100 reentries");}

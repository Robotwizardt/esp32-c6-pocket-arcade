#include "../../main/game_mines.h"
#include <cstdio>
#include <cstdlib>
using namespace gameMines;
static void check(bool b){if(!b)std::abort();}
int main(){
 for(unsigned seed=1;seed<=200;++seed)for(unsigned first=0;first<36;++first){Game g(seed);check(g.reveal(first));auto&s=g.state();unsigned mines=0;for(unsigned i=0;i<36;++i){if(s.cells[i].mine)++mines;if(std::abs((int)(i%6)-(int)(first%6))<=1&&std::abs((int)(i/6)-(int)(first/6))<=1)check(!s.cells[i].mine);}check(mines==6&&s.cells[first].adjacent==0&&s.phase!=Phase::Dead);for(unsigned i=0;i<36;++i)if(!s.cells[i].mine)g.reveal(i);check(s.phase==Phase::Won&&s.opened==30);}
 Game g(9);check(g.flag(0)&&!g.reveal(0)&&g.state().phase==Phase::Ready);for(unsigned i=1;i<6;++i)check(g.flag(i));check(!g.flag(6));check(g.flag(0)&&g.reveal(0));auto&s=g.state();for(unsigned i=0;i<36;++i)if(s.cells[i].mine&&!s.cells[i].flag){g.reveal(i);break;}check(s.phase==Phase::Dead&&!g.flag(35));g.restart();check(g.state().phase==Phase::Ready&&g.state().flags==0&&!g.reveal(36));
 std::puts("PASS: mines 7200 safe starts and wins, six mines, blank expansion, flag protection and cap, loss, restart");
}

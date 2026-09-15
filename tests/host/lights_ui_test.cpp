#include "capture.h"
#include "../../main/game_lights_ui.h"
#include <vector>
using namespace gameLights;
static bool exited;
static void exit_game(){exited=true;}
static void show(){exited=false;configure(42);auto old=lv_screen_active();lv_screen_load(create_screen(exit_game));lv_obj_delete(old);settle();}
static void leave(){click_label("返回");require(exited,"exit");auto old=lv_screen_active();lv_screen_load(lv_obj_create(nullptr));lv_obj_delete(old);settle();}
static uint32_t flip(unsigned i){
 uint32_t m=1u<<i;if(i%5)m|=1u<<(i-1);if(i%5<4)m|=1u<<(i+1);if(i>=5)m|=1u<<(i-5);if(i<20)m|=1u<<(i+5);return m;
}
static std::vector<unsigned> solve(uint32_t initial){
 for(unsigned first=0;first<32;++first){uint32_t b=initial;std::vector<unsigned> v;
  for(unsigned i=0;i<5;++i)if(first&(1u<<i)){b^=flip(i);v.push_back(i);}
  for(unsigned i=5;i<25;++i)if(b&(1u<<(i-5))){b^=flip(i);v.push_back(i);}
  if(!b)return v;
 }return {};
}
// Coordinates are matched to the final UI layout during acceptance.
static void cell(unsigned i){tap(96+(i%5)*72,144+(i/5)*72);}
int main(){
 init_display();show();auto initial=state().board;require(initial!=0,"nonempty puzzle");screenshot("lights-ready.ppm");
 tap(395,40);cell(0);require(state().board==initial,"change modal blocks board");
 tap(144,294);require(state().board==initial,"cancel preserves board");
 cell(0);require(state().steps==1&&state().board==(initial^flip(0)),"touch toggles cross");
 click_label("重置");require(state().board==initial&&state().steps==0,"reset same puzzle");
 auto solution=solve(initial);require(!solution.empty(),"solvable");
 for(auto i:solution){if(state().phase==Phase::Won)break;cell(i);}
 require(state().phase==Phase::Won&&state().board==0,"solve via touch");screenshot("lights-won.ppm");
 click_label("下一关");require(state().level==2&&state().board!=0,"next puzzle");
 tap(395,40);tap(336,294);require(state().level==2&&state().steps==0&&state().board!=0,"confirmed change puzzle");leave();
 for(int i=0;i<5;++i){show();leave();}lv_mem_monitor_t a,b;lv_mem_monitor(&a);
 for(int i=0;i<100;++i){show();cell(0);leave();}lv_mem_monitor(&b);
 require(a.used_cnt==b.used_cnt&&b.free_size+512>=a.free_size,"screen allocations released");
 std::printf("PASS: lights touch cross, reset, solve, next level, 100 reentries; heap %zu -> %zu\n",a.free_size,b.free_size);
}

#include "game_memory_ui.h"
#include "game_ui_common.h"
#include <cstdio>
namespace gameMemory {
namespace {
using namespace gameUi;
Game model;lv_obj_t*screen,*overlay,*hud,*cells[16],*values[16];lv_timer_t*timer;uint32_t last;void(*exit_fn)();
void render(){char t[64];const auto&s=model.state();std::snprintf(t,sizeof(t),"步数 %u    配对 %u / 8",s.moves,s.pairs);lv_label_set_text(hud,t);for(int i=0;i<16;++i){bool shown=s.matched[i]||s.first==i||s.second==i;std::snprintf(t,sizeof(t),"%u",s.cards[i]);lv_label_set_text(values[i],shown?t:"?");lv_obj_center(values[i]);lv_obj_set_style_bg_color(cells[i],lv_color_hex(s.matched[i]?0x176343:shown?0x624490:0x172A42),0);}}
void close(){if(overlay){auto o=overlay;overlay=nullptr;lv_obj_delete_async(o);}last=lv_tick_get();}
void returned(lv_event_t*){if(exit_fn)exit_fn();}
void restarted(lv_event_t*){close();model.restart();render();}
void restart_request(lv_event_t*){if(!overlay)overlay=dialog(screen,"重新开始？","当前配对进度会清空。","取消","确认重开",[](lv_event_t*){close();},restarted);}
void clicked(lv_event_t*e){if(overlay)return;unsigned i=(uintptr_t)lv_event_get_user_data(e);if(model.reveal(i)){last=lv_tick_get();render();if(model.state().phase==Phase::Won)overlay=dialog(screen,"全部配对！","八组卡片全部找到了。","返回","再来一局",returned,restarted);}}
void tick(lv_timer_t*){auto now=lv_tick_get();auto dt=now-last;last=now;if(overlay)return;auto phase=model.state().phase;model.advance(dt);if(phase!=model.state().phase)render();}
void deleted(lv_event_t*e){if(lv_event_get_target_obj(e)!=screen)return;if(timer){lv_timer_delete(timer);timer=nullptr;}screen=overlay=hud=nullptr;}
}
void configure(uint32_t seed){model=Game(seed);}const State&state(){return model.state();}
lv_obj_t*create_screen(void(*exit)()){
 model.restart();exit_fn=exit;overlay=nullptr;screen=box(nullptr,0,0,480,480,0x070A12);lv_obj_set_style_radius(screen,0,0);
 button(screen,24,18,88,"返回",returned);auto t=text(screen,"记忆翻牌",&lobby_zh_24);lv_obj_set_pos(t,142,28);button(screen,344,18,112,"重开",restart_request);
 hud=text(screen,"");lv_obj_set_pos(hud,64,82);
 for(int i=0;i<16;++i){cells[i]=box(screen,64+(i%4)*88,116+(i/4)*88,76,76,0x172A42);values[i]=text(cells[i],"?",&lobby_zh_28);lv_obj_center(values[i]);lv_obj_add_event_cb(cells[i],clicked,LV_EVENT_CLICKED,(void*)(uintptr_t)i);}
 render();last=lv_tick_get();timer=lv_timer_create(tick,20,nullptr);lv_obj_add_event_cb(screen,deleted,LV_EVENT_DELETE,nullptr);return screen;
}
}

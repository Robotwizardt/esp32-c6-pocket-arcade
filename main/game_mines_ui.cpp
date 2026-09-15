#include "game_mines_ui.h"
#include "game_ui_common.h"
#include <cstdio>
namespace gameMines {
namespace {
using namespace gameUi;
Game model;lv_obj_t *screen,*overlay,*hud,*mode_button,*cells[36],*labels[36];bool flags;void(*exit_fn)();
void render(){const auto&s=model.state();char t[64];std::snprintf(t,sizeof(t),"余雷 %u    已开 %u / 30",6-s.flags,s.opened);lv_label_set_text(hud,t);
 lv_label_set_text(lv_obj_get_child(mode_button,0),flags?"切到开格":"切到插旗");
 for(unsigned i=0;i<36;++i){auto c=s.cells[i];uint32_t bg=0x172A42,fg=0xF8FAFC;t[0]=0;
  if(c.mine&&(s.phase==Phase::Dead||s.phase==Phase::Won)){std::snprintf(t,sizeof(t),"%s",s.phase==Phase::Won?"旗":"雷");bg=s.phase==Phase::Won?0x176343:0x8D3044;}
  else if(c.flag){std::snprintf(t,sizeof(t),"旗");bg=0x6F5126;}
  else if(c.revealed){bg=0x0D1C2C;if(c.adjacent){std::snprintf(t,sizeof(t),"%u",c.adjacent);static const uint32_t colors[]={0,0x60A5FA,0x4ADE80,0xFB7185,0xC084FC,0xFBBF24,0x2DD4BF,0xF8FAFC,0xCBD5E1};fg=colors[c.adjacent];}}
  lv_obj_set_style_bg_color(cells[i],lv_color_hex(bg),0);lv_obj_set_style_text_color(labels[i],lv_color_hex(fg),0);lv_label_set_text(labels[i],t);lv_obj_center(labels[i]);
 }
}
void close(){if(overlay){auto old=overlay;overlay=nullptr;lv_obj_delete_async(old);}}
void returned(lv_event_t*){if(exit_fn)exit_fn();}
void restarted(lv_event_t*){close();model.restart();flags=false;render();}
void restart_request(lv_event_t*){if(!overlay)overlay=dialog(screen,"重新开始？","当前扫雷进度会清空。","取消","确认重开",[](lv_event_t*){close();},restarted);}
void mode(lv_event_t*){if(!overlay){flags=!flags;render();}}
void clicked(lv_event_t*e){if(overlay)return;unsigned i=(uintptr_t)lv_event_get_user_data(e);bool changed=flags?model.flag(i):model.reveal(i);if(!changed)return;render();if(model.state().phase==Phase::Dead)overlay=dialog(screen,"踩到雷了","再来一局，首次开格保证安全。","返回","再来一局",returned,restarted);else if(model.state().phase==Phase::Won)overlay=dialog(screen,"扫雷成功！","所有安全格都打开了。","返回","再来一局",returned,restarted);}
void deleted(lv_event_t*e){if(lv_event_get_target_obj(e)==screen)screen=overlay=hud=mode_button=nullptr;}
}
void configure(uint32_t seed){model=Game(seed);}const State&state(){return model.state();}
lv_obj_t*create_screen(void(*exit)()){
 model.restart();flags=false;overlay=nullptr;exit_fn=exit;screen=box(nullptr,0,0,480,480,0x070A12);lv_obj_set_style_radius(screen,0,0);
 button(screen,24,18,88,"返回",returned);auto t=text(screen,"扫雷",&lobby_zh_24);lv_obj_set_pos(t,146,28);mode_button=button(screen,244,18,112,"切到插旗",mode);button(screen,364,18,92,"重开",restart_request);
 hud=text(screen,"");lv_obj_set_pos(hud,62,80);
 for(int i=0;i<36;++i){cells[i]=box(screen,62+(i%6)*60,112+(i/6)*60,56,56,0x172A42);labels[i]=text(cells[i],"",&lobby_zh_24);lv_obj_add_event_cb(cells[i],clicked,LV_EVENT_CLICKED,(void*)(uintptr_t)i);}
 render();lv_obj_add_event_cb(screen,deleted,LV_EVENT_DELETE,nullptr);return screen;
}
}

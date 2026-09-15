#include "game_breakout_ui.h"
#include "game_ui_common.h"
#include <cstdio>
#include <cstdlib>
namespace gameBreakout {
namespace {
using namespace gameUi;
Game model;lv_obj_t*screen,*overlay,*board,*hud,*hint;lv_timer_t*timer;uint32_t last;void(*exit_fn)();lv_point_t press;bool dragged;
void render(){const auto&s=model.state();char t[64];std::snprintf(t,sizeof(t),"击碎 %u / 24    生命 %u",s.score,s.lives);lv_label_set_text(hud,t);lv_label_set_text(hint,s.phase==Phase::Ready?"点击发球，左右拖动挡板":"左右拖动接球，清除全部砖块");lv_obj_invalidate(board);}
void rect(lv_layer_t*l,int x,int y,int w,int h,uint32_t c,int radius=4){lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_color_hex(c);d.bg_opa=LV_OPA_COVER;d.radius=radius;lv_area_t a{x,y,x+w-1,y+h-1};lv_draw_rect(l,&d,&a);}
void draw(lv_event_t*e){lv_area_t a;lv_obj_get_coords(board,&a);auto l=lv_event_get_layer(e);const auto&s=model.state();static const uint32_t colors[]={0xFB7185,0xFBBF24,0x4ADE80,0x38BDF8};for(unsigned i=0;i<24;++i)if(s.bricks[i])rect(l,a.x1+8+(i%6)*60,a.y1+28+(i/6)*28,52,18,colors[i/6]);rect(l,a.x1+(int)(s.paddle-48),a.y1+300,96,10,0xA5B4FC);rect(l,a.x1+(int)s.x-5,a.y1+(int)s.y-5,10,10,0xF8FAFC,5);}
void close(){if(overlay){auto o=overlay;overlay=nullptr;lv_obj_delete_async(o);}last=lv_tick_get();}
void returned(lv_event_t*){if(exit_fn)exit_fn();}
void restarted(lv_event_t*){close();model.restart();render();}
void resumed(lv_event_t*){close();model.resume();render();}
void paused(lv_event_t*){if(!overlay&&model.state().phase==Phase::Running){model.pause();overlay=dialog(screen,"游戏已暂停","准备好后继续接球。","返回","继续",returned,resumed);}}
void restart_request(lv_event_t*){if(!overlay)overlay=dialog(screen,"重新开始？","将恢复三条生命和全部砖块。","取消","确认重开",[](lv_event_t*){close();},restarted);}
void input(lv_event_t*e){if(overlay)return;auto code=lv_event_get_code(e);auto indev=lv_indev_active();if(!indev)return;lv_point_t p;lv_indev_get_point(indev,&p);
 if(code==LV_EVENT_PRESSED){press=p;dragged=false;}
 if(code==LV_EVENT_PRESSING){if(std::abs(p.x-press.x)>15||std::abs(p.y-press.y)>15)dragged=true;model.paddle(p.x-56);render();}
 if(code==LV_EVENT_CLICKED&&!dragged){model.paddle(p.x-56);model.start();last=lv_tick_get();render();}
}
void tick(lv_timer_t*){auto now=lv_tick_get();auto dt=now-last;last=now;if(overlay)return;auto before=model.state().phase;model.advance(dt);if(before==Phase::Running)render();if(model.state().phase==Phase::Won)overlay=dialog(screen,"全部击碎！","二十四块砖全部清除了。","返回","再来一局",returned,restarted);else if(model.state().phase==Phase::Dead)overlay=dialog(screen,"挑战结束","三条生命已用完，再试一次吧。","返回","再来一局",returned,restarted);}
void deleted(lv_event_t*e){if(lv_event_get_target_obj(e)!=screen)return;if(timer){lv_timer_delete(timer);timer=nullptr;}screen=overlay=board=hud=hint=nullptr;}
}
const State&state(){return model.state();}
lv_obj_t*create_screen(void(*exit)()){
 model.restart();overlay=nullptr;exit_fn=exit;screen=box(nullptr,0,0,480,480,0x070A12);lv_obj_set_style_radius(screen,0,0);
 button(screen,24,18,88,"返回",returned);auto t=text(screen,"打砖块",&lobby_zh_24);lv_obj_set_pos(t,142,28);button(screen,264,18,88,"暂停",paused);button(screen,364,18,92,"重开",restart_request);
 hud=text(screen,"");lv_obj_set_pos(hud,56,80);board=box(screen,56,114,368,326,0x102337);lv_obj_add_event_cb(board,draw,LV_EVENT_DRAW_MAIN,nullptr);lv_obj_add_event_cb(board,input,LV_EVENT_ALL,nullptr);
 hint=text(screen,"",&lobby_zh_16);lv_obj_set_pos(hint,56,448);render();last=lv_tick_get();timer=lv_timer_create(tick,20,nullptr);lv_obj_add_event_cb(screen,deleted,LV_EVENT_DELETE,nullptr);return screen;
}
}

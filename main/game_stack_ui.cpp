#include "game_stack_ui.h"
#include "fonts/lobby_fonts.h"
#include <cstdio>
#include <cstdlib>
namespace gameStack {
namespace {
Game model;
lv_obj_t *screen,*board,*score_label,*hint,*overlay;
lv_timer_t *timer;
uint32_t last_tick,best;
void(*exit_fn)();
lv_point_t press;
bool dragged;
lv_obj_t* box(lv_obj_t* p,int x,int y,int w,int h,uint32_t c){
 auto o=lv_obj_create(p);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
 lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_bg_color(o,lv_color_hex(c),0);
 lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_set_style_radius(o,12,0);return o;
}
lv_obj_t* label(lv_obj_t*p,const char*t,const lv_font_t*f){
 auto o=lv_label_create(p);lv_label_set_text(o,t);lv_obj_set_style_text_font(o,f,0);
 lv_obj_set_style_text_color(o,lv_color_hex(0xF8FAFC),0);lv_obj_clear_flag(o,LV_OBJ_FLAG_CLICKABLE);return o;
}
void button(lv_obj_t*p,int x,int y,int w,const char*t,lv_event_cb_t cb){
 auto b=box(p,x,y,w,48,0x172A42);lv_obj_add_flag(b,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_set_style_bg_color(b,lv_color_hex(0x365474),LV_STATE_PRESSED);
 auto l=label(b,t,&lobby_zh_20);lv_obj_center(l);lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,nullptr);
}
void refresh(){
 char t[64];if(model.state().score>best)best=model.state().score;
 std::snprintf(t,sizeof(t),"层数 %u    最高 %lu",model.state().score,(unsigned long)best);lv_label_set_text(score_label,t);
 const char*msg=model.state().phase==Phase::Ready?"点击棋盘开始":model.state().perfect?"完美对齐！继续叠高":"点击落块，对齐保留重叠部分";
 lv_label_set_text(hint,msg);lv_obj_invalidate(board);
}
void rect(lv_layer_t*l,int x,int y,int w,int h,uint32_t c){
 lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_color_hex(c);d.bg_opa=LV_OPA_COVER;d.radius=4;
 lv_area_t a{x,y,x+w-1,y+h-1};lv_draw_rect(l,&d,&a);
}
void draw(lv_event_t*e){
 auto l=lv_event_get_layer(e);lv_area_t a;lv_obj_get_coords(board,&a);const auto&s=model.state();
 for(unsigned i=0;i<s.count;++i){auto b=s.tower[i];rect(l,a.x1+(int)b.x,a.y2-26-(int)i*24,(int)b.width,22,i%2?0x38BDF8:0x4F83E8);}
 if(s.phase!=Phase::Dead)rect(l,a.x1+(int)s.moving.x,a.y2-26-(int)s.count*24,(int)s.moving.width,22,0xFBBF24);
}
void close(){if(overlay){auto old=overlay;overlay=nullptr;lv_obj_delete_async(old);}last_tick=lv_tick_get();}
void modal(const char*title,const char*left,const char*right,lv_event_cb_t lc,lv_event_cb_t rc){
 if(overlay)return;
 overlay=box(screen,0,0,480,480,0x000000);lv_obj_set_style_radius(overlay,0,0);lv_obj_set_style_bg_opa(overlay,205,0);
 auto panel=box(overlay,36,154,408,180,0x15283D);auto t=label(panel,title,&lobby_zh_24);lv_obj_set_pos(t,24,24);
 button(panel,24,104,164,left,lc);button(panel,220,104,164,right,rc);
}
void returned(lv_event_t*){if(exit_fn)exit_fn();}
void restarted(lv_event_t*){close();model.restart();refresh();}
void resumed(lv_event_t*){close();model.resume();refresh();}
void paused(lv_event_t*){
 if(model.state().phase!=Phase::Running||overlay)return;
 model.pause();modal("游戏已暂停","返回","继续",returned,resumed);
}
void restart_request(lv_event_t*){
 if(overlay)return;
 modal("重新开始？","取消","确认重开",[](lv_event_t*){close();},restarted);
}
void input(lv_event_t*e){
 auto code=lv_event_get_code(e);auto indev=lv_indev_active();
 if(code==LV_EVENT_PRESSED){dragged=false;if(indev)lv_indev_get_point(indev,&press);}
 if(code==LV_EVENT_RELEASED && indev){lv_point_t p;lv_indev_get_point(indev,&p);dragged=std::abs(p.x-press.x)>20||std::abs(p.y-press.y)>20;}
 if(code!=LV_EVENT_CLICKED||dragged||overlay)return;
 if(model.state().phase==Phase::Ready){model.start();last_tick=lv_tick_get();}
 else if(model.state().phase==Phase::Running){
  model.advance(lv_tick_get()-last_tick);last_tick=lv_tick_get();
  if(!model.drop())modal("挑战结束","返回","再来一次",returned,restarted);
 }
 refresh();
}
void tick(lv_timer_t*){
 auto now=lv_tick_get();auto dt=now-last_tick;last_tick=now;
 if(!overlay && model.state().phase==Phase::Running){model.advance(dt);lv_obj_invalidate(board);}
}
void deleted(lv_event_t*e){
 if(lv_event_get_target_obj(e)!=screen)return;
 if(timer){lv_timer_delete(timer);timer=nullptr;}screen=board=score_label=hint=overlay=nullptr;
}
}
const State& state(){return model.state();}
lv_obj_t* create_screen(void(*exit)()){
 model.restart();exit_fn=exit;dragged=false;overlay=nullptr;
 screen=box(nullptr,0,0,480,480,0x070A12);lv_obj_set_style_radius(screen,0,0);
 button(screen,24,18,88,"返回",returned);auto t=label(screen,"叠叠高",&lobby_zh_24);lv_obj_set_pos(t,140,28);
 button(screen,264,18,88,"暂停",paused);button(screen,364,18,92,"重开",restart_request);
 score_label=label(screen,"",&lobby_zh_20);lv_obj_set_pos(score_label,56,80);
 board=box(screen,56,114,368,326,0x102337);lv_obj_add_event_cb(board,draw,LV_EVENT_DRAW_MAIN,nullptr);lv_obj_add_event_cb(board,input,LV_EVENT_ALL,nullptr);
 hint=label(screen,"",&lobby_zh_16);lv_obj_set_pos(hint,56,448);refresh();
 last_tick=lv_tick_get();timer=lv_timer_create(tick,30,nullptr);lv_obj_add_event_cb(screen,deleted,LV_EVENT_DELETE,nullptr);return screen;
}
}

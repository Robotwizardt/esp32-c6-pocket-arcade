#include "game_reaction_ui.h"
#include "fonts/lobby_fonts.h"
#include <cstdio>
namespace gameReaction {
namespace {
Game model;
lv_obj_t *screen,*area,*headline,*subline,*progress,*overlay;
lv_timer_t *timer;void(*exit_fn)();
lv_obj_t* box(lv_obj_t*p,int x,int y,int w,int h,uint32_t c){auto o=lv_obj_create(p);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_bg_color(o,lv_color_hex(c),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_set_style_radius(o,18,0);return o;}
lv_obj_t* text(lv_obj_t*p,const char*t,const lv_font_t*f){auto l=lv_label_create(p);lv_label_set_text(l,t);lv_obj_set_style_text_font(l,f,0);lv_obj_set_style_text_color(l,lv_color_hex(0xF8FAFC),0);lv_obj_clear_flag(l,LV_OBJ_FLAG_CLICKABLE);return l;}
void button(lv_obj_t*p,int x,int y,int w,const char*t,lv_event_cb_t cb){auto b=box(p,x,y,w,48,0x172A42);lv_obj_set_style_radius(b,12,0);auto l=text(b,t,&lobby_zh_20);lv_obj_center(l);lv_obj_set_style_bg_color(b,lv_color_hex(0x365474),LV_STATE_PRESSED);lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,nullptr);}
void render(){
 const auto&s=model.state();uint32_t color=0x172A42;const char*title="测测你的反应";const char*hint="点击开始，变绿后立即点击";char value[64],count[48];
 switch(s.phase){
 case Phase::Idle:break;
 case Phase::Waiting:color=0x7F2838;title="等待变绿";hint="现在不要点";break;
 case Phase::Go:color=0x14613F;title="现在点击！";hint="越快越好";break;
 case Phase::Early:color=0x7F2838;title="抢跑了";hint="本次不计入，点击重试";break;
 case Phase::Canceled:title="本次已取消";hint="本次不计入，点击重试";break;
 case Phase::Timeout:title="没有及时点击";hint="本次不计入，点击重试";break;
 case Phase::Result:std::snprintf(value,sizeof(value),"%lu ms",(unsigned long)s.last);title=value;hint="点击开始下一次";break;
 case Phase::Summary:std::snprintf(value,sizeof(value),"平均 %lu ms",(unsigned long)s.average);title=value;hint="已完成五次，点击再测一组";break;
 }
 std::snprintf(count,sizeof(count),"有效测试 %u / 5",s.rounds);lv_label_set_text(progress,count);
 lv_obj_set_style_bg_color(area,lv_color_hex(color),0);lv_label_set_text(headline,title);lv_obj_align(headline,LV_ALIGN_CENTER,0,-28);lv_label_set_text(subline,hint);lv_obj_align(subline,LV_ALIGN_CENTER,0,28);
}
void touch(lv_event_t*){if(!overlay){model.press(lv_tick_get());render();}}
void tick(lv_timer_t*){if(!overlay){auto old=model.state().phase;model.advance(lv_tick_get());if(old!=model.state().phase)render();}}
void returned(lv_event_t*){if(exit_fn)exit_fn();}
void close(){if(overlay){auto old=overlay;overlay=nullptr;lv_obj_delete_async(old);}}
void restart_request(lv_event_t*){
 if(overlay)return;
 // A timed attempt cannot be paused fairly; abandon it without recording a result.
 if(model.state().phase==Phase::Waiting||model.state().phase==Phase::Go){model.cancel_attempt();render();}
 overlay=box(screen,0,0,480,480,0);lv_obj_set_style_radius(overlay,0,0);lv_obj_set_style_bg_opa(overlay,205,0);
 auto p=box(overlay,36,150,408,190,0x15283D);auto l=text(p,"重新开始五次测试？",&lobby_zh_24);lv_obj_set_pos(l,24,24);
 button(p,24,112,164,"取消",[](lv_event_t*){close();});button(p,220,112,164,"确认重开",[](lv_event_t*){close();model.restart();render();});
}
void deleted(lv_event_t*e){if(lv_event_get_target_obj(e)!=screen)return;if(timer){lv_timer_delete(timer);timer=nullptr;}screen=area=headline=subline=progress=overlay=nullptr;}
}
void configure(uint32_t seed){model=Game(seed);}
const State& state(){return model.state();}
lv_obj_t* create_screen(void(*exit)()){
 model.restart();exit_fn=exit;overlay=nullptr;screen=box(nullptr,0,0,480,480,0x070A12);lv_obj_set_style_radius(screen,0,0);
 button(screen,24,18,88,"返回",returned);auto title=text(screen,"反应挑战",&lobby_zh_24);lv_obj_set_pos(title,146,28);button(screen,344,18,112,"重开",restart_request);
 progress=text(screen,"",&lobby_zh_20);lv_obj_set_pos(progress,56,84);
 area=box(screen,48,126,384,304,0x172A42);lv_obj_add_event_cb(area,touch,LV_EVENT_PRESSED,nullptr);
 headline=text(area,"",&lobby_zh_28);subline=text(area,"",&lobby_zh_20);render();
 timer=lv_timer_create(tick,10,nullptr);lv_obj_add_event_cb(screen,deleted,LV_EVENT_DELETE,nullptr);return screen;
}
}

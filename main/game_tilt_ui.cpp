#include "game_tilt_ui.h"
#include "game_ui_common.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace gameTilt {
namespace {
using namespace gameUi;
enum class Setup {Prompt,Neutral,Right,Down,Done};
Game model;tiltInput::Filter filter;tiltInput::Read read_fn;tiltInput::Active active_fn;
lv_obj_t*screen,*board,*hud,*hint,*overlay;lv_timer_t*timer;void(*exit_fn)();
Setup setup;tiltInput::Sample current,average,anchor;unsigned samples;bool sensor_ok,has_sample;
uint32_t last_tick,last_received,last_stamp,last_draw;const char*message="";
void render(){
 char t[64];std::snprintf(t,sizeof(t),"第 %u / 3 关    用时 %lu 秒",model.state().level,(unsigned long)(model.state().elapsed_ms/1000));lv_label_set_text(hud,t);
 if(!sensor_ok)lv_label_set_text(hint,"等待传感器数据，请稍候");
 else if(setup==Setup::Neutral){std::snprintf(t,sizeof(t),"保持平放静止 %u%%",samples*100/40);lv_label_set_text(hint,t);}
 else lv_label_set_text(hint,message);
 lv_obj_invalidate(board);
}
void rect(lv_layer_t*l,int x,int y,int w,int h,uint32_t c,int r=4){lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.bg_color=lv_color_hex(c);d.bg_opa=LV_OPA_COVER;d.radius=r;lv_area_t a{x,y,x+w-1,y+h-1};lv_draw_rect(l,&d,&a);}
void draw(lv_event_t*e){auto layer=lv_event_get_layer(e);lv_area_t a;lv_obj_get_coords(board,&a);const auto&geo=model.level();
 for(unsigned i=0;i<geo.count;++i){auto r=geo.arrayRects[i];rect(layer,a.x1+(int)r.x,a.y1+(int)r.y,(int)r.w,(int)r.h,0x4B6685);}
 rect(layer,a.x1+(int)geo.start.x-7,a.y1+(int)geo.start.y-7,14,14,0x345C75,7);
 rect(layer,a.x1+(int)geo.goal.x-16,a.y1+(int)geo.goal.y-16,32,32,0x4ADE80,16);
 rect(layer,a.x1+(int)geo.goal.x-10,a.y1+(int)geo.goal.y-10,20,20,0x113B2B,10);
 rect(layer,a.x1+(int)model.state().x-10,a.y1+(int)model.state().y-10,20,20,0xFBBF24,10);
}
void close(){if(overlay){auto o=overlay;overlay=nullptr;lv_obj_delete_async(o);}last_tick=lv_tick_get();}
void returned(lv_event_t*){if(exit_fn)exit_fn();}
void confirm_right(lv_event_t*);void confirm_down(lv_event_t*);
void prompt_right(const char*body){close();overlay=dialog(screen,"向右倾斜",body,"返回","确认方向",returned,confirm_right);}
void prompt_down(const char*body){close();overlay=dialog(screen,"向下倾斜",body,"返回","确认方向",returned,confirm_down);}
void confirm_right(lv_event_t*){
 if(!sensor_ok||!filter.right()){prompt_right("让屏幕右边缘降低一些，保持后确认。");return;}
 setup=Setup::Down;prompt_down("让屏幕下边缘降低，保持后确认。");
}
void confirm_down(lv_event_t*){
 if(!sensor_ok||!filter.down()){prompt_down("请朝下边缘倾斜，不要沿右侧方向。");return;}
 setup=Setup::Done;close();message="校准完成，放平后点棋盘开始";render();
}
void begin_calibration(lv_event_t*){close();model.pause();filter.reset();setup=Setup::Neutral;samples=0;average={};message="保持平放静止";render();}
void calibration_request(lv_event_t*){
 if(overlay)return;
 model.pause();setup=Setup::Prompt;overlay=dialog(screen,"方向校准","屏幕朝上平放，随后按提示倾斜。","返回","开始校准",returned,begin_calibration);
}
void restart(lv_event_t*){close();model.restart();message="放平后点击棋盘开始";render();}
void resumed(lv_event_t*){if(!sensor_ok){message="仍未读到数据，请稍候";render();return;}close();model.resume();message="倾斜屏幕，让小球进入绿圈";render();}
void pause_request(lv_event_t*){if(overlay||setup!=Setup::Done)return;if(model.state().phase==Phase::Running){model.pause();overlay=dialog(screen,"游戏已暂停","继续当前关卡，或从第一关重来。","重来","继续",restart,resumed);}}
void next(lv_event_t*){close();model.next_level();if(model.state().phase==Phase::Completed)overlay=dialog(screen,"全部通关！","三个迷宫全部完成。","返回","重新挑战",returned,restart);else{message="放平后点击棋盘开始";render();}}
void tapped(lv_event_t*){
 if(overlay||setup!=Setup::Done||!sensor_ok)return;
 auto control=filter.control();if(std::abs(control.x)>.15f||std::abs(control.y)>.15f){message="请先放平，再点击棋盘";render();return;}
 if(model.state().phase==Phase::Ready)model.start();else if(model.state().phase==Phase::Paused)model.resume();
 message="倾斜屏幕，让小球进入绿圈";last_tick=lv_tick_get();render();
}
void collect_neutral(const tiltInput::Sample&s){
 const auto&a=s.accel;const auto&g=s.gyro;float magnitude=std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);float rotation=std::sqrt(g.x*g.x+g.y*g.y+g.z*g.z);
 // Check stability relative to the first sample, not uncalibrated gyro zero.
 auto distance=[](tiltInput::Vec u,tiltInput::Vec v){return std::sqrt((u.x-v.x)*(u.x-v.x)+(u.y-v.y)*(u.y-v.y)+(u.z-v.z)*(u.z-v.z));};
 if(magnitude<.85f||magnitude>1.15f||rotation>.50f){samples=0;average={};return;}
 if(samples&&(distance(a,anchor.accel)>.06f||distance(g,anchor.gyro)>.08f)){samples=0;average={};}
 if(!samples)anchor=s;
 average.accel.x+=a.x;average.accel.y+=a.y;average.accel.z+=a.z;average.gyro.x+=g.x;average.gyro.y+=g.y;average.gyro.z+=g.z;++samples;
 if(samples==40){average.accel.x/=40;average.accel.y/=40;average.accel.z/=40;average.gyro.x/=40;average.gyro.y/=40;average.gyro.z/=40;
  if(filter.neutral(average)){setup=Setup::Right;prompt_right("让屏幕右边缘降低，保持后确认。");}else{samples=0;average={};}
 }
}
void tick(lv_timer_t*){
 const bool previous_ok=sensor_ok;const auto previous_phase=model.state().phase;
 uint32_t now=lv_tick_get(),dt=now-last_tick;last_tick=now;tiltInput::Sample s;bool valid=read_fn&&read_fn(s);
 if(valid&&(!has_sample||s.millis!=last_stamp)){
  float sample_dt=has_sample?(s.millis-last_stamp)/1000.0f:.02f;
  if(filter.update(s,sample_dt)){current=s;last_stamp=s.millis;last_received=now;has_sample=true;if(setup==Setup::Neutral)collect_neutral(s);}else valid=false;
 }
 sensor_ok=valid&&has_sample&&now-last_received<200;
 if(!sensor_ok&&setup==Setup::Neutral){samples=0;average={};}
 if(!overlay&&setup==Setup::Done&&model.state().phase==Phase::Running){
  if(!sensor_ok){model.pause();overlay=dialog(screen,"传感器暂无数据","游戏已暂停，恢复数据后可继续。","返回","重试",returned,resumed);}
  else{auto c=filter.control();model.advance(std::min<uint32_t>(dt,100),c.x,c.y);if(model.state().phase==Phase::Won)overlay=dialog(screen,"到达终点！","稳稳地进入了绿色终点。","返回",model.state().level<3?"下一关":"完成挑战",returned,next);}
 }
 if(previous_ok!=sensor_ok||previous_phase!=model.state().phase||((model.state().phase==Phase::Running||setup==Setup::Neutral)&&now-last_draw>=40)){render();last_draw=now;}
}
void deleted(lv_event_t*e){if(lv_event_get_target_obj(e)!=screen)return;if(timer){lv_timer_delete(timer);timer=nullptr;}if(active_fn)active_fn(false);screen=board=hud=hint=overlay=nullptr;}
}
void configure(tiltInput::Read read,tiltInput::Active active){read_fn=read;active_fn=active;}
const State&state(){return model.state();}
lv_obj_t*create_screen(void(*exit)()){
 model.restart();filter.reset();setup=Setup::Prompt;overlay=nullptr;exit_fn=exit;sensor_ok=has_sample=false;samples=0;message="先校准方向";
 screen=box(nullptr,0,0,480,480,0x070A12);lv_obj_set_style_radius(screen,0,0);button(screen,24,18,88,"返回",returned);auto title=text(screen,"重力滚球",&lobby_zh_24);lv_obj_set_pos(title,142,28);button(screen,264,18,88,"暂停",pause_request);button(screen,364,18,92,"校准",calibration_request);
 hud=text(screen,"");lv_obj_set_pos(hud,56,80);board=box(screen,56,114,368,326,0x102337);lv_obj_add_event_cb(board,draw,LV_EVENT_DRAW_MAIN,nullptr);lv_obj_add_event_cb(board,tapped,LV_EVENT_CLICKED,nullptr);hint=text(screen,"",&lobby_zh_16);lv_obj_set_pos(hint,56,448);
 if(active_fn) active_fn(true);
 last_tick=lv_tick_get();last_received=last_draw=last_tick;timer=lv_timer_create(tick,20,nullptr);lv_obj_add_event_cb(screen,deleted,LV_EVENT_DELETE,nullptr);calibration_request(nullptr);render();return screen;
}
}

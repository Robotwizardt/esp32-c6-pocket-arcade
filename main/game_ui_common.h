#pragma once
#include "lvgl.h"
#include "fonts/lobby_fonts.h"
namespace gameUi {
inline lv_obj_t* box(lv_obj_t*p,int x,int y,int w,int h,uint32_t c){auto o=lv_obj_create(p);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_clear_flag(o,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_style_bg_color(o,lv_color_hex(c),0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_set_style_radius(o,12,0);return o;}
inline lv_obj_t* text(lv_obj_t*p,const char*t,const lv_font_t*f=&lobby_zh_20){auto l=lv_label_create(p);lv_label_set_text(l,t);lv_obj_set_style_text_font(l,f,0);lv_obj_set_style_text_color(l,lv_color_hex(0xF8FAFC),0);lv_obj_clear_flag(l,LV_OBJ_FLAG_CLICKABLE);return l;}
inline lv_obj_t* button(lv_obj_t*p,int x,int y,int w,const char*t,lv_event_cb_t cb,void*data=nullptr){auto b=box(p,x,y,w,48,0x172A42);auto l=text(b,t);lv_obj_center(l);lv_obj_set_style_bg_color(b,lv_color_hex(0x365474),LV_STATE_PRESSED);lv_obj_add_event_cb(b,cb,LV_EVENT_CLICKED,data);return b;}
inline lv_obj_t* dialog(lv_obj_t*screen,const char*title,const char*body,const char*left,const char*right,lv_event_cb_t lc,lv_event_cb_t rc){auto o=box(screen,0,0,480,480,0);lv_obj_set_style_radius(o,0,0);lv_obj_set_style_bg_opa(o,205,0);auto p=box(o,36,144,408,204,0x15283D);auto t=text(p,title,&lobby_zh_24);lv_obj_set_pos(t,24,20);auto b=text(p,body,&lobby_zh_16);lv_obj_set_pos(b,24,60);lv_obj_set_width(b,360);lv_label_set_long_mode(b,LV_LABEL_LONG_WRAP);button(p,24,132,164,left,lc);button(p,220,132,164,right,rc);return o;}
}


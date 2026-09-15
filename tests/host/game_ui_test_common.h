#pragma once
#include "capture.h"
static bool test_exited;
static void test_exit(){test_exited=true;}
using ScreenFactory=lv_obj_t*(*)(void(*)());
static void test_enter(ScreenFactory factory){test_exited=false;auto old=lv_screen_active();lv_screen_load(factory(test_exit));lv_obj_delete(old);settle();}
static void test_leave(){click_label("返回");require(test_exited,"return callback");auto old=lv_screen_active();lv_screen_load(lv_obj_create(nullptr));lv_obj_delete(old);settle();}
static void test_lifecycle(ScreenFactory factory){for(int i=0;i<5;++i){test_enter(factory);test_leave();}lv_mem_monitor_t a,b;lv_mem_monitor(&a);for(int i=0;i<100;++i){test_enter(factory);test_leave();}lv_tick_inc(5000);lv_timer_handler();lv_mem_monitor(&b);require(a.used_cnt==b.used_cnt&&b.free_size+512>=a.free_size,"no retained screen/timer allocations");std::printf("HEAP %zu -> %zu, max %zu\n",a.free_size,b.free_size,b.max_used);}

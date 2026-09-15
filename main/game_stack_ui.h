#pragma once
#include "game_stack.h"
#include "lvgl.h"
namespace gameStack {
lv_obj_t* create_screen(void(*exit)());
const State& state();
}

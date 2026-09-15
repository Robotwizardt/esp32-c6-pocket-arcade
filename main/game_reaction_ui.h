#pragma once
#include "game_reaction.h"
#include "lvgl.h"
namespace gameReaction {
void configure(uint32_t seed);
const State& state();
lv_obj_t* create_screen(void(*exit)());
}

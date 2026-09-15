#pragma once
#include "game_tilt.h"
#include "tilt_input.h"
#include "lvgl.h"
namespace gameTilt {
void configure(tiltInput::Read read,tiltInput::Active active);
const State&state();
lv_obj_t*create_screen(void(*exit)());
}

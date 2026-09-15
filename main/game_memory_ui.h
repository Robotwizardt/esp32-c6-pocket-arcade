#pragma once
#include "game_memory.h"
#include "lvgl.h"
namespace gameMemory {void configure(uint32_t seed);const State&state();lv_obj_t*create_screen(void(*exit)());}

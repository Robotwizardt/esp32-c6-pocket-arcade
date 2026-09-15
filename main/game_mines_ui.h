#pragma once
#include "game_mines.h"
#include "lvgl.h"
namespace gameMines {void configure(uint32_t seed);const State&state();lv_obj_t*create_screen(void(*exit)());}

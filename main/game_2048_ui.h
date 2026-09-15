#pragma once
#include "lvgl.h"
#include "game_2048.h"
namespace game2048 {
using Load = bool (*)(Archive &);
using Save = bool (*)(const Archive &);
using Exit = void (*)();
void configure(uint32_t seed, Load load, Save save);
lv_obj_t *create_screen(Exit exit);
}

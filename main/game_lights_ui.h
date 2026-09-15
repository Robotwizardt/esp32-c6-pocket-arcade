#pragma once

#include "game_lights.h"
#include "lvgl.h"

namespace gameLights {

using Exit = void (*)();

void configure(uint32_t seed);
const State &state();
lv_obj_t *create_screen(Exit exit);

} // namespace gameLights

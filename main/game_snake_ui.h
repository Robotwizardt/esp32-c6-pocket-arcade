#pragma once

#include "game_snake.h"
#include "lvgl.h"

namespace gameSnake {

using Exit = void (*)();

/* Configure the in-RAM session before creating the screen. */
void configure(uint32_t seed);

/* Read-only hooks are intentionally small so host UI tests can inspect state. */
const Game &game();
const State &state();

/* Create one self-contained LVGL screen. The caller owns navigation. */
lv_obj_t *create_screen(Exit exit);

} // namespace gameSnake

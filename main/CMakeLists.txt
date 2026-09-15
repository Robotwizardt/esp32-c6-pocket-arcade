idf_component_register(
    SRCS "main.cpp" "game_2048.cpp" "game_2048_ui.cpp" "game_2048_storage.cpp"
         "game_snake.cpp" "game_snake_ui.cpp"
         "game_stack.cpp" "game_stack_ui.cpp"
         "game_lights.cpp" "game_lights_ui.cpp"
         "game_tilt.cpp" "game_tilt_ui.cpp" "tilt_input.cpp" "imu_port.cpp"
         "game_reaction.cpp" "game_reaction_ui.cpp"
         "game_breakout.cpp" "game_breakout_ui.cpp"
         "game_mines.cpp" "game_mines_ui.cpp"
         "game_memory.cpp" "game_memory_ui.cpp"
         "fonts/lobby_zh_16.c" "fonts/lobby_zh_20.c" "fonts/lobby_zh_24.c" "fonts/lobby_zh_28.c"
    INCLUDE_DIRS "."
    REQUIRES
        app_bsp
        port_bsp
        pmicpower
        lvgl__lvgl
        esp_hw_support
        nvs_flash
        esp_driver_gpio
        esp_driver_i2c
        esp_timer)

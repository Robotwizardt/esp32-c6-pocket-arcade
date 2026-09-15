#pragma once
#include <cstdint>
using gpio_num_t=int;
constexpr int GPIO_NUM_9=9,GPIO_NUM_10=10,GPIO_MODE_INPUT=1,GPIO_PULLUP_ENABLE=1,GPIO_PULLDOWN_DISABLE=0,GPIO_INTR_DISABLE=0;
struct gpio_config_t {uint64_t pin_bit_mask;int mode,pull_up_en,pull_down_en,intr_type;};
inline int test_gpio_levels[11]={1,1,1,1,1,1,1,1,1,1,1};
inline int gpio_get_level(int pin){return test_gpio_levels[pin];}
inline int gpio_config(const gpio_config_t*){return 0;}

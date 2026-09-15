#pragma once
#include "tilt_input.h"
struct i2c_master_bus_t;
namespace imuPort {
bool initialize(i2c_master_bus_t*bus);
void active(bool enabled);
bool read(tiltInput::Sample& sample);
}

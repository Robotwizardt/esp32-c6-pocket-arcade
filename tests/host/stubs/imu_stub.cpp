#include "../../../main/imu_port.h"
#include "lvgl.h"
namespace imuPort {
static bool enabled;
bool initialize(i2c_master_bus_t*){return true;}
void active(bool value){enabled=value;}
bool read(tiltInput::Sample&s){s={{0,0,1},{0,0,0},lv_tick_get()};return enabled;}
}

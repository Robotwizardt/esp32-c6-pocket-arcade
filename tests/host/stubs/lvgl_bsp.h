#pragma once
struct i2c_master_bus_t;
class I2cMasterBus { public: I2cMasterBus(int,int,int) {} i2c_master_bus_t* Get_I2cBusHandle(){return nullptr;} };
class DisplayPort {
public:
    template<typename... Args> DisplayPort(Args...) {}
    void DisplayPort_TouchInit() {}
    void Set_Backlight(unsigned char value) { brightness = value; }
    unsigned char brightness = 0;
};
static void Lvgl_PortInit(DisplayPort &) {}
static int Lvgl_lock(int) { return 0; }
static void Lvgl_unlock() {}
#define ESP_OK 0

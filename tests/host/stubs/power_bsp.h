#pragma once
static void Custom_PmicPortInit(I2cMasterBus *, int) {}

struct BatteryStatus {bool valid=false;bool connected=false;bool charging=false;bool usb=false;int percent=-1;};
static BatteryStatus test_battery{true,true,true,true,76};
static BatteryStatus ReadBatteryStatus(){return test_battery;}

// QMI8658 register settings follow the Waveshare example and QST datasheet.
#include "imu_port.h"
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <atomic>
namespace imuPort {
namespace {
i2c_master_dev_handle_t device=nullptr;
std::atomic<bool> requested{false};
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
tiltInput::Sample latest;bool has_sample=false;
esp_err_t write_reg(uint8_t reg,uint8_t value){uint8_t bytes[]={reg,value};return i2c_master_transmit(device,bytes,2,20);}
esp_err_t read_reg(uint8_t reg,uint8_t*bytes,unsigned size){return i2c_master_transmit_receive(device,&reg,1,bytes,size,20);}
bool sample(tiltInput::Sample&s){
 uint8_t status=0,data[12];if(read_reg(0x2e,&status,1)!=ESP_OK||(status&3)!=3||read_reg(0x35,data,12)!=ESP_OK)return false;
 s=tiltInput::decode(data,(uint32_t)(esp_timer_get_time()/1000));return true;
}
void worker(void*){
 bool enabled=false;unsigned report_samples=0;
 for(;;){bool want=requested.load();if(want!=enabled){if(write_reg(0x08,want?3:0)==ESP_OK){enabled=want;report_samples=0;portENTER_CRITICAL(&mux);has_sample=false;portEXIT_CRITICAL(&mux);}}
  if(enabled){tiltInput::Sample s;if(sample(s)){portENTER_CRITICAL(&mux);latest=s;has_sample=true;portEXIT_CRITICAL(&mux);
    if(report_samples<500&&++report_samples%50==0)ESP_LOGI("imu","motion accel_g=(%.3f,%.3f,%.3f) gyro_rad_s=(%.3f,%.3f,%.3f)",s.accel.x,s.accel.y,s.accel.z,s.gyro.x,s.gyro.y,s.gyro.z);}}
  vTaskDelay(pdMS_TO_TICKS(20));
 }
}
}
bool initialize(i2c_master_bus_t*bus){
 if(device)return true;
 const uint8_t addresses[]={0x6b,0x6a};uint8_t address=0;for(uint8_t candidate:addresses)if(i2c_master_probe(bus,candidate,30)==ESP_OK){address=candidate;break;}
 if(!address){ESP_LOGW("imu","QMI8658 not found");return false;}
 i2c_device_config_t config={};config.dev_addr_length=I2C_ADDR_BIT_LEN_7;config.device_address=address;config.scl_speed_hz=400000;
 if(i2c_master_bus_add_device(bus,&config,&device)!=ESP_OK)return false;
 uint8_t who=0;bool ok=read_reg(0,&who,1)==ESP_OK&&who==5;
 // Auto-increment; +/-2g and +/-512dps, both 125Hz, accel+gyro enabled.
 ok=ok&&write_reg(0x08,0)==ESP_OK&&write_reg(0x02,0x60)==ESP_OK&&write_reg(0x03,0x06)==ESP_OK&&write_reg(0x04,0x46)==ESP_OK&&write_reg(0x08,3)==ESP_OK;
 if(!ok){ESP_LOGW("imu","QMI8658 setup failed (WHO_AM_I=%02x)",who);i2c_master_bus_rm_device(device);device=nullptr;return false;}
 tiltInput::Sample initial;bool sampled=false;
 for(unsigned i=0;i<40&&!sampled;++i){vTaskDelay(pdMS_TO_TICKS(20));sampled=sample(initial);}
 if(sampled)ESP_LOGI("imu","QMI8658 ready addr=0x%02x accel_g=(%.3f,%.3f,%.3f) gyro_rad_s=(%.3f,%.3f,%.3f)",address,initial.accel.x,initial.accel.y,initial.accel.z,initial.gyro.x,initial.gyro.y,initial.gyro.z);
 else {uint8_t status=0,c2=0,c3=0,c7=0;read_reg(0x2e,&status,1);read_reg(3,&c2,1);read_reg(4,&c3,1);read_reg(8,&c7,1);ESP_LOGW("imu","Awaiting samples status=%02x ctrl2=%02x ctrl3=%02x ctrl7=%02x",status,c2,c3,c7); }
 write_reg(0x08,0);
 if(xTaskCreate(worker,"arcade_imu",4096,nullptr,3,nullptr)!=pdPASS){i2c_master_bus_rm_device(device);device=nullptr;return false;}
 return true;
}
void active(bool enabled){requested.store(enabled);}
bool read(tiltInput::Sample&s){
 if(!device||!requested.load())return false;
 portENTER_CRITICAL(&mux);bool valid=has_sample;s=latest;portEXIT_CRITICAL(&mux);
 return valid && (uint32_t)(esp_timer_get_time()/1000)-s.millis<200;
}
}

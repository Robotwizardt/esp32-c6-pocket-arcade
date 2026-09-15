#pragma once
#include <cstdint>
namespace tiltInput {
struct Vec{float x=0,y=0,z=0;};
struct Sample{Vec accel,gyro;uint32_t millis=0;};
Sample decode(const uint8_t bytes[12], uint32_t millis);
struct Control{float x=0,y=0;};
class Filter{
 Vec gravity_{0,0,1},neutral_{0,0,1},bias_{},right_{},down_{};
 bool initialized_=false,right_set_=false,calibrated_=false;
public:
 void reset();bool update(const Sample&s,float dt);
 bool neutral(const Sample&average);bool right();bool down();
 bool calibrated()const{return calibrated_;}Control control()const;
 Vec gravity()const{return gravity_;}
};
using Read=bool(*)(Sample&);using Active=void(*)(bool);
}

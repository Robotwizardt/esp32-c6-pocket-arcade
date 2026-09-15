#include "../../main/tilt_input.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <limits>
using namespace tiltInput;
static void check(bool b){if(!b)std::abort();}
int main(){
 uint8_t raw[12]={0,0xe0,0,0x20,0,0x40,0x40,0,0xc0,0xff,0,0};auto decoded=decode(raw,123);check(decoded.accel.x==-0.5f&&decoded.accel.y==0.5f&&decoded.accel.z==1&&std::abs(decoded.gyro.x-0.0174533f)<0.00001f&&decoded.gyro.y<0&&decoded.millis==123);
 Filter f;Sample s{{0,0,1},{0,0,0},0};check(f.neutral(s)&&!f.right());
 auto feed=[&](Vec a){s.accel=a;s.gyro={};for(int i=0;i<150;++i)check(f.update(s,.02f));};
 feed({.30f,0,.953939f});check(f.right());check(!f.down());feed({0,.3f,.953939f});check(f.down()&&f.calibrated());
 feed({0,0,1});check(f.control().x==0&&f.control().y==0);
 feed({.3f,0,.953939f});check(f.control().x>.6f&&std::abs(f.control().y)<.02f);
 feed({0,-.3f,.953939f});check(f.control().y<-.6f&&std::abs(f.control().x)<.02f);
 feed({0,0,1});s.gyro={1,0,0};check(f.update(s,.02f)&&f.gravity().y>.01f); // Gyroscope contributes even before accel changes.
 s.accel.x=std::numeric_limits<float>::quiet_NaN();check(!f.update(s,.02f));s.accel={0,0,1};check(!f.neutral(s));
 f.reset();s.gyro={.02f,-.01f,0};check(f.neutral(s));for(int i=0;i<10000;++i)check(f.update(s,.02f));check(std::abs(f.gravity().x)<.001f&&std::abs(f.gravity().y)<.001f);
 std::puts("PASS: IMU signed decoding, units, complementary gyro/accel fusion, axis calibration, deadzone, bias and invalid samples");
}

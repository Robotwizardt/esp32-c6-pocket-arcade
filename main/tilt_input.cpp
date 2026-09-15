#include "tilt_input.h"
#include <algorithm>
#include <cmath>
namespace tiltInput {
static float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static Vec scale(Vec v,float s){return {v.x*s,v.y*s,v.z*s};}
static Vec add(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
static Vec cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static float norm(Vec v){return std::sqrt(dot(v,v));}
static bool finite(Vec v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
Sample decode(const uint8_t b[12],uint32_t millis){
 auto value=[&](unsigned i){return (int16_t)((uint16_t)b[i]|((uint16_t)b[i+1]<<8));};
 constexpr float scale=3.14159265358979323846f/(180.0f*64.0f);
 return {{value(0)/16384.0f,value(2)/16384.0f,value(4)/16384.0f},{value(6)*scale,value(8)*scale,value(10)*scale},millis};
}
void Filter::reset(){*this=Filter();}
bool Filter::update(const Sample&s,float dt){
 if(!finite(s.accel)||!finite(s.gyro)||!std::isfinite(dt))return false;
 float magnitude=norm(s.accel);if(magnitude<0.1f||magnitude>2.5f)return false;
 Vec measured=scale(s.accel,1/magnitude);
 if(!initialized_){gravity_=measured;initialized_=true;return true;}
 dt=std::clamp(dt,0.001f,0.05f);Vec omega=add(s.gyro,scale(bias_,-1));
 gravity_=add(gravity_,scale(cross(omega,gravity_),-dt));
 // Gyroscope predicts rotation; gravity corrects drift when acceleration is plausible.
 if(magnitude>0.8f&&magnitude<1.2f){float alpha=dt/(0.35f+dt);gravity_=add(scale(gravity_,1-alpha),scale(measured,alpha));}
 float length=norm(gravity_);if(length<0.1f)return false;gravity_=scale(gravity_,1/length);return true;
}
bool Filter::neutral(const Sample&s){
 float n=norm(s.accel);if(!finite(s.accel)||!finite(s.gyro)||n<0.85f||n>1.15f||norm(s.gyro)>0.50f)return false;
 neutral_=gravity_=scale(s.accel,1/n);bias_=s.gyro;initialized_=true;right_set_=calibrated_=false;return true;
}
bool Filter::right(){
 Vec tangent=add(gravity_,scale(neutral_,-dot(gravity_,neutral_)));float n=norm(tangent);
 if(!initialized_||n<0.15f||n>0.75f)return false;
 right_=scale(tangent,1/n);right_set_=true;calibrated_=false;return true;
}
bool Filter::down(){
 Vec tangent=add(gravity_,scale(neutral_,-dot(gravity_,neutral_)));float n=norm(tangent);
 if(!right_set_||n<0.15f||n>0.75f||std::abs(dot(tangent,right_))/n>0.55f)return false;
 tangent=add(tangent,scale(right_,-dot(tangent,right_)));down_=scale(tangent,1/norm(tangent));calibrated_=true;return true;
}
Control Filter::control()const{
 if(!calibrated_)return {};
 auto response=[](float v){float a=std::abs(v);if(a<0.025f)return 0.0f;return std::copysign(std::min(1.0f,(a-0.025f)/0.40f),v);};
 return {response(dot(gravity_,right_)),response(dot(gravity_,down_))};
}
}

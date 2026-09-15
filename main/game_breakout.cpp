#include "game_breakout.h"
#include <algorithm>
#include <cmath>
namespace gameBreakout {
void Game::serve(){s_.x=s_.paddle;s_.y=288;s_.vx=70;s_.vy=-130;s_.phase=Phase::Ready;}
void Game::restart(){s_={};s_.bricks.fill(true);serve();}
void Game::start(){if(s_.phase==Phase::Ready)s_.phase=Phase::Running;}
void Game::pause(){if(s_.phase==Phase::Running)s_.phase=Phase::Paused;}
void Game::resume(){if(s_.phase==Phase::Paused)s_.phase=Phase::Running;}
void Game::paddle(float x){if(s_.phase!=Phase::Ready&&s_.phase!=Phase::Running)return;s_.paddle=std::clamp(x,kPaddleWidth/2,kWidth-kPaddleWidth/2);if(s_.phase==Phase::Ready)s_.x=s_.paddle;}
void Game::step(float dt){
 float oldx=s_.x,oldy=s_.y;s_.x+=s_.vx*dt;s_.y+=s_.vy*dt;
 if(s_.x<kRadius){s_.x=kRadius;s_.vx=std::abs(s_.vx);}else if(s_.x>kWidth-kRadius){s_.x=kWidth-kRadius;s_.vx=-std::abs(s_.vx);}
 if(s_.y<kRadius){s_.y=kRadius;s_.vy=std::abs(s_.vy);}
 for(unsigned i=0;i<24;++i){if(!s_.bricks[i])continue;float left=8+(i%6)*60,top=28+(i/6)*28,right=left+52,bottom=top+18;
  float dx=s_.x-std::clamp(s_.x,left,right),dy=s_.y-std::clamp(s_.y,top,bottom);if(dx*dx+dy*dy>kRadius*kRadius)continue;
  s_.bricks[i]=false;++s_.score;
  if(oldy+kRadius<=top){s_.y=top-kRadius;s_.vy=-std::abs(s_.vy);}
  else if(oldy-kRadius>=bottom){s_.y=bottom+kRadius;s_.vy=std::abs(s_.vy);}
  else if(oldx+kRadius<=left){s_.x=left-kRadius;s_.vx=-std::abs(s_.vx);}
  else if(oldx-kRadius>=right){s_.x=right+kRadius;s_.vx=std::abs(s_.vx);}
  else{s_.vy=-s_.vy;s_.y=oldy;}
  if(s_.score==24) s_.phase=Phase::Won;
  break;
 }
 if(s_.vy>0 && oldy+kRadius<=kPaddleY && s_.y+kRadius>=kPaddleY && s_.x>=s_.paddle-kPaddleWidth/2-kRadius && s_.x<=s_.paddle+kPaddleWidth/2+kRadius){
  float relative=std::clamp((s_.x-s_.paddle)/(kPaddleWidth/2),-1.0f,1.0f);float speed=150+s_.score*2;
  s_.vx=relative*120;s_.vy=-std::sqrt(speed*speed-s_.vx*s_.vx);s_.y=kPaddleY-kRadius;
 }
 if(s_.y-kRadius>kHeight){if(--s_.lives==0)s_.phase=Phase::Dead;else serve();}
}
void Game::advance(uint32_t ms){
 ms=std::min<uint32_t>(ms,100u);while(ms&&s_.phase==Phase::Running){unsigned slice=std::min<uint32_t>(ms,4u);step(slice/1000.0f);ms-=slice;}
}
}


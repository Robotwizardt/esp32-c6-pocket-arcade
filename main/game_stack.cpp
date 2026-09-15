#include "game_stack.h"
#include <algorithm>
#include <cmath>
namespace gameStack {
void Game::spawn(){
 s_.moving.width=s_.tower[s_.count-1].width;
 float speed=std::min(210.0f,80.0f+s_.score*5.0f);
 s_.moving.x=s_.score%2?kBoardWidth-s_.moving.width:0;
 s_.velocity=s_.score%2?-speed:speed;
}
void Game::restart(){s_={};s_.tower[0]={76,216};spawn();}
void Game::start(){if(s_.phase==Phase::Ready)s_.phase=Phase::Running;}
void Game::pause(){if(s_.phase==Phase::Running)s_.phase=Phase::Paused;}
void Game::resume(){if(s_.phase==Phase::Paused)s_.phase=Phase::Running;}
void Game::advance(uint32_t ms){
 if(s_.phase!=Phase::Running)return;
 const float span=kBoardWidth-s_.moving.width;
 if(span<=0)return;
 float travel=(s_.velocity>0?s_.moving.x:2*span-s_.moving.x)+std::abs(s_.velocity)*(ms/1000.0f);
 travel=std::fmod(travel,2*span);
 s_.moving.x=travel<=span?travel:2*span-travel;
 s_.velocity=(travel<span?1:-1)*std::abs(s_.velocity);
}
bool Game::drop(){
 if(s_.phase!=Phase::Running)return false;
 const Block base=s_.tower[s_.count-1];
 s_.perfect=std::abs(s_.moving.x-base.x)<=3.0f;
 if(s_.perfect)s_.moving.x=base.x;
 float left=std::max(base.x,s_.moving.x);
 float width=std::min(base.x+base.width,s_.moving.x+s_.moving.width)-left;
 if(width<1.0f){s_.phase=Phase::Dead;return false;}
 if(s_.count==s_.tower.size()){
  for(unsigned i=1;i<s_.count;++i)s_.tower[i-1]=s_.tower[i];
  --s_.count;
 }
 s_.tower[s_.count++]={left,width};++s_.score;spawn();return true;
}
}

#include "game_reaction.h"
namespace gameReaction {
uint32_t Game::random(){random_^=random_<<13;random_^=random_>>17;random_^=random_<<5;return random_;}
void Game::restart(){s_={};since_=delay_=0;}
void Game::cancel_attempt(){if(s_.phase==Phase::Waiting||s_.phase==Phase::Go)s_.phase=Phase::Canceled;}
void Game::press(uint32_t now){
 switch(s_.phase){
 case Phase::Waiting:s_.phase=Phase::Early;break;
 case Phase::Go:
  s_.last=now-since_;
  if(s_.last>=10000){s_.phase=Phase::Timeout;break;}
  s_.times[s_.rounds++]=s_.last;
  if(s_.rounds==s_.times.size()){
   uint32_t sum=0;for(auto t:s_.times)sum+=t;s_.average=sum/s_.rounds;s_.phase=Phase::Summary;
  }else s_.phase=Phase::Result;
  break;
 case Phase::Summary:restart();[[fallthrough]];
 case Phase::Idle:case Phase::Canceled:case Phase::Early:case Phase::Result:case Phase::Timeout:
  since_=now;delay_=1500+random()%3001;s_.phase=Phase::Waiting;break;
 }
}
void Game::advance(uint32_t now){
 if(s_.phase==Phase::Waiting && now-since_>=delay_){s_.phase=Phase::Go;since_=now;}
 else if(s_.phase==Phase::Go && now-since_>=10000)s_.phase=Phase::Timeout;
}
}

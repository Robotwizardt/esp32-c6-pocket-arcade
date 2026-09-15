#include "game_memory.h"
#include <algorithm>
namespace gameMemory {
uint32_t Game::random(){rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;return rng_;}
void Game::restart(){s_={};elapsed_=0;for(unsigned i=0;i<16;++i)s_.cards[i]=i/2+1;for(unsigned i=15;i>0;--i)std::swap(s_.cards[i],s_.cards[random()%(i+1)]);}
bool Game::reveal(unsigned i){
 if(i>=16||s_.phase!=Phase::Playing||s_.matched[i]||(int)i==s_.first)return false;
 if(s_.first<0){s_.first=i;return true;}
 s_.second=i;++s_.moves;
 if(s_.cards[s_.first]==s_.cards[i]){s_.matched[s_.first]=s_.matched[i]=true;++s_.pairs;s_.first=s_.second=-1;if(s_.pairs==8)s_.phase=Phase::Won;}
 else{s_.phase=Phase::Resolving;elapsed_=0;}return true;
}
void Game::advance(uint32_t ms){if(s_.phase!=Phase::Resolving)return;if(ms>=700-elapsed_){s_.first=s_.second=-1;s_.phase=Phase::Playing;elapsed_=0;}else elapsed_+=ms;}
}

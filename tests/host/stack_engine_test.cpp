#include "../../main/game_stack.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace gameStack;
static void check(bool b){if(!b)std::abort();}
int main(){
 Game g;g.advance(1000);check(g.state().moving.x==0);g.start();g.advance(1000);check(std::abs(g.state().moving.x-80)<0.01f);
 check(g.drop()&&g.state().score==1&&g.state().moving.width==212);
 g.pause();float x=g.state().moving.x;g.advance(9999);check(g.state().moving.x==x&&!g.drop());g.resume();
 for(int i=0;i<10000;++i){g.advance(37);check(g.state().moving.x>=0&&g.state().moving.x+g.state().moving.width<=368.001f);}
 g.restart();g.start();auto&s=const_cast<State&>(g.state());
 for(int i=0;i<1000;++i){s.moving.x=s.tower[s.count-1].x+2;check(g.drop()&&s.perfect&&s.moving.width==216&&s.count<=12);}
 check(s.score==1000);s.moving={0,10};s.tower[s.count-1]={100,10};check(!g.drop()&&s.phase==Phase::Dead);
 g.restart();check(g.state().phase==Phase::Ready&&g.state().score==0);
 std::puts("PASS: stack timing, overlap trim, pause, bounce bounds, perfect snap, 1000 floors, miss and restart");
}

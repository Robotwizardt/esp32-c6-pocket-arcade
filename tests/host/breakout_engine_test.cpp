#include "../../main/game_breakout.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace gameBreakout;
static void check(bool b){if(!b)std::abort();}
int main(){Game g;g.paddle(-9);check(g.state().paddle==48&&g.state().x==48);g.paddle(999);check(g.state().paddle==320);g.start();g.pause();float x=g.state().x;g.advance(100);check(g.state().x==x);g.resume();
 auto&s=const_cast<State&>(g.state());s.x=34;s.y=21;s.vx=0;s.vy=150;g.advance(20);check(!s.bricks[0]&&s.score==1&&s.vy<0);
 s.x=2;s.y=200;s.vx=-150;s.vy=0;g.advance(4);check(s.x>=5&&s.vx>0);
 s.x=s.paddle;s.y=294;s.vx=0;s.vy=150;g.advance(20);check(s.vy<0&&s.y<=295);
 for(unsigned life=2;;--life){s.y=333;s.vy=150;s.phase=Phase::Running;g.advance(4);check(s.lives==life);if(!life){check(s.phase==Phase::Dead);break;}check(s.phase==Phase::Ready);g.start();}
 g.restart();s.bricks.fill(false);s.bricks[0]=true;s.score=23;s.x=34;s.y=21;s.vx=0;s.vy=150;g.start();g.advance(20);check(s.phase==Phase::Won&&s.score==24);
 g.restart();g.start();for(unsigned i=0;i<50000;++i){if(s.phase==Phase::Ready)g.start();if(s.phase==Phase::Won||s.phase==Phase::Dead){g.restart();g.start();}g.paddle(s.x);g.advance(16);check(std::isfinite(s.x)&&std::isfinite(s.y)&&s.score<=24&&s.lives<=3&&s.x>=5&&s.x<=363);}
 std::puts("PASS: breakout bricks, walls, paddle reflection, life/serve, victory, pause, 50000 physics frames");}

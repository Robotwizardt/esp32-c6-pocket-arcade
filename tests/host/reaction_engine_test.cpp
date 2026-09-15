#include "../../main/game_reaction.h"
#include <cstdio>
#include <cstdlib>
using namespace gameReaction;
static void check(bool b){if(!b)std::abort();}
int main(){Game g(4);g.press(0);check(g.state().phase==Phase::Waiting);g.advance(1000);check(g.state().phase==Phase::Waiting);g.press(1000);check(g.state().phase==Phase::Early&&g.state().rounds==0);
 uint32_t now=UINT32_MAX-1000;for(unsigned i=0;i<5;++i){g.press(now);g.advance(now+5000);check(g.state().phase==Phase::Go);g.press(now+5250);check(g.state().last==250&&g.state().rounds==i+1);now+=6000;}check(g.state().phase==Phase::Summary&&g.state().average==250);
 g.press(now);check(g.state().rounds==0&&g.state().phase==Phase::Waiting);g.cancel_attempt();check(g.state().phase==Phase::Canceled);g.press(now);g.advance(now+5000);g.advance(now+15000);check(g.state().phase==Phase::Timeout&&g.state().rounds==0);g.restart();check(g.state().phase==Phase::Idle);
 std::puts("PASS: reaction early press, wait window, five results, mean, timestamp wrap, cancel, timeout, restart");}

#include "../../main/game_memory.h"
#include <cstdio>
#include <cstdlib>
using namespace gameMemory;
static void check(bool b){if(!b)std::abort();}
int main(){
 for(unsigned seed=1;seed<=1000;++seed){Game g(seed);unsigned counts[9]={};for(auto c:g.state().cards){check(c>=1&&c<=8);counts[c]++;}for(int i=1;i<=8;++i)check(counts[i]==2);
  for(int value=1;value<=8;++value)for(unsigned i=0;i<16;++i)if(g.state().cards[i]==value)check(g.reveal(i));
  check(g.state().phase==Phase::Won&&g.state().moves==8&&g.state().pairs==8&&!g.reveal(0));
 }
 Game g(2);unsigned second=1;while(g.state().cards[0]==g.state().cards[second])++second;
 check(g.reveal(0)&&!g.reveal(0)&&!g.reveal(99));check(g.reveal(second)&&g.state().phase==Phase::Resolving&&!g.reveal((second+1)%16));g.advance(699);check(g.state().phase==Phase::Resolving);g.advance(1);check(g.state().first==-1&&g.state().second==-1&&g.state().moves==1);g.restart();check(g.state().moves==0);
 std::puts("PASS: memory 1000 shuffles and complete solves, mismatch 700ms lock, duplicate/invalid tap, restart");
}

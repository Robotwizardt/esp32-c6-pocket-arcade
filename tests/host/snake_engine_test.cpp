#include "../../main/game_snake.h"
#include <cstdio>
#include <cstdlib>
using namespace gameSnake;
static void check(bool ok,const char *why) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(2);}}
int main() {
 Game g(123);check(g.phase()==Phase::Ready && g.speed_ms()==300,"slow ready");
 check(g.advance(10000)==StepResult::None,"ready clock stopped");
 check(!g.set_direction(Direction::Left),"reverse start blocked");
 check(g.set_direction(Direction::Up) && !g.set_direction(Direction::Left),"opening turn locked");
 check(g.advance(299)==StepResult::None,"initial interval");
 check(g.advance(1)==StepResult::Moved && g.state().body[0]==Cell{8,7},"initial up movement");
 check(g.set_direction(Direction::Left) && !g.set_direction(Direction::Down),"one queued turn");
 g.tick();check(g.state().body[0]==Cell{7,7},"queued turn applied");
 check(!g.set_direction(Direction::Right),"reverse blocked");
 check(g.pause(),"pause");auto steps=g.state().steps;g.advance(5000);check(g.state().steps==steps,"pause time ignored");
 check(!g.set_direction(Direction::Up) && g.resume(),"pause locks inputs");
 g.restart();auto &s=const_cast<State&>(g.state());s.food={9,8};g.set_direction(Direction::Right);
 check(g.tick()==StepResult::Ate && g.length()==4 && g.score()==1 && g.speed_ms()==290,"food growth and speed");
 g.restart();s.body[0]={1,1};s.body[1]={1,2};s.body[2]={0,2};s.body[3]={0,1};s.length=4;s.direction=Direction::Up;s.food={10,10};
 g.set_direction(Direction::Left);check(g.tick()==StepResult::Moved && s.body[0]==Cell{0,1},"vacating tail legal");
 g.restart();s.body[0]={1,1};s.body[1]={1,2};s.body[2]={0,2};s.body[3]={0,1};s.body[4]={0,0};s.length=5;s.direction=Direction::Up;s.food={10,10};
 g.set_direction(Direction::Left);check(g.tick()==StepResult::Died,"body collision");
 g.restart();g.set_direction(Direction::Up);for(int i=0;i<9;++i)g.tick();check(g.phase()==Phase::Dead,"upper wall collision");
 check(g.tick()==StepResult::None,"terminal stops ticks");g.restart();check(g.phase()==Phase::Ready && g.score()==0,"restart resets");
 std::puts("PASS: slow start, timing, queued turn, reverse lock, pause, food, tail, body/wall collision, restart");
}

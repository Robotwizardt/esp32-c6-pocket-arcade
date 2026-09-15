#include "../../main/game_snake.h"
#include <cstdio>
#include <cstdlib>
using namespace gameSnake;
static void check(bool ok) { if(!ok) std::abort(); }
int main() {
    // Follow a Hamiltonian cycle. Every free food cell is eventually visited;
    // this exercises real food placement and growth all the way to a full board.
    Game game(77);
    auto &s = const_cast<State&>(game.state());
    Cell path[256]; unsigned n=0;
    for(unsigned x=0;x<16;++x) path[n++]={static_cast<uint8_t>(x),0};
    for(unsigned y=1;y<16;++y) {
        if(y%2) for(int x=15;x>=1;--x) path[n++]={static_cast<uint8_t>(x),static_cast<uint8_t>(y)};
        else for(unsigned x=1;x<16;++x) path[n++]={static_cast<uint8_t>(x),static_cast<uint8_t>(y)};
    }
    for(int y=15;y>=1;--y) path[n++]={0,static_cast<uint8_t>(y)};
    check(n==256);
    s.body[0]=path[2];s.body[1]=path[1];s.body[2]=path[0];s.food=path[3];
    check(game.set_direction(Direction::Right));
    unsigned index=2, ticks=0;
    while(game.phase()==Phase::Running && ticks<100000) {
        Cell a=path[index], b=path[(index+1)%256];
        Direction d=b.x>a.x?Direction::Right:b.x<a.x?Direction::Left:b.y>a.y?Direction::Down:Direction::Up;
        game.set_direction(d);auto r=game.tick();++ticks;index=(index+1)%256;
        check(r!=StepResult::Died && game.state().body[0]==b);
        check(game.length()<=256 && game.speed_ms()>=140);
        for(unsigned i=0;i<game.length();++i) {
            check(game.state().body[i].x<16 && game.state().body[i].y<16);
            for(unsigned j=0;j<i;++j) check(game.state().body[i]!=game.state().body[j]);
            if(game.phase()==Phase::Running) check(game.food()!=game.state().body[i]);
        }
    }
    check(game.phase()==Phase::Won && game.length()==256 && game.score()==253);
    std::printf("PASS: full-board growth, unique body, safe food, speed floor over %u ticks\n",ticks);
}

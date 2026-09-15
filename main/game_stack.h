#pragma once
#include <array>
#include <cstdint>
namespace gameStack {
constexpr float kBoardWidth=368;
enum class Phase {Ready, Running, Paused, Dead};
struct Block {float x=0,width=216;};
struct State {
 std::array<Block,12> tower{};
 unsigned count=1, score=0;
 Block moving{};
 float velocity=80;
 Phase phase=Phase::Ready;
 bool perfect=false;
};
class Game {
 State s_{};
 void spawn();
public:
 Game(){restart();}
 void restart();
 void start();
 void advance(uint32_t ms);
 bool drop();
 void pause();
 void resume();
 const State& state()const{return s_;}
};
}

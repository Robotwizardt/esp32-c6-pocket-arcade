#pragma once
#include <array>
#include <cstdint>
namespace gameReaction {
enum class Phase{Idle,Waiting,Go,Early,Canceled,Result,Timeout,Summary};
struct State{Phase phase=Phase::Idle;std::array<uint32_t,5> times{};unsigned rounds=0;uint32_t last=0,average=0;};
class Game{
 State s_{};uint32_t random_=1,since_=0,delay_=0;
 uint32_t random();
public:
 explicit Game(uint32_t seed=1):random_(seed?seed:1){}
 void restart();void cancel_attempt();void press(uint32_t now);void advance(uint32_t now);
 const State& state()const{return s_;}
};
}

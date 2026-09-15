#pragma once
#include <array>
#include <cstdint>
namespace gameMines {
enum class Phase{Ready,Playing,Won,Dead};
struct Cell{bool mine=false,revealed=false,flag=false;uint8_t adjacent=0;};
struct State{std::array<Cell,36> cells{};unsigned opened=0,flags=0;Phase phase=Phase::Ready;};
class Game{
 State s_{};uint32_t rng_=1;uint32_t random();void plant(unsigned safe);
public:
 explicit Game(uint32_t seed=1):rng_(seed?seed:1){}
 void restart(){s_={};}bool reveal(unsigned i);bool flag(unsigned i);const State&state()const{return s_;}
};
}

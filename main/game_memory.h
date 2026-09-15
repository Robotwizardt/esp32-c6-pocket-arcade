#pragma once
#include <array>
#include <cstdint>
namespace gameMemory {
enum class Phase{Playing,Resolving,Won};
struct State{std::array<uint8_t,16> cards{};std::array<bool,16> matched{};int first=-1,second=-1;unsigned moves=0,pairs=0;Phase phase=Phase::Playing;};
class Game{
 State s_{};uint32_t rng_=1,elapsed_=0;uint32_t random();
public:
 explicit Game(uint32_t seed=1):rng_(seed?seed:1){restart();}
 void restart();bool reveal(unsigned i);void advance(uint32_t ms);const State&state()const{return s_;}
};
}

#pragma once
#include <array>
#include <cstdint>
namespace gameBreakout {
constexpr float kWidth=368,kHeight=326,kRadius=5,kPaddleWidth=96,kPaddleY=300;
enum class Phase{Ready,Running,Paused,Won,Dead};
struct State{std::array<bool,24> bricks{};float x=184,y=288,vx=70,vy=-130,paddle=184;unsigned score=0,lives=3;Phase phase=Phase::Ready;};
class Game{
 State s_{};void serve();void step(float seconds);
public:
 Game(){restart();}void restart();void start();void paddle(float x);void advance(uint32_t ms);void pause();void resume();const State&state()const{return s_;}
};
}

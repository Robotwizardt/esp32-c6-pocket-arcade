#include "../../main/game_tilt.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace gameTilt;

namespace {

void check(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

float distance_to_goal(const Game &game) {
    const State &state = game.state();
    const Geometry &geometry = game.level();
    const float dx = state.x - geometry.goal.x;
    const float dy = state.y - geometry.goal.y;
    return std::sqrt(dx * dx + dy * dy);
}

void win_current_level(Game &game) {
    /* Goal detection is deliberately independent of speed. */
    State &state = const_cast<State &>(game.state());
    state.x = game.level().goal.x;
    state.y = game.level().goal.y;
    state.vx = 0.0f;
    state.vy = 0.0f;
    state.phase = Phase::Running;
    game.advance(4, 0.0f, 0.0f);
    check(game.state().phase == Phase::Won, "goal wins without a low-speed requirement");
}

void test_start_pause_resume_and_clock() {
    Game game;
    check(game.state().phase == Phase::Ready, "restart begins ready");
    check(game.state().level == 1, "restart begins level one");
    const float x = game.state().x;
    const float y = game.state().y;

    game.advance(500, 1.0f, 0.0f);
    check(game.state().x == x && game.state().y == y, "ready does not advance");
    game.start();
    check(game.state().phase == Phase::Running, "start enters running");
    game.advance(200, 1.0f, 0.0f);
    check(game.state().elapsed_ms == 200, "running advances elapsed clock");
    game.pause();
    check(game.state().phase == Phase::Paused, "pause enters paused");
    const State paused = game.state();
    game.advance(1000, -1.0f, 1.0f);
    check(game.state().x == paused.x && game.state().y == paused.y &&
              game.state().elapsed_ms == paused.elapsed_ms,
          "paused state is frozen");
    game.resume();
    check(game.state().phase == Phase::Running, "resume enters running");
}

void test_friction_speed_limit_and_boundary() {
    Game game;
    game.start();
    game.advance(3000, 1.0f, 0.0f);
    check(std::hypot(game.state().vx, game.state().vy) <= kMaxSpeed + 0.01f,
          "tilt speed is capped");
    check(game.state().x <= kWidth - kBallRadius + 0.01f,
          "right boundary keeps the whole ball on the board");

    const float before = game.state().x;
    game.advance(2000, 0.0f, 0.0f);
    check(game.state().x >= kBallRadius - 0.01f && game.state().x <= kWidth - kBallRadius + 0.01f,
          "boundary remains valid after friction");
    check(std::fabs(game.state().vx) < 0.1f, "friction settles velocity with no tilt");
    check(std::fabs(game.state().x - before) < 40.0f, "no tilt does not create motion");
}

void test_wall_collision() {
    Game game;
    game.start();
    State &state = const_cast<State &>(game.state());
    const Rect wall = game.level().arrayRects[0];

    state.x = wall.x - kBallRadius - 2.0f;
    state.y = wall.y + wall.h * 0.5f;
    state.vx = 140.0f;
    state.vy = 0.0f;
    game.advance(100, 0.0f, 0.0f);
    check(state.x <= wall.x - kBallRadius + 0.02f, "wall collision prevents penetration");
    check(state.vx <= 0.0f, "wall collision reflects outward velocity");
}

int shortest_path(const Geometry &g) {
    constexpr int nx=88,ny=77;
    std::array<int,nx*ny> distance{};std::array<bool,nx*ny> seen{};std::array<int,nx*ny> queue{};
    auto free=[&](int x,int y){
        float px=10+x*4,py=10+y*4;
        for(unsigned i=0;i<g.count;++i){auto r=g.arrayRects[i];float cx=std::fmax(r.x,std::fmin(px,r.x+r.w)),cy=std::fmax(r.y,std::fmin(py,r.y+r.h));
            if((px-cx)*(px-cx)+(py-cy)*(py-cy)<100.1f)return false;}
        return true;
    };
    int sx=std::lround((g.start.x-10)/4),sy=std::lround((g.start.y-10)/4);
    unsigned head=0,tail=0;queue[tail++]=sy*nx+sx;seen[sy*nx+sx]=true;
    while(head<tail){int id=queue[head++],x=id%nx,y=id/nx;
        if(std::hypot(10+x*4-g.goal.x,10+y*4-g.goal.y)<=14)return distance[id];
        const int dx[]={1,-1,0,0},dy[]={0,0,1,-1};
        for(int i=0;i<4;++i){int xx=x+dx[i],yy=y+dy[i];if(xx<0||xx>=nx||yy<0||yy>=ny)continue;int n=yy*nx+xx;if(!seen[n]&&free(xx,yy)){seen[n]=true;distance[n]=distance[id]+4;queue[tail++]=n;}}
    }
    return -1;
}

void test_levels_and_paths() {
    Game game;
    int previous_length=0;
    for (uint8_t level = 1; level <= kLevelCount; ++level) {
        const Geometry &geometry = game.level();
        check(game.state().level == level, "level view exposes current number");
        check(geometry.number == level && geometry.count > 0 && geometry.count <= kMaxRects,
              "level has fixed geometry");
        check(geometry.start.x >= kBallRadius && geometry.start.x <= kWidth - kBallRadius &&
                  geometry.start.y >= kBallRadius && geometry.start.y <= kHeight - kBallRadius,
              "level start is visible on board");
        check(geometry.goal.x >= kBallRadius && geometry.goal.x <= kWidth - kBallRadius &&
                  geometry.goal.y >= kBallRadius && geometry.goal.y <= kHeight - kBallRadius,
              "level goal is visible on board");

        int length=shortest_path(geometry);
        check(length>0, "maze has a route for the whole ball");
        const int minimum[]={800,1100,1450};
        check(length>=minimum[level-1] && length>previous_length, "maze requires a progressively longer route without perimeter shortcut");
        previous_length=length;
        std::printf("Level %u shortest safe route: %d pixels\n",level,length);
        win_current_level(game);
        if (level < kLevelCount) {
            check(game.next_level(), "won level advances");
            check(game.state().phase == Phase::Ready && game.state().level == level + 1,
                  "next level is ready");
            check(game.state().elapsed_ms == 0, "next level resets elapsed time");
        } else {
            check(game.next_level(), "last level completes");
            check(game.state().phase == Phase::Completed, "three levels complete the game");
        }
    }
    game.restart();
    check(game.state().phase == Phase::Ready && game.state().level == 1,
          "restart replays from level one");
}

} // namespace

int main() {
    test_start_pause_resume_and_clock();
    test_friction_speed_limit_and_boundary();
    test_wall_collision();
    test_levels_and_paths();
    std::puts("PASS: tilt engine state machine, friction, speed cap, boundaries, walls, goals, and three levels");
}


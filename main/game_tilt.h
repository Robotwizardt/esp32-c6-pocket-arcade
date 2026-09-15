#pragma once

#include <array>
#include <cstdint>

namespace gameTilt {

constexpr float kWidth = 368.0f;
constexpr float kHeight = 326.0f;
constexpr float kBallRadius = 10.0f;
constexpr float kGoalRadius = 14.0f;
constexpr float kMaxSpeed = 140.0f;
constexpr uint8_t kLevelCount = 3;
constexpr uint8_t kMaxRects = 8;

struct Point {
    float x = 0.0f;
    float y = 0.0f;
};

/* Axis-aligned solid wall, expressed as a top-left point and size. */
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    constexpr float left() const { return x; }
    constexpr float top() const { return y; }
    constexpr float right() const { return x + w; }
    constexpr float bottom() const { return y + h; }
};

enum class Phase : uint8_t {
    Ready = 0,
    Running,
    Paused,
    Won,
    Completed,
};

/* Read-only wall geometry and endpoints for one maze. */
struct Geometry {
    std::array<Rect, kMaxRects> arrayRects{};
    uint8_t count = 0;
    Point start{};
    Point goal{};
    uint8_t number = 1;

};

struct State {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    uint32_t elapsed_ms = 0;
    uint8_t level = 1;
    Phase phase = Phase::Ready;
};

class Game {
public:
    Game() { restart(); }

    /* Restore level one and wait for an explicit start(). */
    void restart();

    /* Ready -> Running.  Other phases are left unchanged. */
    void start();
    void pause();
    void resume();

    /* Advance while running, using a normalized tilt in [-1, 1] per axis. */
    void advance(uint32_t ms, float tiltx, float tilty);

    /* Won -> next level, or Won on level three -> Completed. */
    bool next_level();

    const State &state() const { return state_; }

    /* Read-only current level geometry. */
    const Geometry &level() const { return geometry_; }

private:
    State state_{};
    Geometry geometry_{};

    void load_level(uint8_t level_number);
    void step(float seconds, float tiltx, float tilty);
    void resolve_boundaries();
    bool resolve_wall(const Rect &wall);
    bool at_goal() const;
};

} // namespace gameTilt


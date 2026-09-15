#include "game_tilt.h"

#include <algorithm>
#include <cmath>

namespace gameTilt {
namespace {

constexpr float kAcceleration = 900.0f;
constexpr float kFriction = 4.0f;
constexpr float kCollisionEpsilon = 0.001f;
constexpr uint32_t kStepMs = 4;

struct LevelDefinition {
    Point start;
    Point goal;
    std::array<Rect, kMaxRects> rects;
    uint8_t count;
};

/*
 * Three levels use alternating top/bottom openings between vertical walls.
 * Each wall reaches a board edge, so the ball cannot route around the maze.
 */
constexpr LevelDefinition kLevels[kLevelCount] = {
    {
        {28.0f, 28.0f},
        {330.0f, 284.0f},
        {{{88.0f, 0.0f, 12.0f, 254.0f},
          {180.0f, 72.0f, 12.0f, 254.0f},
          {272.0f, 0.0f, 12.0f, 254.0f},
          {},
          {},
          {},
          {},
          {}}},
        3,
    },
    {
        {28.0f, 298.0f},
        {330.0f, 32.0f},
        {{{68.0f, 52.0f, 12.0f, 274.0f},
          {140.0f, 0.0f, 12.0f, 274.0f},
          {212.0f, 52.0f, 12.0f, 274.0f},
          {284.0f, 0.0f, 12.0f, 274.0f},
          {},
          {},
          {},
          {}}},
        4,
    },
    {
        {28.0f, 28.0f},
        {335.0f, 294.0f},
        {{{52.0f, 0.0f, 12.0f, 290.0f},
          {112.0f, 36.0f, 12.0f, 290.0f},
          {172.0f, 0.0f, 12.0f, 290.0f},
          {232.0f, 36.0f, 12.0f, 290.0f},
          {292.0f, 0.0f, 12.0f, 290.0f},
          {},
          {},
          {}}},
        5,
    },
};

float clamp_tilt(float value) {
    if (!std::isfinite(value)) {
        return 0.0f;
    }
    return std::clamp(value, -1.0f, 1.0f);
}

float clamp_coordinate(float value, float low, float high) {
    return std::max(low, std::min(value, high));
}

void saturating_add(uint32_t &value, uint32_t amount) {
    if (UINT32_MAX - value < amount) {
        value = UINT32_MAX;
    } else {
        value += amount;
    }
}

} // namespace

void Game::restart() {
    load_level(1);
}

void Game::load_level(uint8_t level_number) {
    if (level_number < 1) {
        level_number = 1;
    }
    if (level_number > kLevelCount) {
        level_number = kLevelCount;
    }

    const LevelDefinition &definition = kLevels[level_number - 1];
    geometry_.arrayRects = definition.rects;
    geometry_.count = definition.count;
    geometry_.start = definition.start;
    geometry_.goal = definition.goal;
    geometry_.number = level_number;

    state_ = {};
    state_.x = definition.start.x;
    state_.y = definition.start.y;
    state_.level = level_number;
    state_.phase = Phase::Ready;
}

void Game::start() {
    if (state_.phase == Phase::Ready) {
        state_.phase = Phase::Running;
    }
}

void Game::pause() {
    if (state_.phase == Phase::Running) {
        state_.phase = Phase::Paused;
        state_.vx = state_.vy = 0.0f;
    }
}

void Game::resume() {
    if (state_.phase == Phase::Paused) {
        state_.phase = Phase::Running;
    }
}

void Game::advance(uint32_t ms, float tiltx, float tilty) {
    if (state_.phase != Phase::Running || ms == 0) {
        return;
    }

    const float x_tilt = clamp_tilt(tiltx);
    const float y_tilt = clamp_tilt(tilty);
    uint32_t remaining = ms;
    while (remaining != 0 && state_.phase == Phase::Running) {
        const uint32_t slice = std::min(remaining, kStepMs);
        step(static_cast<float>(slice) / 1000.0f, x_tilt, y_tilt);
        saturating_add(state_.elapsed_ms, slice);
        remaining -= slice;
    }
}

void Game::step(float seconds, float tiltx, float tilty) {
    state_.vx += tiltx * kAcceleration * seconds;
    state_.vy += tilty * kAcceleration * seconds;

    /* Linear drag is stable for a zero tilt and does not require a filter. */
    const float drag = std::max(0.0f, 1.0f - kFriction * seconds);
    state_.vx *= drag;
    state_.vy *= drag;

    const float speed_squared = state_.vx * state_.vx + state_.vy * state_.vy;
    if (speed_squared > kMaxSpeed * kMaxSpeed) {
        const float scale = kMaxSpeed / std::sqrt(speed_squared);
        state_.vx *= scale;
        state_.vy *= scale;
    }

    state_.x += state_.vx * seconds;
    state_.y += state_.vy * seconds;

    /* A 4 ms step at the speed cap moves less than one pixel. */
    resolve_boundaries();
    for (unsigned iteration = 0; iteration < 4; ++iteration) {
        bool hit_wall = false;
        for (uint8_t i = 0; i < geometry_.count; ++i) {
            hit_wall = resolve_wall(geometry_.arrayRects[i]) || hit_wall;
        }
        resolve_boundaries();
        if (!hit_wall) {
            break;
        }
    }

    if (at_goal()) {
        state_.phase = Phase::Won;
        state_.vx = 0.0f;
        state_.vy = 0.0f;
    }
}

void Game::resolve_boundaries() {
    const float min_x = kBallRadius;
    const float max_x = kWidth - kBallRadius;
    const float min_y = kBallRadius;
    const float max_y = kHeight - kBallRadius;

    if (state_.x < min_x) {
        state_.x = min_x;
        if (state_.vx < 0.0f) {
            state_.vx = -state_.vx;
        }
    } else if (state_.x > max_x) {
        state_.x = max_x;
        if (state_.vx > 0.0f) {
            state_.vx = -state_.vx;
        }
    }

    if (state_.y < min_y) {
        state_.y = min_y;
        if (state_.vy < 0.0f) {
            state_.vy = -state_.vy;
        }
    } else if (state_.y > max_y) {
        state_.y = max_y;
        if (state_.vy > 0.0f) {
            state_.vy = -state_.vy;
        }
    }
}

bool Game::resolve_wall(const Rect &wall) {
    const float right = wall.right();
    const float bottom = wall.bottom();
    const float closest_x = clamp_coordinate(state_.x, wall.x, right);
    const float closest_y = clamp_coordinate(state_.y, wall.y, bottom);
    float delta_x = state_.x - closest_x;
    float delta_y = state_.y - closest_y;
    const float distance_squared = delta_x * delta_x + delta_y * delta_y;
    const float radius_squared = kBallRadius * kBallRadius;

    if (distance_squared >= radius_squared) {
        return false;
    }

    float normal_x = 0.0f;
    float normal_y = 0.0f;
    float distance = 0.0f;
    if (distance_squared > 0.000001f) {
        distance = std::sqrt(distance_squared);
        normal_x = delta_x / distance;
        normal_y = delta_y / distance;
    } else {
        /* The center is in a wall; choose its shallowest escape direction. */
        const float to_left = state_.x - wall.x;
        const float to_right = right - state_.x;
        const float to_top = state_.y - wall.y;
        const float to_bottom = bottom - state_.y;
        const float shallowest = std::min(std::min(to_left, to_right), std::min(to_top, to_bottom));
        if (shallowest == to_left) {
            normal_x = -1.0f;
            distance = -to_left;
        } else if (shallowest == to_right) {
            normal_x = 1.0f;
            distance = -to_right;
        } else if (shallowest == to_top) {
            normal_y = -1.0f;
            distance = -to_top;
        } else {
            normal_y = 1.0f;
            distance = -to_bottom;
        }
    }

    const float correction = kBallRadius - distance + kCollisionEpsilon;
    state_.x += normal_x * correction;
    state_.y += normal_y * correction;

    const float normal_speed = state_.vx * normal_x + state_.vy * normal_y;
    if (normal_speed < 0.0f) {
        /* A modest bounce keeps a tilt against a wall responsive. */
        constexpr float restitution = 0.65f;
        state_.vx -= (1.0f + restitution) * normal_speed * normal_x;
        state_.vy -= (1.0f + restitution) * normal_speed * normal_y;
    }
    return true;
}

bool Game::at_goal() const {
    const float dx = state_.x - geometry_.goal.x;
    const float dy = state_.y - geometry_.goal.y;
    return dx * dx + dy * dy <= kGoalRadius * kGoalRadius;
}

bool Game::next_level() {
    if (state_.phase != Phase::Won) {
        return false;
    }
    if (state_.level < kLevelCount) {
        load_level(static_cast<uint8_t>(state_.level + 1));
    } else {
        state_.phase = Phase::Completed;
    }
    return true;
}

} // namespace gameTilt


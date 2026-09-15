#pragma once

#include <array>
#include <cstdint>

namespace gameSnake {

constexpr uint8_t kBoardWidth = 16;
constexpr uint8_t kBoardHeight = 16;
constexpr uint16_t kMaxLength = static_cast<uint16_t>(kBoardWidth) * kBoardHeight;
constexpr uint16_t kInitialSpeedMs = 300;
constexpr uint16_t kMinimumSpeedMs = 140;

struct Cell {
    uint8_t x = 0;
    uint8_t y = 0;

    constexpr bool operator==(const Cell &other) const { return x == other.x && y == other.y; }
    constexpr bool operator!=(const Cell &other) const { return !(*this == other); }
};

enum class Direction : uint8_t {
    None = 0,
    Up,
    Right,
    Down,
    Left,
};

enum class Phase : uint8_t {
    Ready = 0,
    Running,
    Paused,
    Dead,
    Won,
};

enum class StepResult : uint8_t {
    None = 0,
    Moved,
    Ate,
    Died,
    Won,
};

/*
 * A fixed-size snapshot keeps the game independent of LVGL and heap-free.
 * body[0] is always the head; only body[0..length) is valid.
 */
struct State {
    std::array<Cell, kMaxLength> body{};
    Cell food{0, 0};
    uint16_t length = 0;
    uint16_t score = 0;
    uint16_t speed_ms = kInitialSpeedMs;
    uint32_t steps = 0;
    Direction direction = Direction::Right;
    Direction queued_direction = Direction::None;
    Phase phase = Phase::Ready;
};

class Game {
public:
    explicit Game(uint32_t seed = 1);

    /* Reset the board while retaining the deterministic random stream. */
    void restart();

    /*
     * Queue one turn for the next tick. During Ready, the first legal swipe
     * starts the game. A reverse swipe is ignored, including before start.
     * At most one turn can be queued between ticks.
     */
    bool set_direction(Direction direction);
    bool request_direction(Direction direction) { return set_direction(direction); }

    /* Advance exactly one grid cell. Useful for deterministic host tests. */
    StepResult tick();

    /* Advance the clock; may perform more than one tick for a large delta. */
    StepResult advance(uint32_t elapsed_ms);

    bool pause();
    bool resume();
    bool toggle_pause();

    const State &state() const { return state_; }
    const State &snapshot() const { return state_; }
    uint32_t seed() const { return seed_; }
    uint16_t speed_ms() const { return state_.speed_ms; }
    uint16_t score() const { return state_.score; }
    uint16_t length() const { return state_.length; }
    Phase phase() const { return state_.phase; }
    Direction direction() const { return state_.direction; }
    Cell food() const { return state_.food; }

private:
    State state_{};
    uint32_t seed_ = 1;
    uint32_t random_state_ = 1;
    uint32_t elapsed_ms_ = 0;
    bool turn_queued_ = false;

    uint32_t random();
    void spawn_food();
    static bool is_opposite(Direction a, Direction b);
    static Cell next_cell(Cell cell, Direction direction);
};

} // namespace gameSnake

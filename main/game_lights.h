#pragma once

#include <array>
#include <cstdint>

namespace gameLights {

constexpr uint8_t kBoardSide = 5;
constexpr uint8_t kCellCount = kBoardSide * kBoardSide;
constexpr uint32_t kBoardMask = (1u << kCellCount) - 1u;

enum class Phase : uint8_t {
    Playing = 0,
    Won = 1,
    Solved = Won,
};

/* A complete, fixed-size snapshot of one puzzle. */
struct State {
    std::array<uint8_t, kCellCount> cells{};
    uint32_t board = 0;
    uint16_t on_count = 0;
    uint32_t steps = 0;
    uint32_t level = 1;
    Phase phase = Phase::Playing;
};

class Game {
public:
    explicit Game(uint32_t seed = 1);

    /* Restore the beginning of the current puzzle and clear its step count. */
    void reset();

    /* Flip a cell and its orthogonal neighbours. */
    bool click(uint8_t row, uint8_t column);
    bool click(uint8_t index);

    /* Move to a solved-only next level, or create a new puzzle at this level. */
    bool next_level();
    bool change_level();

    const State &state() const { return state_; }

private:
    State state_{};
    uint32_t initial_board_ = 0;
    uint32_t seed_ = 1;
    uint32_t random_state_ = 1;
    uint32_t random();
    void generate_level();
    void sync_state();
    static uint32_t flip_mask(uint8_t row, uint8_t column);
};

/* The UI-facing singleton is deliberately as small as the other games' API. */
void configure(uint32_t seed);
const State &state();

} // namespace gameLights

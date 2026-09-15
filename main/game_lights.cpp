#include "game_lights.h"

#include <algorithm>

namespace gameLights {

namespace {

constexpr uint8_t kNoCell = 0xFF;

} // namespace

Game::Game(uint32_t seed)
    : seed_(seed ? seed : 1), random_state_(seed ? seed : 1)
{
    state_.level = 1;
    generate_level();
}

uint32_t Game::random()
{
    /* xorshift32 has no heap/state outside this object and is deterministic. */
    if (random_state_ == 0) random_state_ = 1;
    uint32_t value = random_state_;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    random_state_ = value ? value : 1;
    return random_state_;
}

uint32_t Game::flip_mask(uint8_t row, uint8_t column)
{
    if (row >= kBoardSide || column >= kBoardSide) return 0;

    uint32_t mask = 1u << (static_cast<uint32_t>(row) * kBoardSide + column);
    if (row > 0) mask |= 1u << (static_cast<uint32_t>(row - 1) * kBoardSide + column);
    if (row + 1 < kBoardSide) mask |= 1u << (static_cast<uint32_t>(row + 1) * kBoardSide + column);
    if (column > 0) mask |= 1u << (static_cast<uint32_t>(row) * kBoardSide + column - 1);
    if (column + 1 < kBoardSide) mask |= 1u << (static_cast<uint32_t>(row) * kBoardSide + column + 1);
    return mask;
}

void Game::sync_state()
{
    state_.board = state_.board & kBoardMask;
    state_.on_count = 0;
    for (uint8_t index = 0; index < kCellCount; ++index) {
        const uint8_t on = static_cast<uint8_t>((state_.board >> index) & 1u);
        state_.cells[index] = on;
        state_.on_count = static_cast<uint16_t>(state_.on_count + on);
    }
}

void Game::generate_level()
{
    /* Every generated position starts at all-off and is reached only through
     * legal presses.  The reverse sequence therefore always solves it. */
    uint32_t board = 0;
    uint8_t previous = kNoCell;

    const uint32_t level_bias = std::min<uint32_t>(state_.level, 8u) * 2u;
    const uint32_t scramble_count = 10u + level_bias + random() % 10u;
    for (uint32_t move = 0; move < scramble_count; ++move) {
        uint8_t index = static_cast<uint8_t>(random() % kCellCount);
        if (index == previous) index = static_cast<uint8_t>((index + 1u) % kCellCount);
        previous = index;
        board ^= flip_mask(static_cast<uint8_t>(index / kBoardSide),
                           static_cast<uint8_t>(index % kBoardSide));
    }

    /* Consecutive random moves can cancel over GF(2).  Keep every puzzle
     * visibly non-empty, while preserving the legal-move construction. */
    if (board == 0) {
        const uint8_t index = static_cast<uint8_t>(random() % kCellCount);
        board ^= flip_mask(static_cast<uint8_t>(index / kBoardSide),
                           static_cast<uint8_t>(index % kBoardSide));
    }

    initial_board_ = board & kBoardMask;
    state_.board = initial_board_;
    state_.steps = 0;
    state_.phase = Phase::Playing;
    sync_state();
}

void Game::reset()
{
    state_.board = initial_board_;
    state_.steps = 0;
    state_.phase = Phase::Playing;
    sync_state();
}

bool Game::click(uint8_t row, uint8_t column)
{
    if (row >= kBoardSide || column >= kBoardSide) return false;
    return click(static_cast<uint8_t>(row * kBoardSide + column));
}

bool Game::click(uint8_t index)
{
    if (index >= kCellCount || state_.phase == Phase::Won) return false;
    const uint8_t row = static_cast<uint8_t>(index / kBoardSide);
    const uint8_t column = static_cast<uint8_t>(index % kBoardSide);
    state_.board ^= flip_mask(row, column);
    state_.steps++;
    state_.phase = state_.board == 0 ? Phase::Won : Phase::Playing;
    sync_state();
    return true;
}

bool Game::next_level()
{
    if (state_.phase != Phase::Won) return false;
    if (state_.level < UINT32_MAX) ++state_.level;
    generate_level();
    return true;
}

bool Game::change_level()
{
    generate_level();
    return true;
}

} // namespace gameLights

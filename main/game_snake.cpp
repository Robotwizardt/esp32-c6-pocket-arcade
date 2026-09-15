#include "game_snake.h"

#include <algorithm>

namespace gameSnake {

namespace {
constexpr uint8_t kInvalidCell = 0xFF;
}

Game::Game(uint32_t seed)
    : seed_(seed ? seed : 1), random_state_(seed ? seed : 1)
{
    restart();
}

uint32_t Game::random()
{
    /* Xorshift32 is small, deterministic, and has no library/runtime state. */
    uint32_t value = random_state_ ? random_state_ : 1;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    random_state_ = value ? value : 1;
    return random_state_;
}

bool Game::is_opposite(Direction a, Direction b)
{
    return (a == Direction::Up && b == Direction::Down) ||
           (a == Direction::Down && b == Direction::Up) ||
           (a == Direction::Left && b == Direction::Right) ||
           (a == Direction::Right && b == Direction::Left);
}

Cell Game::next_cell(Cell cell, Direction direction)
{
    switch(direction) {
    case Direction::Up:
        --cell.y;
        break;
    case Direction::Right:
        ++cell.x;
        break;
    case Direction::Down:
        ++cell.y;
        break;
    case Direction::Left:
        --cell.x;
        break;
    case Direction::None:
        break;
    }
    return cell;
}

void Game::restart()
{
    state_ = {};
    state_.body[0] = {8, 8};
    state_.body[1] = {7, 8};
    state_.body[2] = {6, 8};
    state_.length = 3;
    state_.direction = Direction::Right;
    state_.queued_direction = Direction::None;
    state_.phase = Phase::Ready;
    state_.speed_ms = kInitialSpeedMs;
    elapsed_ms_ = 0;
    turn_queued_ = false;
    spawn_food();
}

bool Game::set_direction(Direction direction)
{
    if(direction == Direction::None || state_.phase == Phase::Paused ||
       state_.phase == Phase::Dead || state_.phase == Phase::Won) {
        return false;
    }

    if(state_.phase == Phase::Ready) {
        /* The initial body points right. A left swipe cannot start it. */
        if(direction == Direction::Right ||
           (direction != Direction::None && !is_opposite(direction, state_.direction))) {
            state_.direction = direction;
            state_.queued_direction = direction;
            turn_queued_ = true;
            state_.phase = Phase::Running;
            return true;
        }
        return false;
    }

    if(state_.phase != Phase::Running || turn_queued_ || direction == state_.direction ||
       is_opposite(direction, state_.direction)) {
        return false;
    }

    state_.queued_direction = direction;
    turn_queued_ = true;
    return true;
}

void Game::spawn_food()
{
    std::array<Cell, kMaxLength> empty{};
    uint16_t count = 0;
    for(uint8_t y = 0; y < kBoardHeight; ++y) {
        for(uint8_t x = 0; x < kBoardWidth; ++x) {
            const Cell candidate{x, y};
            bool occupied = false;
            for(uint16_t index = 0; index < state_.length; ++index) {
                if(state_.body[index] == candidate) {
                    occupied = true;
                    break;
                }
            }
            if(!occupied && count < kMaxLength) empty[count++] = candidate;
        }
    }

    if(count == 0) {
        state_.food = {kInvalidCell, kInvalidCell};
        state_.phase = Phase::Won;
        return;
    }
    state_.food = empty[random() % count];
}

StepResult Game::tick()
{
    if(state_.phase != Phase::Running) return StepResult::None;

    if(turn_queued_) {
        state_.direction = state_.queued_direction;
        state_.queued_direction = Direction::None;
        turn_queued_ = false;
    }

    const Cell head = state_.body[0];
    const Cell next = next_cell(head, state_.direction);
    if(next.x >= kBoardWidth || next.y >= kBoardHeight) {
        state_.phase = Phase::Dead;
        return StepResult::Died;
    }

    const bool eating = next == state_.food;
    /* The tail leaves on a normal step, so entering its cell is legal. */
    const uint16_t collision_length = eating ? state_.length : static_cast<uint16_t>(state_.length - 1);
    for(uint16_t index = 0; index < collision_length; ++index) {
        if(state_.body[index] == next) {
            state_.phase = Phase::Dead;
            return StepResult::Died;
        }
    }

    const uint16_t new_length = static_cast<uint16_t>(state_.length + (eating ? 1 : 0));
    /* Shift from the tail toward the head without writing body[kMaxLength]. */
    for(uint16_t index = new_length; index > 1; --index) {
        state_.body[index - 1] = state_.body[index - 2];
    }
    state_.body[0] = next;
    state_.length = new_length;
    ++state_.steps;

    if(!eating) return StepResult::Moved;

    ++state_.score;
    state_.speed_ms = std::max<uint16_t>(kMinimumSpeedMs,
                                         static_cast<uint16_t>(state_.speed_ms - 10));
    if(state_.length == kMaxLength) {
        state_.food = {kInvalidCell, kInvalidCell};
        state_.phase = Phase::Won;
        return StepResult::Won;
    }
    spawn_food();
    return StepResult::Ate;
}

StepResult Game::advance(uint32_t elapsed_ms)
{
    if(state_.phase != Phase::Running) return StepResult::None;

    elapsed_ms_ += elapsed_ms;
    StepResult result = StepResult::None;
    while(state_.phase == Phase::Running && elapsed_ms_ >= state_.speed_ms) {
        elapsed_ms_ -= state_.speed_ms;
        result = tick();
    }
    return result;
}

bool Game::pause()
{
    if(state_.phase != Phase::Running) return false;
    state_.phase = Phase::Paused;
    return true;
}

bool Game::resume()
{
    if(state_.phase != Phase::Paused) return false;
    state_.phase = Phase::Running;
    return true;
}

bool Game::toggle_pause()
{
    return state_.phase == Phase::Running ? pause() : resume();
}

} // namespace gameSnake

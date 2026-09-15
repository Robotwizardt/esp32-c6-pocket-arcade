#include "game_2048.h"
#include <algorithm>
#include <limits>

namespace game2048 {
static constexpr uint32_t kMaxTile = 1u << 30;
static bool valid(const State &state) {
    unsigned occupied = 0;
    for(auto value : state.cells) {
        if(value == 0) continue;
        if(value < 2 || value > kMaxTile || (value & (value - 1))) return false;
        ++occupied;
    }
    return occupied != 0 && state.random != 0 && state.continued <= 1 && state.score % 4 == 0;
}
Game::Game(uint32_t seed) {
    data_.current.random = seed ? seed : 1;
    restart();
}
uint32_t Game::random() {
    auto &value = data_.current.random;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}
void Game::spawn() {
    std::array<unsigned, 16> empty{};
    unsigned count = 0;
    for(unsigned i = 0; i < 16; ++i) if(!data_.current.cells[i]) empty[count++] = i;
    spawned_ = -1;
    if(count == 0) return;
    spawned_ = static_cast<int>(empty[random() % count]);
    data_.current.cells[spawned_] = random() % 10 == 0 ? 4 : 2;
}
void Game::restart() {
    auto seed = data_.current.random;
    data_.current = {};
    data_.current.random = seed ? seed : 1;
    data_.previous = {};
    data_.undo_available = 0;
    spawn();
    spawn();
}
static unsigned index(Direction direction, unsigned line, unsigned offset) {
    switch(direction) {
    case Direction::Left: return line * 4 + offset;
    case Direction::Right: return line * 4 + 3 - offset;
    case Direction::Up: return offset * 4 + line;
    case Direction::Down: return (3 - offset) * 4 + line;
    }
    return 0;
}
bool Game::move(Direction direction) {
    if(win_pending()) return false;
    State before = data_.current;
    for(unsigned line = 0; line < 4; ++line) {
        std::array<uint32_t, 4> values{}, merged{};
        unsigned count = 0, output = 0;
        for(unsigned i = 0; i < 4; ++i) {
            auto value = before.cells[index(direction, line, i)];
            if(value) values[count++] = value;
        }
        for(unsigned i = 0; i < count; ++i) {
            auto value = values[i];
            if(i + 1 < count && value == values[i + 1] && value < kMaxTile) {
                value *= 2;
                ++i;
                uint64_t sum = uint64_t(data_.current.score) + value;
                data_.current.score = static_cast<uint32_t>(std::min<uint64_t>(sum, UINT32_MAX - 3u));
            }
            merged[output++] = value;
        }
        for(unsigned i = 0; i < 4; ++i) data_.current.cells[index(direction, line, i)] = merged[i];
    }
    if(data_.current.cells == before.cells) return false;
    data_.previous = before;
    data_.undo_available = 1;
    data_.best = std::max(data_.best, data_.current.score);
    spawn();
    return true;
}
bool Game::undo() {
    if(!data_.undo_available) return false;
    data_.current = data_.previous;
    data_.undo_available = 0;
    spawned_ = -1;
    return true;
}
bool Game::can_move() const {
    const auto &cells = data_.current.cells;
    for(unsigned i = 0; i < 16; ++i) {
        if(!cells[i]) return true;
        if(cells[i] >= kMaxTile) continue;
        if(i % 4 < 3 && cells[i] == cells[i + 1]) return true;
        if(i < 12 && cells[i] == cells[i + 4]) return true;
    }
    return false;
}
bool Game::win_pending() const {
    if(data_.current.continued) return false;
    for(auto value : data_.current.cells) if(value >= 2048) return true;
    return false;
}
bool Game::restore(const Archive &archive) {
    if(archive.version != 1 || !valid(archive.current) || archive.undo_available > 1 ||
       (archive.undo_available && !valid(archive.previous)) || archive.best < archive.current.score)
        return false;
    data_ = archive;
    spawned_ = -1;
    return true;
}
}

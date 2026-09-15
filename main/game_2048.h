#pragma once
#include <array>
#include <cstdint>

namespace game2048 {
enum class Direction { Left, Right, Up, Down };
struct State {
    std::array<uint32_t, 16> cells{};
    uint32_t score = 0;
    uint32_t random = 1;
    uint32_t continued = 0;
};
struct Archive {
    uint32_t version = 1;
    State current{};
    State previous{};
    uint32_t undo_available = 0;
    uint32_t best = 0;
};
class Game {
public:
    explicit Game(uint32_t seed = 1);
    void restart();
    bool move(Direction direction);
    bool undo();
    bool can_move() const;
    bool win_pending() const;
    void continue_playing() { data_.current.continued = 1; }
    const Archive &archive() const { return data_; }
    bool restore(const Archive &archive);
    int spawned_cell() const { return spawned_; }
private:
    Archive data_{};
    int spawned_ = -1;
    uint32_t random();
    void spawn();
};
}

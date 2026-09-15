#include "../../main/game_2048.h"
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <type_traits>
using namespace game2048;
static void check(bool value, const char *message) {
    if(!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(2); }
}
static Game position(std::array<uint32_t, 16> cells, uint32_t score = 0) {
    Game game(123);
    Archive a;
    a.current.cells = cells;
    a.current.score = a.best = score;
    a.current.random = 123;
    check(game.restore(a), "fixture restore");
    return game;
}
static void line_test(std::array<uint32_t,16> cells, Direction direction,
                      std::array<uint32_t,16> expected, uint32_t score) {
    auto game = position(cells);
    check(game.move(direction), "move must change board");
    auto result = game.archive().current.cells;
    check(game.spawned_cell() >= 0 && expected[game.spawned_cell()] == 0, "spawn in empty position");
    auto spawned = result[game.spawned_cell()];
    check(spawned == 2 || spawned == 4, "new tile is 2 or 4");
    result[game.spawned_cell()] = 0;
    check(result == expected, "merge result before spawn");
    check(game.archive().current.score == score, "merge score");
}
int main() {
    static_assert(std::is_trivially_copyable<Archive>::value, "archive must serialize as bytes");
    line_test({2,2,2,2}, Direction::Left, {4,4,0,0}, 8);
    line_test({2,2,4,0}, Direction::Left, {4,4,0,0}, 4);
    line_test({4,4,8,8}, Direction::Left, {8,16,0,0}, 24);
    line_test({2,0,2,2}, Direction::Left, {4,2,0,0}, 4);
    line_test({2,2,2,2}, Direction::Right, {0,0,4,4}, 8);
    line_test({2,0,0,0,2,0,0,0,2,0,0,0,2}, Direction::Up, {4,0,0,0,4}, 8);
    line_test({2,0,0,0,2,0,0,0,2,0,0,0,2}, Direction::Down, {0,0,0,0,0,0,0,0,4,0,0,0,4}, 8);
    auto idle = position({2});
    auto before = idle.archive();
    check(!idle.move(Direction::Left), "invalid move ignored");
    check(idle.archive().current.cells == before.current.cells &&
          idle.archive().current.random == before.current.random && !idle.archive().undo_available,
          "invalid move does not spawn, advance RNG or add undo");
    auto game = position({2,2});
    auto original = game.archive().current;
    check(game.move(Direction::Left), "merge");
    auto next = game.archive().current;
    check(game.undo(), "undo available");
    check(game.archive().current.cells == original.cells && game.archive().current.score == original.score,
          "undo restores board and score");
    check(game.archive().best == 4, "undo does not reduce record");
    check(!game.undo(), "only one undo");
    game.move(Direction::Left);
    check(game.archive().current.cells == next.cells, "undo preserves deterministic next spawn");
    auto won = position({1024,1024});
    won.move(Direction::Left);
    check(won.win_pending(), "2048 triggers victory");
    check(!won.move(Direction::Right), "victory awaits acknowledgement");
    won.continue_playing();
    check(!won.win_pending() && won.move(Direction::Right), "continue after 2048");
    auto over = position({2,4,2,4,4,2,4,2,2,4,2,4,4,2,4,2});
    check(!over.can_move(), "checkerboard is game over");
    for(auto d : {Direction::Left,Direction::Right,Direction::Up,Direction::Down}) check(!over.move(d), "no moves on full checkerboard");
    auto a = over.archive();
    a.current.cells[1] = 2;
    check(over.restore(a) && over.can_move(), "full board with matching neighbours remains playable");
    auto bad = game.archive(); bad.version = 2;
    check(!game.restore(bad), "reject unknown save version");
    bad = game.archive(); bad.current.cells[0] = 3;
    check(!game.restore(bad), "reject non-power-of-two save");
    bad = game.archive(); bad.current.random = 0;
    check(!game.restore(bad), "reject zero random seed");
    bad = game.archive(); bad.best = 0;
    check(!game.restore(bad), "reject inconsistent record");
    check(!game.restore(Archive{}), "reject empty save");
    auto saved = game.archive();
    Game loaded(999);
    check(loaded.restore(saved) && loaded.archive().current.cells == game.archive().current.cells && loaded.undo(),
          "archive round trip preserves game and undo");
    auto record = game.archive().best;
    game.restart();
    unsigned count = 0;
    for(auto n : game.archive().current.cells) if(n) { ++count; check(n==2 || n==4, "initial tile value"); }
    check(count == 2 && !game.archive().undo_available && game.archive().current.score == 0 && game.archive().best == record,
          "restart creates two tiles and preserves best");
    unsigned changed = 0;
    Game random_game(92819);
    for(unsigned i = 0; i < 2000; ++i) {
        if(!random_game.can_move()) random_game.restart();
        if(random_game.win_pending()) random_game.continue_playing();
        auto old = random_game.archive().current;
        if(random_game.move(static_cast<Direction>((i * 13 + i / 7) % 4))) {
            ++changed;
            const auto &current = random_game.archive().current;
            auto old_sum = std::accumulate(old.cells.begin(),old.cells.end(),uint64_t(0));
            auto sum = std::accumulate(current.cells.begin(),current.cells.end(),uint64_t(0));
            check(sum - old_sum == current.cells[random_game.spawned_cell()], "tile sum conserved except spawn");
            check(current.score >= old.score, "score monotonic");
        }
    }
    std::printf("PASS: 2048 rules, four directions, win/loss, undo, save validation, %u valid stress moves\n",changed);
}

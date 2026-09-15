#include "../../main/game_lights.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include <vector>

using namespace gameLights;

static void check(bool ok, const char *message)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(2);
    }
}

static uint32_t flip_mask(unsigned index)
{
    uint32_t mask = 1u << index;
    if (index % 5 != 0) mask |= 1u << (index - 1);
    if (index % 5 != 4) mask |= 1u << (index + 1);
    if (index >= 5) mask |= 1u << (index - 5);
    if (index < 20) mask |= 1u << (index + 5);
    return mask;
}

/* Enumerating the first row is enough to solve a 5x5 Lights Out board. */
static std::vector<unsigned> solve(uint32_t initial)
{
    for (unsigned first = 0; first < 32; ++first) {
        uint32_t board = initial;
        std::vector<unsigned> presses;
        for (unsigned column = 0; column < 5; ++column) {
            if (first & (1u << column)) {
                board ^= flip_mask(column);
                presses.push_back(column);
            }
        }
        for (unsigned index = 5; index < 25; ++index) {
            if (board & (1u << (index - 5))) {
                board ^= flip_mask(index);
                presses.push_back(index);
            }
        }
        if (board == 0) return presses;
    }
    return {};
}

int main()
{
    static_assert(std::is_trivially_copyable<State>::value, "state is a value snapshot");

    Game deterministic_a(42);
    Game deterministic_b(42);
    check(deterministic_a.state().board == deterministic_b.state().board,
          "same seed gives same first puzzle");
    check(deterministic_a.state().board != 0 && deterministic_a.state().steps == 0,
          "first puzzle is non-empty and untouched");

    const uint32_t initial = deterministic_a.state().board;
    check(!deterministic_a.next_level(), "next level requires a solved board");
    deterministic_a.click(0);
    check(deterministic_a.state().board == (initial ^ flip_mask(0)),
          "corner flips itself and two neighbours");
    deterministic_a.reset();
    check(deterministic_a.state().board == initial && deterministic_a.state().steps == 0,
          "reset restores the same puzzle");

    const unsigned edge = 2;
    deterministic_a.click(static_cast<uint8_t>(edge));
    check(deterministic_a.state().board == (initial ^ flip_mask(edge)),
          "top edge flips three cells");
    deterministic_a.reset();
    const unsigned center = 12;
    deterministic_a.click(static_cast<uint8_t>(center));
    check(deterministic_a.state().board == (initial ^ flip_mask(center)),
          "center flips five cells");

    unsigned solved_count = 0;
    for (uint32_t seed = 1; seed <= 1000; ++seed) {
        Game game(seed);
        const uint32_t board = game.state().board;
        check(board != 0 && game.state().on_count != 0, "generated board is non-empty");
        const auto solution = solve(board);
        check(!solution.empty(), "generated board is solvable");
        for (unsigned index : solution) {
            if (game.state().phase == Phase::Won) break;
            game.click(static_cast<uint8_t>(index));
        }
        check(game.state().phase == Phase::Won && game.state().board == 0,
              "solution reaches all-off board");
        check(!game.click(0), "solved board ignores extra clicks");
        const uint32_t old_level = game.state().level;
        check(game.next_level() && game.state().level == old_level + 1,
              "solved board advances one level");
        check(game.state().board != 0 && game.state().steps == 0,
              "next level starts non-empty and clears steps");
        ++solved_count;
    }

    Game changed(7);
    const auto old_level = changed.state().level;
    changed.change_level();
    check(changed.state().level == old_level && changed.state().steps == 0,
          "change level keeps level number and clears steps");
    check(changed.state().board != 0, "changed level is non-empty");
    std::printf("PASS: Lights Out rules, boundaries, reset, levels, %u solvable seeds\n",
                solved_count);
}

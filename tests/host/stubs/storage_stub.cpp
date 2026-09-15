#include "../../../main/game_2048_storage.h"
namespace game2048_storage {
static bool exists;
static game2048::Archive archive;
bool initialize() { return true; }
bool load(game2048::Archive &value) { value = archive; return exists; }
bool save(const game2048::Archive &value) { archive = value; exists = true; return true; }
}

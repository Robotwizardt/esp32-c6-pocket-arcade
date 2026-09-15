#pragma once

#include "game_2048.h"

namespace game2048_storage {

// Initializes the default NVS partition.  Failure is reported to the caller;
// this module never erases an existing partition as a recovery measure.
bool initialize();

bool load(game2048::Archive &archive);
bool save(const game2048::Archive &archive);

}  // namespace game2048_storage

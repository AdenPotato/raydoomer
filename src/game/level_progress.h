#pragma once

#include <vector>

namespace game {

/// How far through the game the player has got.
///
/// @remarks
/// Level identity is an index into the authored level sequence, not a name.
/// Renaming a level file then never invalidates a save.
struct LevelProgress {
    /// The level currently being played, or about to be entered.
    int currentLevel = 0;

    /// Levels finished, in completion order. A vector rather than a set: the
    /// order is information (it is the player's route), and a set would give
    /// the save an iteration order that depends on the container rather than on
    /// what happened.
    std::vector<int> completedLevels;

    friend bool operator==(const LevelProgress&, const LevelProgress&) = default;
};

} // namespace game

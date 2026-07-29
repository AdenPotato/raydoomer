#pragma once

#include "game/inventory.h"
#include "game/level_progress.h"
#include "game/player_state.h"

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace game {

/// The save schema version.
///
/// @remarks
/// **Bumping this obliges you to add a migration and its test in the same
/// change** (content_protocol.md). A schema change without a migration strands
/// every existing save.
inline constexpr int kCurrentSaveVersion = 1;

/// Why a save failed to load.
enum class SaveErrorKind {
    Malformed,          ///< Not parseable as JSON, or not a JSON object.
    MissingVersion,     ///< No version field. Unmigratable, so never accepted.
    UnsupportedVersion, ///< Newer than this build understands.
};

/// A typed failure from loading a save.
struct SaveError {
    SaveErrorKind kind;
    std::string message;
};

/// Everything that persists across a level boundary.
///
/// @remarks
/// Saves are written on level exit. Mid-level world state - enemy positions,
/// projectiles in flight, timers - is deliberately **not** here
/// (runtime_architecture.md). That keeps the schema small and stable, at the
/// accepted cost that dying mid-level loses that level's progress.
struct SaveData {
    int version = kCurrentSaveVersion;
    PlayerState player;
    Inventory inventory;
    LevelProgress progress;
    int currency = 0;

    /// The RNG seed, so a loaded run replays identically. Stored exactly: a
    /// narrowing conversion here would break reproducibility in a way that only
    /// surfaces as "loot feels different after loading".
    uint64_t rngSeed = 0;
};

using LoadResult = std::expected<SaveData, SaveError>;

/// Serializes a save to JSON.
std::string toJson(const SaveData& save);

/// Parses a save from JSON.
///
/// @returns The save, or a typed error naming what failed.
/// @note Never throws, for any input. A hand-edited save must fail validation
///       cleanly rather than corrupt the run.
LoadResult fromJson(std::string_view json);

} // namespace game

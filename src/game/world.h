#pragma once

#include "engine/physics.h"
#include "game/inventory.h"
#include "game/level_progress.h"
#include "game/player_state.h"
#include "game/save.h"

#include <cstdint>

namespace engine {
class EventDispatcher;
class Rng;
} // namespace engine

namespace game {

/// The root container for simulation state, and the owner of the data model.
///
/// @remarks
/// **Ownership is the point of this type.** Everything the run consists of lives
/// here, in one place, so there is exactly one answer to "where does this state
/// live" and no system holds its own copy:
///
///  - `PlayerState`   health, armor - what combat reads and writes
///  - `Inventory`     what the player is carrying
///  - `LevelProgress` where they are in the sequence
///  - `currency`      run-scoped, and persisted
///
/// What the world deliberately does **not** own:
///
///  - **Positions and velocities.** Box3D owns those; entities hold body
///    handles (runtime_architecture.md - transforms are physics-authoritative).
///  - **Content definitions.** Weapon stats and item affixes live in `data/` and
///    are loaded by the content layer. The world references them by id, so a
///    balance change never invalidates a save.
///  - **Presentation state.** Camera, animation, and interpolation are
///    downstream and must never feed back (presentation_protocol.md).
///
/// The world steps headless: it reads no clock, touches no device, and takes its
/// time, randomness, and event sink as parameters (the four seams).
class World {
public:
    /// Advances the simulation by one fixed tick.
    ///
    /// @param dt     Length of the tick in simulated seconds. Always the fixed
    ///               step, never a variable frame delta.
    /// @param rng    Seeded randomness. Systems receive it; they never fetch it.
    /// @param events Systems publish through this rather than calling each other.
    ///
    /// @note Steps physics exactly once, then advances the tick counter and
    ///       simulated clock. Gameplay systems arrive from ADE-12 onward and
    ///       are called from here in an explicit, fixed order
    ///       (runtime_architecture.md). The signature is the locked contract
    ///       they plug into.
    void tick(float dt, engine::Rng& rng, engine::EventDispatcher& events);

    /// The physics world. Owned here so it is stepped exactly once per
    /// simulation tick, structurally rather than by convention: a caller cannot
    /// step it per rendered frame because it does not reach it.
    engine::PhysicsWorld& physics() { return physics_; }
    const engine::PhysicsWorld& physics() const { return physics_; }

    PlayerState& player() { return player_; }
    const PlayerState& player() const { return player_; }

    Inventory& inventory() { return inventory_; }
    const Inventory& inventory() const { return inventory_; }

    LevelProgress& progress() { return progress_; }
    const LevelProgress& progress() const { return progress_; }

    int currency() const { return currency_; }
    void setCurrency(int value) { currency_ = value; }

    /// @returns How many ticks have been run. Simulated, not wall-clock.
    int tickCount() const { return tickCount_; }

    /// @returns Simulated seconds elapsed in this level.
    float elapsedSeconds() const { return elapsedSeconds_; }

    /// Projects the persistent subset of the world into a save.
    ///
    /// @param rngSeed The seed to resume from, so a loaded run replays
    ///                identically. Passed in rather than read from the world:
    ///                the RNG is owned by the caller, not by the world.
    /// @note Deliberately lossy. Tick count and elapsed time are level-scoped
    ///       and are not persisted; saves happen at level boundaries.
    SaveData toSave(uint64_t rngSeed) const;

    /// Restores the persistent state from a save.
    /// @note Resets level-scoped state (tick count, elapsed time). A loaded save
    ///       starts a level, it does not resume one mid-way.
    void restore(const SaveData& save);

private:
    engine::PhysicsWorld physics_;

    PlayerState player_;
    Inventory inventory_;
    LevelProgress progress_;
    int currency_ = 0;

    int tickCount_ = 0;
    float elapsedSeconds_ = 0.0f;
};

} // namespace game

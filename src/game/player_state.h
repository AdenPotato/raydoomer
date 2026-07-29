#pragma once

namespace game {

/// The player's persistent combat state.
///
/// @remarks
/// Deliberately plain data with no behaviour. Rules that read or change these
/// values live in systems, so the state can be constructed directly in a test
/// without a running game (gameplay_protocol.md).
///
/// Position is **not** here: Box3D owns position and velocity for anything
/// physical, and the player holds a body handle
/// (runtime_architecture.md - transforms are physics-authoritative). Nor is it
/// saved: saves happen at level boundaries, where the position is the next
/// level's spawn point rather than wherever the player was standing.
struct PlayerState {
    int health = 100;
    int maxHealth = 100;
    int armor = 0;

    friend bool operator==(const PlayerState&, const PlayerState&) = default;
};

} // namespace game

#pragma once

#include "engine/physics.h"
#include "game/input_snapshot.h"

namespace game {

/// Everything a designer would want to change about how the player moves.
///
/// @remarks
/// Every one of these is a tunable in `data/player.json`, never a literal in the
/// controller (`gameplay_protocol.md`). A numeric literal in movement code
/// cannot be tuned, reviewed, or diffed meaningfully.
///
/// The defaults here exist so a missing or partial data file still produces a
/// moving player rather than a frozen one; the real values live in data.
struct MovementTunables {
    /// Ground speed cap along the input direction, in m/s. Quake 3's 320 units
    /// per second. **Not a cap on total speed** - see `accelerate`.
    float maxSpeed = 8.13f;

    /// Ground acceleration coefficient. Dimensionless, as in Quake: the speed
    /// added per tick is `groundAccelerate * dt * wishSpeed`.
    float groundAccelerate = 10.0f;

    /// Air acceleration coefficient. **The strafe-jump dial.** Quake 3 ships 1;
    /// CPMA-style movement uses far more. Zero removes air control entirely.
    float airAccelerate = 1.0f;

    /// Speed cap along the wish direction **while airborne**, in m/s.
    ///
    /// @note Small on purpose - 30 Quake units, about 0.76 m/s. Because the cap
    ///       applies to the projection of velocity onto the input direction, a
    ///       sideways push while moving forward is barely capped at all, and
    ///       speed accumulates. That is strafe-jumping, and this value is what
    ///       makes it possible.
    float airWishSpeedCap = 0.76f;

    /// Friction coefficient. Dimensionless, as in Quake.
    float friction = 6.0f;

    /// Speed floor for the friction calculation, in m/s. Below this, drop is
    /// computed against this value rather than the actual speed, which is what
    /// makes stopping crisp instead of asymptotic.
    float stopSpeed = 2.54f;

    /// Upward metres per second applied on jump.
    float jumpImpulse = 6.5f;

    /// Radius of the player's collision shape, in metres.
    ///
    /// @note The ground probe starts at the body **centre**, so it must reach
    ///       past the shape before it can hit anything. One source for both the
    ///       spawned shape and the probe, so they cannot drift apart.
    float characterRadius = 0.5f;

    /// Camera height above the body centre, in metres. Below the top of the
    /// shape the view comes out of the player's chest.
    float eyeHeight = 0.4f;

    /// Vertical field of view in degrees. Feel value: wider reads as faster.
    float fieldOfView = 90.0f;

    /// Radians of look per pixel of mouse movement.
    float mouseSensitivity = 0.003f;

    /// How far **below the shape** still counts as standing on it, in metres.
    ///
    /// @note Without a tolerance the player is briefly airborne on any tick it
    ///       bounces a fraction of a millimetre off the floor, and air control
    ///       flickers on and off.
    float groundedTolerance = 0.15f;

    friend bool operator==(const MovementTunables&, const MovementTunables&) = default;
};

/// Parses tunables from JSON. Missing fields keep their default.
///
/// @remarks
/// Deliberately lenient about absence and strict about nothing: a partial file
/// is a designer mid-edit, not a corruption. A malformed file is reported by the
/// caller's loader.
MovementTunables movementTunablesFromJson(std::string_view json);

/// Advances the player one fixed tick.
///
/// @param physics The world the player's body lives in.
/// @param body    The player's body handle.
/// @param input   Sampled this frame; aim comes from here, not from the world.
/// @param dt      The fixed tick length. Never a frame delta.
///
/// @remarks
/// Horizontal movement is set directly rather than applied as force: a shooter
/// wants crisp response, and force-based movement through a solver produces the
/// mushy, mass-dependent feel that force is good at and this is not.
///
/// Vertical velocity is left to the solver, so gravity, falling and landing all
/// stay physics-authoritative.
void stepPlayer(engine::PhysicsWorld& physics, engine::BodyHandle body,
                const InputSnapshot& input, float dt, const MovementTunables& tunables);

} // namespace game

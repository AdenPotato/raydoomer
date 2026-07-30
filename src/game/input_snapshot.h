#pragma once

namespace game {

/// What the player asked for, sampled once per frame.
///
/// @remarks
/// **Input enters the tick as a parameter**, exactly like time and randomness.
/// That makes `World::tick` a pure function of state, input, dt and RNG - which
/// is what makes a run reproducible and a test possible.
///
/// ### Aim is sampled per frame, not per tick
///
/// `yawRadians` and `pitchRadians` are accumulated from raw mouse delta **every
/// frame**, then handed to whichever ticks run that frame
/// ([locked_decisions.md](../../docs/reference/locked_decisions.md)).
///
/// The simulation is locked at 60 Hz and the frame rate is not. If aim were
/// simulation state it would update 60 times a second no matter how fast the
/// game rendered, and on a high-refresh display that is immediately felt. Aim
/// arriving as an input keeps looking as smooth as the monitor while movement
/// physics stays on the fixed step where it belongs.
///
/// This is not presentation feeding back into the rules: the camera reads aim,
/// it does not produce it.
struct InputSnapshot {
    bool moveForward = false;
    bool moveBack = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;
    bool fire = false;

    /// Horizontal look angle in radians. 0 faces +Z.
    float yawRadians = 0.0f;

    /// Vertical look angle in radians, clamped by the caller to avoid flipping.
    float pitchRadians = 0.0f;

    friend bool operator==(const InputSnapshot&, const InputSnapshot&) = default;
};

} // namespace game

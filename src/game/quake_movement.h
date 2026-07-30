#pragma once

#include "platform/math_types.h"

namespace game {

/// Quake's acceleration.
///
/// @param velocity  Current velocity. Only X and Z participate; Y is untouched.
/// @param wishDir   Unit vector the player is asking to move along.
/// @param wishSpeed Speed cap **along `wishDir`** - not a cap on total speed.
/// @param accel     Acceleration coefficient. Dimensionless, as in Quake.
/// @param dt        Fixed tick length.
///
/// @remarks
/// The defining detail is that the cap applies to the **projection of velocity
/// onto `wishDir`**, not to the magnitude of velocity:
///
/// ```
/// addSpeed = wishSpeed - dot(velocity, wishDir)
/// if addSpeed <= 0: add nothing
/// velocity += wishDir * min(accel * dt * wishSpeed, addSpeed)
/// ```
///
/// Three consequences follow, and they are the whole character of the movement:
///
/// 1. **Momentum is preserved when turning.** Velocity is added to, never
///    replaced, so the player carves rather than pivoting on the spot.
/// 2. **Arriving overspeed is not punished.** If projected speed already
///    exceeds the cap, nothing is added - but nothing is taken away either.
///    Only friction slows you.
/// 3. **Strafing sideways while moving forward gains speed.** The projection
///    onto a perpendicular wish direction is near zero, so the cap is never
///    reached and speed accumulates past `wishSpeed`. In the air, where the cap
///    is small, this is strafe-jumping.
///
/// Point 3 is emergent, not coded, and it is what makes this Quake movement
/// rather than a slower version of something else.
platform::Vec3 accelerate(platform::Vec3 velocity, platform::Vec3 wishDir,
                          float wishSpeed, float accel, float dt);

/// Quake's ground friction.
///
/// @param stopSpeed Floor used in the drop calculation. Below this speed,
///        deceleration is computed against `stopSpeed` rather than the actual
///        speed, which is what produces a crisp stop. Without the floor,
///        deceleration is proportional to speed and the player creeps toward
///        zero asymptotically, which feels like ice.
///
/// @remarks
/// Horizontal only. Vertical velocity belongs to gravity and the solver;
/// applying friction to it would make falling feel like sinking.
platform::Vec3 applyFriction(platform::Vec3 velocity, float friction, float stopSpeed,
                             float dt);

} // namespace game

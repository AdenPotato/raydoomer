#pragma once

#include "platform/math_types.h"

namespace engine {

/// Raises a body-centre position to eye level.
///
/// @param bodyCentre The player's physics position.
/// @param eyeHeight  Metres above the centre. A tunable, not a literal.
///
/// @remarks
/// Without this the view comes out of the player's chest, which reads as being
/// oddly short and makes judging jumps harder than it should be.
platform::Point3 eyePosition(platform::Point3 bodyCentre, float eyeHeight);

/// Builds a first-person camera from an eye position and a look angle.
///
/// @param eye           Where the camera sits.
/// @param yawRadians    Horizontal look. **0 faces +Z**, matching the movement
///                      controller's forward. If the two ever disagree, walking
///                      forward and looking forward diverge.
/// @param pitchRadians  Vertical look. Positive is up. The caller clamps this
///                      short of straight up so the view cannot flip.
/// @param fovDegrees    Vertical field of view. A tunable.
///
/// @returns A camera whose target sits exactly one metre ahead along the look
///          direction, so that direction is a unit vector.
///
/// @remarks
/// Pure: same inputs, same camera, always. That is what keeps golden-image
/// tests repeatable, and it is why this is arithmetic in `engine/` rather than
/// state in the render backend.
///
/// The camera is **presentation**. Its position derives from simulated state,
/// and nothing in the rules may read it back (presentation_protocol.md).
platform::Camera firstPersonCamera(platform::Point3 eye, float yawRadians,
                                   float pitchRadians, float fovDegrees);

} // namespace engine

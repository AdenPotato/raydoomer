#pragma once

#include "platform/math_types.h"

namespace engine {

/// The right-hand vector for a camera-facing quad at `spritePosition`.
///
/// @remarks
/// Yaw only: the result is always level with the horizon. Rolling a sprite to
/// face a pitched camera looks wrong for this style, and Doom never did it.
///
/// Extracted here rather than left in the render backend because it is
/// arithmetic, and arithmetic in the backend is arithmetic no headless test can
/// reach.
///
/// @returns A unit vector perpendicular to the horizontal view direction.
///          Degenerate inputs - a camera directly above the sprite, or at the
///          same position - return an arbitrary but **valid** unit vector rather
///          than NaN. NaNs here reach vertex data and blank the screen, which is
///          a miserable thing to diagnose.
platform::Vec3 billboardRight(platform::Point3 spritePosition,
                              platform::Point3 cameraPosition);

/// Which directional frame of a sprite to show.
///
/// @param actorYawRadians   Which way the actor is facing.
/// @param viewerAngleRadians Angle from the actor to the viewer.
/// @param frameCount        How many directions the sprite has. Doom uses 8.
///
/// @returns A frame index in `[0, frameCount)`. Frame 0 is the actor facing the
///          viewer, and sectors are **centred** on their frame rather than
///          starting at it - so frame 0 spans half a sector either side of
///          dead-on. Offsetting by half a sector is the classic bug and it makes
///          sprites appear to snap early as an actor turns.
int directionalFrame(float actorYawRadians, float viewerAngleRadians, int frameCount);

} // namespace engine

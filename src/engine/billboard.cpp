#include "engine/billboard.h"

#include <cmath>
#include <numbers>

namespace engine {
namespace {

constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;

// Below this the horizontal view direction is meaningless and any perpendicular
// will do. Chosen well above float noise so a near-vertical view resolves
// stably rather than jittering between frames.
constexpr float kDegenerateThreshold = 1.0e-4f;

float wrapPositive(float radians) {
    const float wrapped = std::fmod(radians, kTwoPi);
    return wrapped < 0.0f ? wrapped + kTwoPi : wrapped;
}

} // namespace

platform::Vec3 billboardRight(platform::Point3 spritePosition,
                              platform::Point3 cameraPosition) {
    // Only the horizontal component matters: the quad stays upright.
    const auto dx = static_cast<float>(cameraPosition.x - spritePosition.x);
    const auto dz = static_cast<float>(cameraPosition.z - spritePosition.z);

    const float lengthSquared = dx * dx + dz * dz;
    if (lengthSquared < kDegenerateThreshold) {
        // Camera directly above, below, or exactly at the sprite. Every
        // orientation is equally correct, so pick one that is at least valid.
        return platform::Vec3{ 1.0f, 0.0f, 0.0f };
    }

    const float length = std::sqrt(lengthSquared);
    // Perpendicular to the view direction, in the horizontal plane: rotating
    // (dx, dz) by 90 degrees gives (dz, -dx).
    return platform::Vec3{ dz / length, 0.0f, -dx / length };
}

int directionalFrame(float actorYawRadians, float viewerAngleRadians, int frameCount) {
    if (frameCount <= 1) {
        return 0;
    }

    const float relative = wrapPositive(viewerAngleRadians - actorYawRadians);
    const float sector = kTwoPi / static_cast<float>(frameCount);

    // The half-sector offset is what centres frame 0 on "facing the viewer"
    // rather than starting it there. Without it every sprite switches frame
    // half a sector early, which reads as the actor snapping round.
    const float shifted = relative + sector * 0.5f;

    const int frame = static_cast<int>(shifted / sector);
    // The shift can push the top sector past the end; wrap rather than clamp,
    // because the angle is circular.
    return frame % frameCount;
}

} // namespace engine

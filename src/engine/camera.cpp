#include "engine/camera.h"

#include <cmath>

namespace engine {

platform::Point3 eyePosition(platform::Point3 bodyCentre, float eyeHeight) {
    return platform::Point3{ bodyCentre.x, bodyCentre.y + eyeHeight, bodyCentre.z };
}

platform::Camera firstPersonCamera(platform::Point3 eye, float yawRadians,
                                   float pitchRadians, float fovDegrees) {
    // Spherical to Cartesian. Yaw 0 faces +Z to match the movement controller,
    // which is why sin drives X and cos drives Z rather than the other way
    // round - the more common convention has yaw 0 facing +X.
    const float cosPitch = std::cos(pitchRadians);
    const platform::Vec3 direction{
        std::sin(yawRadians) * cosPitch,
        std::sin(pitchRadians),
        std::cos(yawRadians) * cosPitch,
    };

    // The target is one metre ahead, so the direction stays a unit vector. Any
    // other distance would make it useless as an aim ray later.
    return platform::Camera{
        eye,
        platform::Point3{ eye.x + direction.x, eye.y + direction.y, eye.z + direction.z },
        fovDegrees,
    };
}

} // namespace engine

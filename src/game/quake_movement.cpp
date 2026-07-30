#include "game/quake_movement.h"

#include <cmath>

namespace game {

platform::Vec3 accelerate(platform::Vec3 velocity, platform::Vec3 wishDir,
                          float wishSpeed, float accel, float dt) {
    // Projection of current velocity onto the requested direction. This, rather
    // than the magnitude of velocity, is what the cap applies to.
    const float currentSpeed = velocity.x * wishDir.x + velocity.z * wishDir.z;

    const float addSpeed = wishSpeed - currentSpeed;
    if (addSpeed <= 0.0f) {
        // Already at or beyond the cap along this direction. Add nothing - and
        // crucially, take nothing away. An overspeed player keeps their speed.
        return velocity;
    }

    float accelSpeed = accel * dt * wishSpeed;
    if (accelSpeed > addSpeed) {
        accelSpeed = addSpeed;
    }

    return platform::Vec3{
        velocity.x + wishDir.x * accelSpeed,
        velocity.y,
        velocity.z + wishDir.z * accelSpeed,
    };
}

platform::Vec3 applyFriction(platform::Vec3 velocity, float friction, float stopSpeed,
                             float dt) {
    const float speed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
    if (speed <= 0.0f) {
        return velocity;
    }

    // The stopSpeed floor. Below it, drop is computed against stopSpeed rather
    // than the actual speed, so the last fraction of a metre per second is shed
    // at a constant rate instead of asymptotically.
    const float control = speed < stopSpeed ? stopSpeed : speed;
    const float drop = control * friction * dt;

    float newSpeed = speed - drop;
    if (newSpeed < 0.0f) {
        newSpeed = 0.0f;
    }
    const float scale = newSpeed / speed;

    return platform::Vec3{ velocity.x * scale, velocity.y, velocity.z * scale };
}

} // namespace game

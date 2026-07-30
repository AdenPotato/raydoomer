#include "game/player_controller.h"

#include "game/quake_movement.h"

#include <nlohmann/json.hpp>

#include <cmath>

namespace game {
namespace {

/// How far below the body centre to look for ground.
///
/// Must clear the shape itself: probing only by the tolerance starts and ends
/// inside the player, finds nothing, and reports permanently airborne.
float groundProbeLength(const MovementTunables& tunables) {
    return tunables.characterRadius + tunables.groundedTolerance;
}

/// The horizontal direction the player is asking to move, in world space.
///
/// Aim yaw decides which way "forward" points, which is why aim is an input
/// rather than simulation state.
engine::Vec3 wishDirection(const InputSnapshot& input) {
    // Local axes: forward is +Z at yaw 0, right is +X.
    float localForward = 0.0f;
    float localRight = 0.0f;
    if (input.moveForward) localForward += 1.0f;
    if (input.moveBack) localForward -= 1.0f;
    if (input.moveRight) localRight += 1.0f;
    if (input.moveLeft) localRight -= 1.0f;

    if (localForward == 0.0f && localRight == 0.0f) {
        return engine::Vec3{ 0.0f, 0.0f, 0.0f };
    }

    // Normalise before rotating, so diagonal movement is not faster than
    // straight movement - the classic bug this avoids.
    const float length = std::sqrt(localForward * localForward + localRight * localRight);
    localForward /= length;
    localRight /= length;

    const float sinYaw = std::sin(input.yawRadians);
    const float cosYaw = std::cos(input.yawRadians);

    // Right-handed, Y up. Facing +Z, right is `forward x up` = -X, which is why
    // the right terms are negated relative to the naive form. Getting this
    // backwards mirrors the controls: A strafes right and D strafes left.
    return engine::Vec3{
        localForward * sinYaw - localRight * cosYaw,
        0.0f,
        localForward * cosYaw + localRight * sinYaw,
    };
}

/// Reads a float, tolerating both absence and the wrong type.
///
/// nlohmann's `value()` handles a missing key but **throws** on a wrong-typed
/// one, so `"maxSpeed": "fast"` in a hand-edited data file would crash the game.
/// content_protocol.md: never crash on bad input. A malformed field falls back
/// to the default, which keeps the player moving while the designer fixes it.
float readFloat(const nlohmann::json& document, const char* key, float fallback) {
    const auto field = document.find(key);
    if (field == document.end() || !field->is_number()) {
        return fallback;
    }
    return field->get<float>();
}

} // namespace

MovementTunables movementTunablesFromJson(std::string_view json) {
    MovementTunables tunables;

    const auto document = nlohmann::json::parse(json, nullptr, /*allow_exceptions=*/false);
    if (document.is_discarded() || !document.is_object()) {
        return tunables; // a broken file still yields a moving player
    }

    tunables.maxSpeed = readFloat(document, "maxSpeed", tunables.maxSpeed);
    tunables.groundAccelerate =
        readFloat(document, "groundAccelerate", tunables.groundAccelerate);
    tunables.airAccelerate = readFloat(document, "airAccelerate", tunables.airAccelerate);
    tunables.airWishSpeedCap =
        readFloat(document, "airWishSpeedCap", tunables.airWishSpeedCap);
    tunables.friction = readFloat(document, "friction", tunables.friction);
    tunables.stopSpeed = readFloat(document, "stopSpeed", tunables.stopSpeed);
    tunables.jumpImpulse = readFloat(document, "jumpImpulse", tunables.jumpImpulse);
    tunables.characterRadius =
        readFloat(document, "characterRadius", tunables.characterRadius);
    tunables.eyeHeight = readFloat(document, "eyeHeight", tunables.eyeHeight);
    tunables.fieldOfView = readFloat(document, "fieldOfView", tunables.fieldOfView);
    tunables.mouseSensitivity =
        readFloat(document, "mouseSensitivity", tunables.mouseSensitivity);
    tunables.groundedTolerance =
        readFloat(document, "groundedTolerance", tunables.groundedTolerance);
    return tunables;
}

void stepPlayer(engine::PhysicsWorld& physics, engine::BodyHandle body,
                const InputSnapshot& input, float dt, const MovementTunables& tunables) {
    const auto currentVelocity = physics.linearVelocity(body);
    const auto position = physics.position(body);
    if (!currentVelocity.has_value() || !position.has_value()) {
        return; // stale handle; already reported by the wrapper
    }

    // Ground check by probing straight down. A tolerance is essential: without
    // one the player counts as airborne on any frame it bounces a fraction of a
    // millimetre off the floor, and air control flickers on and off.
    const auto ground = physics.raycastClosest(
        *position, engine::Vec3{ 0.0f, -groundProbeLength(tunables), 0.0f });
    const bool grounded = ground.has_value();

    const engine::Vec3 wish = wishDirection(input);
    const bool hasInput = wish.x != 0.0f || wish.z != 0.0f;

    platform::Vec3 velocity = *currentVelocity;

    // Friction first, then acceleration - Quake's order. Reversing them shaves
    // the speed you just gained and makes acceleration feel weaker than its
    // number suggests.
    if (grounded && !hasInput) {
        velocity = applyFriction(velocity, tunables.friction, tunables.stopSpeed, dt);
    }

    if (hasInput) {
        // The only difference between ground and air is the cap and the
        // coefficient. Everything that makes air movement distinctive falls out
        // of the small airborne cap rather than from separate code.
        const float wishSpeed = grounded ? tunables.maxSpeed : tunables.airWishSpeedCap;
        const float accel = grounded ? tunables.groundAccelerate : tunables.airAccelerate;
        velocity = accelerate(velocity, wish, wishSpeed, accel, dt);
    }

    float velocityX = velocity.x;
    float velocityZ = velocity.z;

    float velocityY = velocity.y;
    if (input.jump && grounded) {
        velocityY = tunables.jumpImpulse;
    }

    physics.setLinearVelocity(body, engine::Vec3{ velocityX, velocityY, velocityZ });
}

} // namespace game

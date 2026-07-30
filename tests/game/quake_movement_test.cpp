// Quake's movement model.
//
// The defining difference from a naive controller: acceleration projects
// current velocity onto the wish direction rather than adding and clamping the
// magnitude. From that one choice everything else follows - the weighty ramp,
// the preserved momentum when turning, and strafe-jumping.
//
// These tests pin the model itself. Whether it feels right is a playtest.

#include <gtest/gtest.h>

#include "game/player_controller.h"
#include "game/quake_movement.h"

#include <algorithm>
#include <cmath>

namespace {

// Quake 3 defaults, converted from Quake units (1 unit ~ 0.0254 m).
game::MovementTunables quakeDefaults() {
    game::MovementTunables t;
    t.maxSpeed = 8.13f;          // 320 ups
    t.groundAccelerate = 10.0f;
    t.airAccelerate = 1.0f;
    t.airWishSpeedCap = 0.76f;   // 30 ups - the strafe-jump enabler
    t.friction = 6.0f;
    t.stopSpeed = 2.54f;         // 100 ups
    return t;
}

float speedOf(platform::Vec3 v) {
    return std::sqrt(v.x * v.x + v.z * v.z);
}

constexpr float kTick = 1.0f / 60.0f;

} // namespace

// --- Ground acceleration ----------------------------------------------------

TEST(QuakeMovement, AcceleratesFromRestTowardMaxSpeed) {
    const auto t = quakeDefaults();
    platform::Vec3 velocity{};
    const platform::Vec3 wish{ 0.0f, 0.0f, 1.0f };

    for (int i = 0; i < 120; ++i) {
        velocity = game::accelerate(velocity, wish, t.maxSpeed, t.groundAccelerate, kTick);
    }

    EXPECT_NEAR(speedOf(velocity), t.maxSpeed, 0.01f);
}

TEST(QuakeMovement, NeverExceedsWishSpeedAlongTheWishDirection) {
    // The cap is on velocity PROJECTED onto the input direction, which is what
    // makes strafe-jumping possible while keeping straight-line speed bounded.
    const auto t = quakeDefaults();
    platform::Vec3 velocity{};
    const platform::Vec3 wish{ 0.0f, 0.0f, 1.0f };

    for (int i = 0; i < 600; ++i) {
        velocity = game::accelerate(velocity, wish, t.maxSpeed, t.groundAccelerate, kTick);
    }

    const float alongWish = velocity.x * wish.x + velocity.z * wish.z;
    EXPECT_LE(alongWish, t.maxSpeed + 0.01f);
}

TEST(QuakeMovement, AddsNothingWhenAlreadyAtWishSpeed) {
    const auto t = quakeDefaults();
    const platform::Vec3 wish{ 0.0f, 0.0f, 1.0f };
    platform::Vec3 velocity{ 0.0f, 0.0f, t.maxSpeed };

    const auto after = game::accelerate(velocity, wish, t.maxSpeed, t.groundAccelerate, kTick);

    EXPECT_NEAR(after.z, t.maxSpeed, 1e-4f);
}

TEST(QuakeMovement, AddsNothingWhenAlreadyFasterThanWishSpeed) {
    // Arriving above the cap - off a ramp, or from a strafe jump - must not be
    // braked by the accelerate step. Only friction slows you.
    const auto t = quakeDefaults();
    const platform::Vec3 wish{ 0.0f, 0.0f, 1.0f };
    platform::Vec3 velocity{ 0.0f, 0.0f, t.maxSpeed * 2.0f };

    const auto after = game::accelerate(velocity, wish, t.maxSpeed, t.groundAccelerate, kTick);

    EXPECT_NEAR(after.z, t.maxSpeed * 2.0f, 1e-4f) << "accelerate braked an overspeed player";
}

TEST(QuakeMovement, TurningPreservesMomentumRatherThanSnapping) {
    // Velocity is added to, never replaced. A controller that simply sets
    // velocity to wishDir * maxSpeed turns on a coin, which is precisely the
    // feel Quake does not have.
    const auto t = quakeDefaults();
    platform::Vec3 velocity{ 0.0f, 0.0f, t.maxSpeed };
    const platform::Vec3 wish{ 1.0f, 0.0f, 0.0f }; // hard right turn

    velocity = game::accelerate(velocity, wish, t.maxSpeed, t.groundAccelerate, kTick);

    EXPECT_GT(velocity.z, t.maxSpeed * 0.9f) << "forward momentum vanished on turning";
    EXPECT_GT(velocity.x, 0.0f) << "the new direction contributed nothing";
}

// --- Friction ---------------------------------------------------------------

TEST(QuakeMovement, FrictionBringsAMovingPlayerToRest) {
    const auto t = quakeDefaults();
    platform::Vec3 velocity{ 0.0f, 0.0f, t.maxSpeed };

    for (int i = 0; i < 300; ++i) {
        velocity = game::applyFriction(velocity, t.friction, t.stopSpeed, kTick);
    }

    EXPECT_NEAR(speedOf(velocity), 0.0f, 0.01f);
}

TEST(QuakeMovement, StopSpeedGivesACrispStopRatherThanAnAsymptote) {
    // Below stopSpeed, friction is computed against stopSpeed rather than the
    // actual speed. Without that floor, deceleration is proportional to speed
    // and the player creeps toward zero forever.
    const auto t = quakeDefaults();
    platform::Vec3 slow{ 0.0f, 0.0f, 0.05f };

    for (int i = 0; i < 30; ++i) {
        slow = game::applyFriction(slow, t.friction, t.stopSpeed, kTick);
    }

    EXPECT_FLOAT_EQ(speedOf(slow), 0.0f) << "crept toward zero instead of stopping";
}

TEST(QuakeMovement, FrictionDoesNotReverseDirection) {
    const auto t = quakeDefaults();
    platform::Vec3 velocity{ 0.0f, 0.0f, 1.0f };

    for (int i = 0; i < 300; ++i) {
        velocity = game::applyFriction(velocity, t.friction, t.stopSpeed, kTick);
        EXPECT_GE(velocity.z, 0.0f) << "friction pushed the player backwards";
    }
}

TEST(QuakeMovement, FrictionLeavesVerticalVelocityAlone) {
    // Gravity and jumping are the solver's business. Friction applied to Y would
    // make falling feel like sinking through treacle.
    const auto t = quakeDefaults();
    platform::Vec3 velocity{ 3.0f, -9.0f, 0.0f };

    const auto after = game::applyFriction(velocity, t.friction, t.stopSpeed, kTick);

    EXPECT_FLOAT_EQ(after.y, -9.0f);
}

// --- Air movement, and the behaviour it produces -----------------------------

TEST(QuakeMovement, AirAccelerationIsCappedByWishSpeedNotByMaxSpeed) {
    // In the air the cap is airWishSpeedCap, a fraction of ground speed. Pushing
    // straight forward in the air therefore does almost nothing.
    const auto t = quakeDefaults();
    const platform::Vec3 wish{ 0.0f, 0.0f, 1.0f };
    platform::Vec3 velocity{ 0.0f, 0.0f, t.maxSpeed };

    for (int i = 0; i < 60; ++i) {
        velocity = game::accelerate(velocity, wish, t.airWishSpeedCap, t.airAccelerate, kTick);
    }

    EXPECT_NEAR(velocity.z, t.maxSpeed, 0.01f)
        << "forward air acceleration gained speed it should not";
}

TEST(QuakeMovement, StrafeJumpingAccumulatesSpeedBeyondMaxSpeed) {
    // Strafe-jumping, modelled as it is actually performed: hold a strafe key
    // and TURN continuously, keeping the wish direction just inside the angle
    // where its projection onto velocity still falls under the cap.
    //
    // That angle matters. Too shallow - say 45 degrees - and the projection of
    // an 8 m/s velocity onto the wish direction is around 5.7 m/s, far above the
    // 0.76 cap, so addSpeed is negative and NOTHING is added. Two earlier
    // versions of this test failed for exactly that reason. The usable window
    // sits near cos(theta) = cap / speed, about 85 degrees.
    //
    // The gain per tick is small by design at Quake 3's airAccelerate of 1,
    // which is why the technique reads as a skill rather than a button.
    const auto t = quakeDefaults();
    platform::Vec3 velocity{ 0.0f, 0.0f, t.maxSpeed };
    const float startSpeed = speedOf(velocity);
    float previousSpeed = startSpeed;

    for (int i = 0; i < 600; ++i) {
        const float speed = speedOf(velocity);
        const float heading = std::atan2(velocity.x, velocity.z);

        // Just past the threshold where the projection clears the cap.
        const float threshold = std::acos(std::min(1.0f, t.airWishSpeedCap / speed));
        const float offset = heading + threshold + 0.02f;
        const platform::Vec3 wish{ std::sin(offset), 0.0f, std::cos(offset) };

        velocity = game::accelerate(velocity, wish, t.airWishSpeedCap, t.airAccelerate, kTick);

        // Never loses speed. accelerate only ever adds.
        EXPECT_GE(speedOf(velocity), previousSpeed - 1e-4f) << "lost speed at tick " << i;
        previousSpeed = speedOf(velocity);
    }

    EXPECT_GT(speedOf(velocity), startSpeed * 1.02f)
        << "strafe jumping did not accumulate speed - this is not Quake movement";
    EXPECT_GT(speedOf(velocity), t.maxSpeed)
        << "speed never exceeded maxSpeed, which strafe jumping exists to do";
}

TEST(QuakeMovement, AFixedSidewaysPushSaturatesRatherThanAccumulating) {
    // The contrast that makes the previous test meaningful: without turning, the
    // gain is bounded by airWishSpeedCap and stops. Speed gain requires the
    // player to actively steer, which is exactly why it reads as a skill.
    const auto t = quakeDefaults();
    platform::Vec3 velocity{ 0.0f, 0.0f, t.maxSpeed };
    const platform::Vec3 wish{ 1.0f, 0.0f, 0.0f };

    for (int i = 0; i < 600; ++i) {
        velocity = game::accelerate(velocity, wish, t.airWishSpeedCap, t.airAccelerate, kTick);
    }

    EXPECT_LT(velocity.x, t.airWishSpeedCap + 0.01f)
        << "a fixed push accumulated without limit";
}

TEST(QuakeMovement, AirAccelerationTunableControlsHowStrongTheEffectIs) {
    // airAccelerate is the dial. Zero should remove air control entirely, which
    // is the option for a project that does not want movement tech.
    const auto t = quakeDefaults();
    const platform::Vec3 wish{ 1.0f, 0.0f, 0.0f };
    platform::Vec3 velocity{ 0.0f, 0.0f, t.maxSpeed };
    const auto before = velocity;

    for (int i = 0; i < 60; ++i) {
        velocity = game::accelerate(velocity, wish, t.airWishSpeedCap, 0.0f, kTick);
    }

    EXPECT_FLOAT_EQ(velocity.x, before.x) << "zero air acceleration still steered";
}

// --- Determinism -------------------------------------------------------------

TEST(QuakeMovement, IsDeterministic) {
    const auto t = quakeDefaults();
    auto run = [&] {
        platform::Vec3 velocity{};
        const platform::Vec3 wish{ 0.3f, 0.0f, 0.9f };
        for (int i = 0; i < 200; ++i) {
            velocity = game::accelerate(velocity, wish, t.maxSpeed, t.groundAccelerate, kTick);
            velocity = game::applyFriction(velocity, t.friction, t.stopSpeed, kTick);
        }
        return velocity;
    };

    EXPECT_EQ(run(), run());
}

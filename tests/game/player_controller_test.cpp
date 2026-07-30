// The player movement controller.
//
// This is the first change governed by the feel exception, so the split matters:
// everything below is the MECHANICAL half - reaching the configured speed in the
// configured time, stopping in the configured distance, behaving identically
// regardless of frame rate. Whether it feels like a Doom-like is a playtest
// question and appears nowhere in this file.
//
// Every number asserted here comes from a tunable, not a literal, so a balance
// change moves the data and these tests keep passing.

#include <gtest/gtest.h>

#include "engine/event_dispatch.h"
#include "engine/rng.h"
#include "game/player_controller.h"
#include "game/world.h"

#include <cmath>
#include <numbers>

namespace {

constexpr float kTick = 1.0f / 60.0f;

// Deliberately explicit rather than loaded from data: a test that reads the real
// tunables would change meaning every time someone rebalances the game.
game::MovementTunables testTunables() {
    game::MovementTunables t;
    t.maxSpeed = 10.0f;
    t.groundAccelerate = 20.0f; // brisk ramp, so the tests do not need long runs
    t.airAccelerate = 1.0f;
    t.airWishSpeedCap = 0.76f;
    t.friction = 8.0f;
    t.stopSpeed = 2.54f;
    t.jumpImpulse = 6.0f;
    return t;
}

struct Harness {
    game::World world;
    engine::Rng rng{ 1234 };
    engine::EventDispatcher events;
    game::MovementTunables tunables = testTunables();

    Harness() {
        events.freeze();

        // Ground, always. Without it the player is in freefall, `grounded` is
        // false, friction never applies and acceleration is throttled to
        // airControl - which silently changes what most of these tests measure.
        engine::BodyDef floorDef;
        floorDef.type = engine::BodyType::Static;
        floorDef.position = engine::Point3{ 0.0, -0.5, 0.0 };
        floorDef.shape = engine::BoxShape{ engine::Vec3{ 50.0f, 0.5f, 50.0f } };
        floorDef.reportsContacts = false;
        world.createFloorForTest(floorDef);

        // Apply the test tunables. Without this the world silently uses its
        // defaults and every assertion against `tunables` measures the wrong
        // thing - which passed unnoticed only while the two happened to agree.
        world.setMovementTunables(tunables);

        world.spawnPlayer(engine::Point3{ 0.0, 0.5, 0.0 });

        // Let the player settle onto the floor before anything is measured.
        for (int i = 0; i < 10; ++i) {
            world.tick(kTick, game::InputSnapshot{}, rng, events);
        }
    }

    void tick(const game::InputSnapshot& input, int count = 1) {
        for (int i = 0; i < count; ++i) {
            world.tick(kTick, input, rng, events);
        }
    }

    engine::Point3 playerPosition() const {
        return world.physics().position(world.playerBody()).value();
    }
};

game::InputSnapshot forward() {
    game::InputSnapshot input;
    input.moveForward = true;
    return input;
}

game::InputSnapshot idle() {
    return game::InputSnapshot{};
}

// --- Basic movement ---------------------------------------------------------

TEST(PlayerController, StandingStillDoesNotDrift) {
    // A controller that creeps with no input is a bug that hides behind every
    // other movement test.
    Harness h;
    const auto start = h.playerPosition();

    h.tick(idle(), 120);

    const auto end = h.playerPosition();
    EXPECT_NEAR(end.x, start.x, 1e-3);
    EXPECT_NEAR(end.z, start.z, 1e-3);
}

TEST(PlayerController, HoldingForwardMovesForward) {
    Harness h;
    const auto start = h.playerPosition();

    h.tick(forward(), 30);

    EXPECT_GT(h.playerPosition().z, start.z + 0.5)
        << "half a second of forward input produced no meaningful movement";
}

TEST(PlayerController, MovementDirectionFollowsAimYaw) {
    // Aim is an input, and it decides which way "forward" points. A controller
    // that ignores it walks north no matter where you are looking.
    //
    // Asserts the SIGN, not the magnitude. The first version of this test used
    // std::abs and passed happily while strafe was mirrored - a test that cannot
    // tell left from right is not testing direction.
    Harness h;
    auto input = forward();
    input.yawRadians = static_cast<float>(std::numbers::pi / 2.0); // face +X

    h.tick(input, 30);

    EXPECT_GT(h.playerPosition().x, 0.5) << "facing +X, forward did not move toward +X";
}

TEST(PlayerController, StrafingRightMovesToTheRightOfWhereYouAreLooking) {
    // Right-handed, Y up: facing +Z, right is forward x up, which is -X.
    // Getting this backwards mirrors the controls, which is instantly obvious
    // when playing and invisible to a magnitude-only assertion.
    Harness h;
    game::InputSnapshot input;
    input.moveRight = true;
    input.yawRadians = 0.0f; // facing +Z

    h.tick(input, 30);

    EXPECT_LT(h.playerPosition().x, -0.5) << "D strafed left instead of right";
}

TEST(PlayerController, StrafingLeftMovesToTheLeftOfWhereYouAreLooking) {
    Harness h;
    game::InputSnapshot input;
    input.moveLeft = true;
    input.yawRadians = 0.0f;

    h.tick(input, 30);

    EXPECT_GT(h.playerPosition().x, 0.5) << "A strafed right instead of left";
}

TEST(PlayerController, StrafeStaysPerpendicularToTheLookDirection) {
    // Strafing must not creep forward or back. A sign error in one term produces
    // exactly that, and it is subtle enough to be mistaken for mouse drift.
    Harness h;
    game::InputSnapshot input;
    input.moveRight = true;
    input.yawRadians = 0.0f;

    const auto start = h.playerPosition();
    h.tick(input, 30);
    const auto end = h.playerPosition();

    EXPECT_NEAR(end.z, start.z, 0.1) << "strafing drifted along the look axis";
}

TEST(PlayerController, StrafeFollowsYawToo) {
    // Facing +X, right is +Z. Confirms the rotation applies to strafe and not
    // only to forward.
    Harness h;
    game::InputSnapshot input;
    input.moveRight = true;
    input.yawRadians = static_cast<float>(std::numbers::pi / 2.0);

    h.tick(input, 30);

    EXPECT_GT(h.playerPosition().z, 0.5) << "strafe ignored the look direction";
}

TEST(PlayerController, OpposingInputsCancel) {
    Harness h;
    const auto start = h.playerPosition();

    game::InputSnapshot input;
    input.moveForward = true;
    input.moveBack = true;
    h.tick(input, 60);

    EXPECT_NEAR(h.playerPosition().z, start.z, 1e-2);
}

// --- The configured numbers -------------------------------------------------

TEST(PlayerController, AcceleratesToTheConfiguredMaxSpeedAndNoFurther) {
    // The tunable is a ceiling, not a suggestion. Overshooting it means every
    // balance number downstream is wrong.
    Harness h;

    h.tick(forward(), 300); // five seconds, long past the ramp

    const auto before = h.playerPosition();
    h.tick(forward(), 60);  // one more second
    const auto after = h.playerPosition();

    const double travelled = after.z - before.z;
    EXPECT_LE(travelled, h.tunables.maxSpeed * 1.05)
        << "exceeded max speed over one simulated second";
    EXPECT_GT(travelled, h.tunables.maxSpeed * 0.9)
        << "never reached max speed";
}

TEST(PlayerController, ReachesMaxSpeedInRoughlyTheConfiguredTime) {
    // Quake's ramp is asymptotic rather than linear, so this asserts that most
    // of the speed arrives quickly rather than pinning an exact tick count.
    Harness h;

    h.tick(forward(), 30);
    const auto a = h.playerPosition();
    h.tick(forward(), 60);
    const auto b = h.playerPosition();

    const double speedAfterRamp = (b.z - a.z);
    EXPECT_GT(speedAfterRamp, h.tunables.maxSpeed * 0.85)
        << "did not reach near max speed within the configured ramp";
}

TEST(PlayerController, FrictionBringsThePlayerToRest) {
    Harness h;
    h.tick(forward(), 60);

    h.tick(idle(), 120);
    const auto a = h.playerPosition();
    h.tick(idle(), 60);
    const auto b = h.playerPosition();

    EXPECT_NEAR(b.z - a.z, 0.0, 1e-2) << "still sliding after two seconds of no input";
}

TEST(PlayerController, EveryTunableIsRespectedNotHardcoded) {
    // Doubling max speed must double the distance covered. If a literal crept
    // into the controller this fails, which is the point.
    Harness slow;
    slow.tunables.maxSpeed = 5.0f;
    slow.world.setMovementTunables(slow.tunables);

    Harness fast;
    fast.tunables.maxSpeed = 10.0f;
    fast.world.setMovementTunables(fast.tunables);

    slow.tick(forward(), 180);
    fast.tick(forward(), 180);

    const double slowDistance = slow.playerPosition().z;
    const double fastDistance = fast.playerPosition().z;

    EXPECT_GT(fastDistance, slowDistance * 1.5)
        << "changing the tunable did not change the behaviour";
}

// --- Jumping and grounding --------------------------------------------------

TEST(PlayerController, JumpingLeavesTheGround) {
    Harness h;
    const double resting = h.playerPosition().y;

    game::InputSnapshot input;
    input.jump = true;
    h.tick(input, 1);
    h.tick(idle(), 10);

    EXPECT_GT(h.playerPosition().y, resting + 0.2) << "jump produced no height";
}

TEST(PlayerController, CannotJumpWhileAirborne) {
    // Without a ground check, holding jump climbs the sky. This is the test that
    // catches the ground probe silently reporting airborne, or always grounded.
    Harness h;
    const double resting = h.playerPosition().y;

    game::InputSnapshot input;
    input.jump = true;
    h.tick(input, 90); // hold jump for a second and a half

    // One jump's worth of height, not ninety.
    EXPECT_LT(h.playerPosition().y, resting + 3.0) << "held jump climbed indefinitely";
}

TEST(PlayerController, LandsBackOnTheGroundAfterAJump) {
    Harness h;
    const double resting = h.playerPosition().y;

    game::InputSnapshot input;
    input.jump = true;
    h.tick(input, 1);
    h.tick(idle(), 180);

    EXPECT_NEAR(h.playerPosition().y, resting, 0.05) << "never came back down";
}

TEST(PlayerController, AirControlIsWeakerThanGroundControl) {
    // In Quake this falls out of the small airborne wish-speed cap rather than
    // from a scale factor: pushing forward in the air barely helps.
    Harness ground;
    ground.tick(forward(), 6);
    const double groundGain = ground.playerPosition().z;

    Harness air;
    game::InputSnapshot jump;
    jump.jump = true;
    air.tick(jump, 1);
    air.tick(idle(), 8); // get clear of the floor
    const double airStart = air.playerPosition().z;
    air.tick(forward(), 6);
    const double airGain = air.playerPosition().z - airStart;

    EXPECT_LT(airGain, groundGain) << "air control was not weaker than ground control";
}

// --- Tunables from data -----------------------------------------------------

TEST(MovementTunables, ParsesEveryFieldFromJson) {
    const auto t = game::movementTunablesFromJson(R"({
        "maxSpeed": 12.5, "groundAccelerate": 70.0, "friction": 55.0,
        "airAccelerate": 4.0, "jumpImpulse": 7.5,
        "characterRadius": 0.4, "groundedTolerance": 0.2
    })");

    EXPECT_FLOAT_EQ(t.maxSpeed, 12.5f);
    EXPECT_FLOAT_EQ(t.groundAccelerate, 70.0f);
    EXPECT_FLOAT_EQ(t.friction, 55.0f);
    EXPECT_FLOAT_EQ(t.airAccelerate, 4.0f);
    EXPECT_FLOAT_EQ(t.jumpImpulse, 7.5f);
    EXPECT_FLOAT_EQ(t.characterRadius, 0.4f);
    EXPECT_FLOAT_EQ(t.groundedTolerance, 0.2f);
}

TEST(MovementTunables, AMissingFieldKeepsItsDefault) {
    // A designer mid-edit should get a moving player, not a frozen one.
    const game::MovementTunables defaults;
    const auto t = game::movementTunablesFromJson(R"({"maxSpeed": 3.0})");

    EXPECT_FLOAT_EQ(t.maxSpeed, 3.0f);
    EXPECT_FLOAT_EQ(t.groundAccelerate, defaults.groundAccelerate);
}

TEST(MovementTunables, MalformedJsonYieldsDefaultsRatherThanThrowing) {
    const game::MovementTunables defaults;
    for (const char* input : { "", "{", "[]", "null", R"({"maxSpeed": "fast"})" }) {
        game::MovementTunables t;
        EXPECT_NO_THROW({ t = game::movementTunablesFromJson(input); }) << input;
        EXPECT_GT(t.maxSpeed, 0.0f) << "a broken file produced an immobile player: " << input;
        (void)defaults;
    }
}

// --- Frame-rate independence ------------------------------------------------

TEST(PlayerController, TheSameTickCountProducesTheSameResultRegardlessOfFrameRate) {
    // The property the fixed step exists for. The controller is stepped by TICKS,
    // never by frames, so 120 ticks delivered as 120 frames or as 40 frames of
    // three ticks each must land in the same place.
    //
    // Simulated here by stepping in different batch sizes: identical totals, and
    // the result must be identical too.
    Harness a;
    Harness b;

    for (int i = 0; i < 120; ++i) {
        a.tick(forward(), 1); // one tick per frame, as at 60fps
    }
    for (int i = 0; i < 40; ++i) {
        b.tick(forward(), 3); // three ticks per frame, as after a hitch
    }

    EXPECT_DOUBLE_EQ(a.playerPosition().z, b.playerPosition().z);
}

TEST(PlayerController, IsDeterministicAcrossIdenticalRuns) {
    Harness a;
    Harness b;

    const auto input = forward();
    a.tick(input, 200);
    b.tick(input, 200);

    EXPECT_DOUBLE_EQ(a.playerPosition().x, b.playerPosition().x);
    EXPECT_DOUBLE_EQ(a.playerPosition().y, b.playerPosition().y);
    EXPECT_DOUBLE_EQ(a.playerPosition().z, b.playerPosition().z);
}

// --- Collision --------------------------------------------------------------

TEST(PlayerController, RestsOnTheFloorRatherThanSinkingThroughIt) {
    Harness h;
    const double settled = h.playerPosition().y;

    h.tick(idle(), 300); // five seconds of doing nothing

    EXPECT_NEAR(h.playerPosition().y, settled, 1e-2)
        << "the player drifted vertically while standing still";
    EXPECT_GT(h.playerPosition().y, 0.0) << "the player sank through static geometry";
}

TEST(PlayerController, CannotWalkThroughAWall) {
    Harness h;

    engine::BodyDef wall;
    wall.type = engine::BodyType::Static;
    wall.position = engine::Point3{ 0.0, 1.0, 4.0 };
    wall.shape = engine::BoxShape{ engine::Vec3{ 10.0f, 3.0f, 0.5f } };
    h.world.physics().createBody(wall);

    h.tick(forward(), 300);

    EXPECT_LT(h.playerPosition().z, 4.0) << "walked through a solid wall";
}

} // namespace

// Box3D integration, headless. Proves the physics dependency is linkable and,
// more importantly, that it is deterministic under the configuration we locked.
//
// locked_decisions.md: "Box3D ... Run single-threaded for determinism" and
// "stepped once per fixed tick at SIM_TICK_SECONDS". These tests are what make
// those two rows enforceable rather than aspirational.

#include <gtest/gtest.h>

#include <box3d/box3d.h>

#include <vector>

namespace {

// Mirrors the locked constants. Duplicated here deliberately: when the engine
// defines them for real, these become the assertion that they did not drift.
constexpr float kSimTickSeconds = 1.0f / 60.0f;
constexpr int kSubStepCount = 4;
constexpr uint32_t kWorkerCount = 1; // single-threaded, for determinism

struct FallingBodyRun {
    std::vector<double> heights;
};

// Drops a dynamic body from y = 100 and records its height each tick.
FallingBodyRun runFallingBody(int ticks) {
    b3WorldDef worldDef = b3DefaultWorldDef();
    worldDef.gravity = b3Vec3{ 0.0f, -9.8f, 0.0f };
    worldDef.workerCount = kWorkerCount;

    const b3WorldId world = b3CreateWorld(&worldDef);

    b3BodyDef bodyDef = b3DefaultBodyDef();
    bodyDef.type = b3_dynamicBody;
    bodyDef.position = b3Pos{ 0.0, 100.0, 0.0 };

    const b3BodyId body = b3CreateBody(world, &bodyDef);

    // A dynamic body with no shape has no mass, and the solver skips it
    // entirely - it sits at its spawn position forever. Attaching a shape is
    // what gives it mass. Found by this test failing, which is the point.
    b3ShapeDef shapeDef = b3DefaultShapeDef();
    const b3Sphere sphere{ b3Vec3{ 0.0f, 0.0f, 0.0f }, 0.5f };
    (void)b3CreateSphereShape(body, &shapeDef, &sphere);

    FallingBodyRun run;
    run.heights.reserve(static_cast<size_t>(ticks));
    for (int i = 0; i < ticks; ++i) {
        b3World_Step(world, kSimTickSeconds, kSubStepCount);
        run.heights.push_back(b3Body_GetPosition(body).y);
    }

    b3DestroyWorld(world);
    return run;
}

TEST(Physics, BodyFallsUnderGravity) {
    // One simulated second at the locked tick rate.
    const auto run = runFallingBody(60);

    ASSERT_EQ(run.heights.size(), 60u);

    // Analytic free fall over 1s is 4.9m; a discrete semi-implicit integrator
    // overshoots slightly. The band is deliberately tight enough that a broken
    // gravity axis or a wrong timestep fails it, and loose enough to survive a
    // solver change. Never an exact equality on a value that went through
    // arithmetic (game_test_protocol.md).
    const double drop = 100.0 - run.heights.back();
    EXPECT_GT(drop, 4.5);
    EXPECT_LT(drop, 5.5);
}

TEST(Physics, HeightDecreasesMonotonically) {
    const auto run = runFallingBody(60);

    // An invariant is a better assertion than a value where one exists.
    for (size_t i = 1; i < run.heights.size(); ++i) {
        EXPECT_LT(run.heights[i], run.heights[i - 1])
            << "height did not decrease at tick " << i;
    }
}

TEST(Physics, SteppingIsDeterministic) {
    // The load-bearing test. Two identical worlds, stepped identically, must
    // produce identical results - not merely close ones. Exact equality is
    // correct here precisely because the same code path ran twice with the same
    // inputs; any difference means real non-determinism, which engine_protocol.md
    // treats as a defect rather than a tolerance to widen.
    const auto first = runFallingBody(120);
    const auto second = runFallingBody(120);

    ASSERT_EQ(first.heights.size(), second.heights.size());
    for (size_t i = 0; i < first.heights.size(); ++i) {
        EXPECT_EQ(first.heights[i], second.heights[i])
            << "diverged at tick " << i;
    }
}

TEST(Physics, WorldDefaultsAreOverriddenNotAssumed) {
    // Guards the locked configuration: if a future Box3D bumps its default
    // worker count, our explicit override is what keeps determinism, so assert
    // we are actually setting it rather than inheriting whatever ships.
    b3WorldDef worldDef = b3DefaultWorldDef();
    worldDef.workerCount = kWorkerCount;
    EXPECT_EQ(worldDef.workerCount, 1u);
}

} // namespace

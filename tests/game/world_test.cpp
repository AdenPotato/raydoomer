// The World stub.
//
// Deliberately minimal. The real data model - entities, systems, ownership - is
// ADE-11, and building it here would be speculative generality
// (gameplay_protocol.md). What exists is exactly what the frame loop needs to
// drive something and what a test needs to prove it was driven correctly.
//
// The tests below are about the loop's contract with the world, not about
// gameplay: that ticks arrive, that they arrive the right number of times, and
// that simulated time advances by exactly the fixed step.

#include <gtest/gtest.h>

#include "game/world.h"

namespace {

constexpr float kTick = 1.0f / 60.0f;

TEST(World, StartsWithNoTicksAndNoElapsedTime) {
    game::World world;
    EXPECT_EQ(world.tickCount(), 0);
    EXPECT_FLOAT_EQ(world.elapsedSeconds(), 0.0f);
}

TEST(World, CountsEveryTick) {
    game::World world;

    for (int i = 0; i < 10; ++i) {
        world.tick(kTick);
    }

    EXPECT_EQ(world.tickCount(), 10);
}

TEST(World, AccumulatesSimulatedTimeNotWallClockTime) {
    // Simulated seconds, from the fixed step it was handed. Nothing here reads
    // a clock - that is seam 1, and the world is downstream of it.
    game::World world;

    for (int i = 0; i < 60; ++i) {
        world.tick(kTick);
    }

    EXPECT_NEAR(world.elapsedSeconds(), 1.0f, 1e-4f);
}

TEST(World, IsSteppableWithNoWindowOrDevice) {
    // Seam 3: the rules step headless. If this test ever needs a window, the
    // world has grown a dependency it should not have.
    game::World world;
    world.tick(kTick);
    SUCCEED();
}

} // namespace

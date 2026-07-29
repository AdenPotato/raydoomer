// The World: the data model and its ownership boundaries.
//
// Rewritten from the ADE-10 stub, whose tick took only a delta. The signature is
// now the locked contract from runtime_architecture.md - time, randomness, and
// the event sink all arrive as parameters, so the world never fetches them.
//
// These tests are about ownership and persistence rather than gameplay: that the
// world holds the run's state, that a save is a faithful and deliberately lossy
// projection of it, and that it all steps with no window and no clock.

#include <gtest/gtest.h>

#include "engine/event_dispatch.h"
#include "engine/physics.h"
#include "engine/rng.h"
#include "game/world.h"

namespace {

constexpr float kTick = 1.0f / 60.0f;

// The injected context every tick needs. Built per test: no shared state, so
// two tests in one process cannot contaminate each other (seam 4).
struct Context {
    engine::Rng rng{ 1234 };
    engine::EventDispatcher events;

    Context() { events.freeze(); }
};

TEST(World, StartsWithNoTicksAndNoElapsedTime) {
    game::World world;
    EXPECT_EQ(world.tickCount(), 0);
    EXPECT_FLOAT_EQ(world.elapsedSeconds(), 0.0f);
}

TEST(World, CountsEveryTick) {
    game::World world;
    Context context;

    for (int i = 0; i < 10; ++i) {
        world.tick(kTick, context.rng, context.events);
    }

    EXPECT_EQ(world.tickCount(), 10);
}

TEST(World, AccumulatesSimulatedTimeNotWallClockTime) {
    game::World world;
    Context context;

    for (int i = 0; i < 60; ++i) {
        world.tick(kTick, context.rng, context.events);
    }

    EXPECT_NEAR(world.elapsedSeconds(), 1.0f, 1e-4f);
}

TEST(World, StepsWithNoWindowClockOrDevice) {
    // Seam 3. If this ever needs hardware, the world has grown a dependency it
    // must not have - and it would stop building in the test configuration.
    game::World world;
    Context context;
    world.tick(kTick, context.rng, context.events);
    SUCCEED();
}

TEST(World, OwnsThePlayerInventoryAndProgress) {
    // One place each piece of state lives. A system mutates it through the
    // world rather than holding its own copy.
    game::World world;

    world.player().health = 42;
    world.inventory().stacks.push_back({ game::ItemId{ 9 }, 3 });
    world.progress().currentLevel = 5;
    world.setCurrency(250);

    EXPECT_EQ(world.player().health, 42);
    ASSERT_EQ(world.inventory().stacks.size(), 1u);
    EXPECT_EQ(world.inventory().stacks[0].item.value, 9u);
    EXPECT_EQ(world.progress().currentLevel, 5);
    EXPECT_EQ(world.currency(), 250);
}

TEST(World, ProjectsItsPersistentStateIntoASave) {
    game::World world;
    world.player().health = 55;
    world.player().armor = 10;
    world.inventory().stacks.push_back({ game::ItemId{ 4 }, 2 });
    world.progress().currentLevel = 2;
    world.progress().completedLevels = { 0, 1 };
    world.setCurrency(77);

    const game::SaveData save = world.toSave(0xABCDEF);

    EXPECT_EQ(save.player.health, 55);
    EXPECT_EQ(save.player.armor, 10);
    EXPECT_EQ(save.inventory.stacks.size(), 1u);
    EXPECT_EQ(save.progress.currentLevel, 2);
    EXPECT_EQ(save.progress.completedLevels, (std::vector<int>{ 0, 1 }));
    EXPECT_EQ(save.currency, 77);
    EXPECT_EQ(save.rngSeed, 0xABCDEFu);
}

TEST(World, RestoringASaveReproducesThePersistentState) {
    game::World original;
    original.player().health = 33;
    original.inventory().stacks.push_back({ game::ItemId{ 7 }, 1 });
    original.progress().currentLevel = 4;
    original.setCurrency(11);

    game::World restored;
    restored.restore(original.toSave(0));

    EXPECT_EQ(restored.player(), original.player());
    EXPECT_EQ(restored.inventory(), original.inventory());
    EXPECT_EQ(restored.progress(), original.progress());
    EXPECT_EQ(restored.currency(), original.currency());
}

TEST(World, ASaveIsDeliberatelyLossyAboutLevelScopedState) {
    // Saves happen at level boundaries, so tick count and elapsed time are not
    // persisted. Restoring starts a level rather than resuming one mid-way -
    // the accepted cost of the checkpoint model.
    game::World world;
    Context context;
    for (int i = 0; i < 30; ++i) {
        world.tick(kTick, context.rng, context.events);
    }
    ASSERT_EQ(world.tickCount(), 30);

    world.restore(world.toSave(0));

    EXPECT_EQ(world.tickCount(), 0);
    EXPECT_FLOAT_EQ(world.elapsedSeconds(), 0.0f);
}

TEST(World, StepsPhysicsExactlyOncePerTick) {
    // "Stepped exactly once per fixed tick, never per rendered frame"
    // (locked_decisions.md). Asserted by comparison rather than by counting:
    // a body dropped in the world must land in precisely the same place as one
    // dropped in a standalone physics world stepped the same number of times.
    // Stepping twice per tick, or not at all, changes the answer.
    engine::BodyDef def;
    def.type = engine::BodyType::Dynamic;
    def.position = engine::Point3{ 0.0, 100.0, 0.0 };
    def.shape = engine::SphereShape{ 0.5f };

    game::World world;
    Context context;
    const auto viaWorld = world.physics().createBody(def);

    engine::PhysicsWorld standalone;
    engine::EventDispatcher idle;
    idle.freeze();
    const auto viaStandalone = standalone.createBody(def);

    for (int i = 0; i < 60; ++i) {
        world.tick(kTick, context.rng, context.events);
        standalone.step(kTick, idle);
    }

    EXPECT_EQ(world.physics().position(viaWorld)->y,
              standalone.position(viaStandalone)->y);
}

TEST(World, TickingIsDeterministicForAGivenSeed) {
    // Two worlds, identical seeds, identical tick sequences: identical results.
    // Trivially true today, and the assertion that will catch it stopping being
    // true once systems consume the RNG.
    game::World first;
    game::World second;
    Context firstContext;
    Context secondContext;

    for (int i = 0; i < 100; ++i) {
        first.tick(kTick, firstContext.rng, firstContext.events);
        second.tick(kTick, secondContext.rng, secondContext.events);
    }

    EXPECT_EQ(first.tickCount(), second.tickCount());
    EXPECT_FLOAT_EQ(first.elapsedSeconds(), second.elapsedSeconds());
    EXPECT_EQ(first.player(), second.player());
}

} // namespace

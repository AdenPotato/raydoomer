// The physics wrapper.
//
// Replaces the raw Box3D dependency test from ADE-23: testing the library
// directly is redundant once a wrapper exists, and these tests cover both.
//
// What matters here is not that Box3D works - it does - but that our boundary
// around it holds: opaque handles that detect staleness, engine types rather
// than library types, deterministic stepping, and contacts surfacing as events
// rather than as a callback into gameplay.

#include <gtest/gtest.h>

#include "engine/event_dispatch.h"
#include "engine/physics.h"

#include <string>
#include <vector>

namespace {

constexpr float kTick = 1.0f / 60.0f;

struct ViolationLog {
    std::vector<std::string> messages;
    engine::ViolationReporter reporter() {
        return [this](std::string_view m) { messages.emplace_back(m); };
    }
};

// A dispatcher with no handlers, for tests that do not care about events.
engine::EventDispatcher makeIdleDispatcher() {
    engine::EventDispatcher events;
    events.freeze();
    return events;
}

engine::BodyHandle dropSphere(engine::PhysicsWorld& world, double height) {
    engine::BodyDef def;
    def.type = engine::BodyType::Dynamic;
    def.position = engine::Point3{ 0.0, height, 0.0 };
    def.shape = engine::SphereShape{ 0.5f };
    return world.createBody(def);
}

// --- Behaviour --------------------------------------------------------------

TEST(Physics, ADynamicBodyFallsUnderGravity) {
    engine::PhysicsWorld world;
    auto events = makeIdleDispatcher();
    const auto body = dropSphere(world, 100.0);

    for (int i = 0; i < 60; ++i) {
        world.step(kTick, events);
    }

    const auto position = world.position(body);
    ASSERT_TRUE(position.has_value());

    // One simulated second of free fall is 4.9m analytically; a discrete
    // integrator overshoots slightly. The band is tight enough that a wrong
    // gravity axis or timestep fails, loose enough to survive a solver change.
    const double drop = 100.0 - position->y;
    EXPECT_GT(drop, 4.5);
    EXPECT_LT(drop, 5.5);
}

TEST(Physics, AStaticBodyDoesNotMove) {
    engine::PhysicsWorld world;
    auto events = makeIdleDispatcher();

    engine::BodyDef def;
    def.type = engine::BodyType::Static;
    def.position = engine::Point3{ 0.0, 10.0, 0.0 };
    def.shape = engine::BoxShape{ engine::Vec3{ 5.0f, 0.5f, 5.0f } };
    const auto floor = world.createBody(def);

    for (int i = 0; i < 60; ++i) {
        world.step(kTick, events);
    }

    const auto position = world.position(floor);
    ASSERT_TRUE(position.has_value());
    EXPECT_DOUBLE_EQ(position->y, 10.0);
}

TEST(Physics, SteppingIsDeterministic) {
    // Two worlds, identical construction, identical steps. Exact equality is
    // correct here: the same code path ran twice with the same inputs, so any
    // difference is real non-determinism, which engine_protocol.md treats as a
    // defect rather than a tolerance to widen.
    auto run = [] {
        engine::PhysicsWorld world;
        auto events = makeIdleDispatcher();
        const auto body = dropSphere(world, 50.0);

        std::vector<double> heights;
        for (int i = 0; i < 120; ++i) {
            world.step(kTick, events);
            heights.push_back(world.position(body)->y);
        }
        return heights;
    };

    EXPECT_EQ(run(), run());
}

TEST(Physics, RunsSingleThreadedForDeterminism) {
    // The locked configuration (locked_decisions.md). Box3D's default worker
    // count could change upstream; our explicit override is what preserves
    // reproducibility, so assert we are actually setting it.
    engine::PhysicsWorld world;
    EXPECT_EQ(world.workerCount(), 1u);
}

// --- Handles ----------------------------------------------------------------

TEST(Physics, ADefaultHandleIsNotValid) {
    engine::PhysicsWorld world;
    EXPECT_FALSE(world.isValid(engine::BodyHandle{}));
}

TEST(Physics, AStaleHandleIsDetectedAndReportedRatherThanFollowed) {
    // engine_protocol.md: "A stale handle is detected and reported, never
    // followed." Following one is a use-after-free wearing a handle's clothing.
    ViolationLog log;
    engine::PhysicsWorld world{ engine::Vec3{ 0.0f, -9.8f, 0.0f }, log.reporter() };

    const auto body = dropSphere(world, 10.0);
    world.destroyBody(body);

    EXPECT_FALSE(world.isValid(body));
    EXPECT_FALSE(world.position(body).has_value());
    ASSERT_FALSE(log.messages.empty());
    EXPECT_NE(log.messages[0].find("stale"), std::string::npos);
}

TEST(Physics, ARecycledSlotDoesNotResurrectAnOldHandle) {
    // The failure a plain index would have: destroy a body, create another that
    // reuses the slot, and the old handle silently addresses the new body. The
    // generation counter is what prevents it.
    ViolationLog log;
    engine::PhysicsWorld world{ engine::Vec3{ 0.0f, -9.8f, 0.0f }, log.reporter() };

    const auto first = dropSphere(world, 10.0);
    world.destroyBody(first);
    const auto second = dropSphere(world, 20.0);

    EXPECT_TRUE(world.isValid(second));
    EXPECT_FALSE(world.isValid(first)) << "the old handle addressed the recycled slot";
    EXPECT_NE(first.generation, second.generation);
}

TEST(Physics, DestroyingAHandleTwiceIsReportedNotUndefined) {
    ViolationLog log;
    engine::PhysicsWorld world{ engine::Vec3{ 0.0f, -9.8f, 0.0f }, log.reporter() };

    const auto body = dropSphere(world, 10.0);
    world.destroyBody(body);
    log.messages.clear();

    world.destroyBody(body);

    EXPECT_FALSE(log.messages.empty());
}

TEST(Physics, BodyCountTracksCreationAndDestruction) {
    engine::PhysicsWorld world;

    EXPECT_EQ(world.bodyCount(), 0);
    const auto a = dropSphere(world, 10.0);
    const auto b = dropSphere(world, 20.0);
    EXPECT_EQ(world.bodyCount(), 2);

    world.destroyBody(a);
    EXPECT_EQ(world.bodyCount(), 1);
    world.destroyBody(b);
    EXPECT_EQ(world.bodyCount(), 0);
}

// --- Contacts as events -----------------------------------------------------

TEST(Physics, AContactSurfacesAsAnEventRatherThanACallback) {
    // Physics reports; the game decides. A callback into gameplay would invert
    // the dependency and reach across the engine boundary.
    engine::EventDispatcher events;
    std::vector<engine::ContactBegan> contacts;
    events.subscribe<engine::ContactBegan>(engine::SystemId{ 1 },
                                           [&](const engine::ContactBegan& e) { contacts.push_back(e); });
    events.freeze();

    engine::PhysicsWorld world;

    engine::BodyDef floorDef;
    floorDef.type = engine::BodyType::Static;
    floorDef.position = engine::Point3{ 0.0, 0.0, 0.0 };
    floorDef.shape = engine::BoxShape{ engine::Vec3{ 10.0f, 0.5f, 10.0f } };
    const auto floor = world.createBody(floorDef);

    const auto ball = dropSphere(world, 3.0);

    // Long enough to fall roughly 2.5m and land.
    for (int i = 0; i < 120 && contacts.empty(); ++i) {
        world.step(kTick, events);
    }

    ASSERT_FALSE(contacts.empty()) << "the ball never reported touching the floor";
    const auto& contact = contacts.front();
    const bool involvesBoth = (contact.a == ball && contact.b == floor) ||
                              (contact.a == floor && contact.b == ball);
    EXPECT_TRUE(involvesBoth) << "contact did not name the two bodies that touched";
}

TEST(Physics, NoContactIsReportedWhenNothingTouches) {
    engine::EventDispatcher events;
    int contacts = 0;
    events.subscribe<engine::ContactBegan>(engine::SystemId{ 1 },
                                           [&](const engine::ContactBegan&) { ++contacts; });
    events.freeze();

    engine::PhysicsWorld world;
    (void)dropSphere(world, 1000.0); // nothing to hit

    for (int i = 0; i < 60; ++i) {
        world.step(kTick, events);
    }

    EXPECT_EQ(contacts, 0);
}

TEST(Physics, OptingOutOfContactsIsPairwiseNotPerBody) {
    // Box3D reports a contact if EITHER shape enables events (contact.c uses
    // ||), so opting out only takes effect when both sides do. Verified against
    // the library rather than assumed: the first version of this test opted out
    // one body, expected silence, and got an event.
    //
    // The semantic is the useful one - a projectile still learns it hit a wall
    // that does not care about being hit.
    engine::EventDispatcher events;
    int contacts = 0;
    events.subscribe<engine::ContactBegan>(engine::SystemId{ 1 },
                                           [&](const engine::ContactBegan&) { ++contacts; });
    events.freeze();

    engine::PhysicsWorld world;

    engine::BodyDef floorDef;
    floorDef.type = engine::BodyType::Static;
    floorDef.position = engine::Point3{ 0.0, 0.0, 0.0 };
    floorDef.shape = engine::BoxShape{ engine::Vec3{ 10.0f, 0.5f, 10.0f } };
    floorDef.reportsContacts = false;
    world.createBody(floorDef);

    engine::BodyDef ballDef;
    ballDef.type = engine::BodyType::Dynamic;
    ballDef.position = engine::Point3{ 0.0, 3.0, 0.0 };
    ballDef.shape = engine::SphereShape{ 0.5f };
    ballDef.reportsContacts = false;
    const auto ball = world.createBody(ballDef);

    for (int i = 0; i < 120; ++i) {
        world.step(kTick, events);
    }

    // It still collides - it simply does not announce it.
    EXPECT_EQ(contacts, 0);
    EXPECT_NEAR(world.position(ball)->y, 1.0, 0.05);
}

// --- Lifetime ---------------------------------------------------------------

TEST(Physics, CreateAndDestroyIsAnExercisedCycle) {
    // "Teardown is exercised, not assumed" (engine_protocol.md). Repeated
    // construction would surface a leaked world or a double free.
    for (int i = 0; i < 20; ++i) {
        engine::PhysicsWorld world;
        auto events = makeIdleDispatcher();
        const auto body = dropSphere(world, 5.0);
        world.step(kTick, events);
        world.destroyBody(body);
    }
    SUCCEED();
}

} // namespace


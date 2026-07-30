// The debug renderer: the first thing that puts the simulation on screen.
//
// Drawing is unobservable in a headless test, so what is tested here is the
// decision-making: which primitives get issued, at what size, in what position,
// and interpolated by how much. The renderer interface is mocked, and the
// assertions are about the draw calls rather than about pixels.
//
// That split is the point of the seam. Everything above `platform/` is ordinary
// testable logic; only the final submission touches a device.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "engine/debug_renderer.h"
#include "engine/event_dispatch.h"
#include "engine/physics.h"

namespace {

using ::testing::_;
using ::testing::AtLeast;
using ::testing::NiceMock;

constexpr float kTick = 1.0f / 60.0f;

class MockRenderer : public platform::Renderer {
public:
    MOCK_METHOD(void, beginScene, (const platform::Camera& camera), (override));
    MOCK_METHOD(void, endScene, (), (override));
    MOCK_METHOD(void, drawWireBox,
                (platform::Point3 center, platform::Vec3 halfExtents, platform::Color color),
                (override));
    MOCK_METHOD(void, drawWireSphere,
                (platform::Point3 center, float radius, platform::Color color), (override));
    MOCK_METHOD(void, drawSolidBox,
                (platform::Point3 center, platform::Vec3 halfExtents, platform::Color color),
                (override));
    MOCK_METHOD(void, drawSolidSphere,
                (platform::Point3 center, float radius, platform::Color color), (override));
    MOCK_METHOD(void, drawLine,
                (platform::Point3 from, platform::Point3 to, platform::Color color), (override));
    MOCK_METHOD(void, drawPoint,
                (platform::Point3 position, float size, platform::Color color), (override));
};

// Puts one sphere and one box in the world, so a mode test covers both
// primitives at once.
void addOneOfEachShape(engine::PhysicsWorld& world) {
    engine::BodyDef sphere;
    sphere.type = engine::BodyType::Static;
    sphere.shape = engine::SphereShape{ 0.5f };
    world.createBody(sphere);

    engine::BodyDef box;
    box.type = engine::BodyType::Static;
    box.position = engine::Point3{ 4.0, 0.0, 0.0 };
    box.shape = engine::BoxShape{ engine::Vec3{ 1.0f, 1.0f, 1.0f } };
    world.createBody(box);
}

engine::EventDispatcher idleDispatcher() {
    engine::EventDispatcher events;
    events.freeze();
    return events;
}

TEST(DebugRenderer, DefaultsToTheBuildTypesSetting) {
    // On in a development build, off in a release build. A diagnostic view must
    // never ship on by accident, and equally must not need switching on by hand
    // every run during development.
    //
    // Asserted against the build type rather than a hardcoded expectation, so
    // this test is meaningful in both configurations.
    engine::DebugRenderer debug;
#ifdef NDEBUG
    EXPECT_FALSE(debug.isEnabled()) << "the debug view is on in a release build";
#else
    EXPECT_TRUE(debug.isEnabled()) << "the debug view is off in a development build";
#endif
    EXPECT_EQ(debug.isEnabled(), engine::DebugRenderer::defaultEnabled());
}

TEST(DebugRenderer, DrawsNothingWhenDisabled) {
    // Off by default outside a development build. A diagnostic view that cannot
    // be switched off is not a diagnostic, it is a feature.
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    engine::BodyDef def;
    def.shape = engine::SphereShape{ 0.5f };
    world.createBody(def);

    engine::DebugRenderer debug;
    debug.setEnabled(false);

    EXPECT_CALL(renderer, drawWireSphere(_, _, _)).Times(0);
    EXPECT_CALL(renderer, beginScene(_)).Times(0);

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, OpensAndClosesTheSceneExactlyOnce) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    engine::DebugRenderer debug;
    debug.setEnabled(true);

    EXPECT_CALL(renderer, beginScene(_)).Times(1);
    EXPECT_CALL(renderer, endScene()).Times(1);

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, DrawsASphereBodyAsAWireSphereOfMatchingRadius) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;

    engine::BodyDef def;
    def.type = engine::BodyType::Static;
    def.position = engine::Point3{ 1.0, 2.0, 3.0 };
    def.shape = engine::SphereShape{ 0.75f };
    world.createBody(def);

    engine::DebugRenderer debug;
    debug.setEnabled(true);

    EXPECT_CALL(renderer, drawWireSphere(platform::Point3{ 1.0, 2.0, 3.0 }, 0.75f, _)).Times(1);
    EXPECT_CALL(renderer, drawWireBox(_, _, _)).Times(0);

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, DrawsABoxBodyAsAWireBoxOfMatchingExtents) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;

    engine::BodyDef def;
    def.type = engine::BodyType::Static;
    def.position = engine::Point3{ 0.0, -1.0, 0.0 };
    def.shape = engine::BoxShape{ engine::Vec3{ 5.0f, 0.5f, 5.0f } };
    world.createBody(def);

    engine::DebugRenderer debug;
    debug.setEnabled(true);

    EXPECT_CALL(renderer,
                drawWireBox(platform::Point3{ 0.0, -1.0, 0.0 },
                            platform::Vec3{ 5.0f, 0.5f, 5.0f }, _))
        .Times(1);
    EXPECT_CALL(renderer, drawWireSphere(_, _, _)).Times(0);

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, DrawsEveryBodyInTheWorld) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;

    for (int i = 0; i < 5; ++i) {
        engine::BodyDef def;
        def.type = engine::BodyType::Static;
        def.position = engine::Point3{ static_cast<double>(i), 0.0, 0.0 };
        def.shape = engine::SphereShape{ 0.5f };
        world.createBody(def);
    }

    engine::DebugRenderer debug;
    debug.setEnabled(true);

    EXPECT_CALL(renderer, drawWireSphere(_, _, _)).Times(5);

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, DoesNotDrawADestroyedBody) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;

    engine::BodyDef def;
    def.type = engine::BodyType::Static;
    def.shape = engine::SphereShape{ 0.5f };
    const auto keep = world.createBody(def);
    const auto discard = world.createBody(def);
    world.destroyBody(discard);
    (void)keep;

    engine::DebugRenderer debug;
    debug.setEnabled(true);

    EXPECT_CALL(renderer, drawWireSphere(_, _, _)).Times(1);

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, DrawsFromInterpolatedPositionNotRawState) {
    // The property that makes motion smooth when the tick and the frame
    // disagree. Drawing raw current state visibly stutters at 144fps against a
    // 60Hz simulation, which is the common case.
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    auto events = idleDispatcher();

    engine::BodyDef def;
    def.type = engine::BodyType::Dynamic;
    def.position = engine::Point3{ 0.0, 100.0, 0.0 };
    def.shape = engine::SphereShape{ 0.5f };
    const auto body = world.createBody(def);

    // Two steps, so there is a previous and a current position to blend.
    world.step(kTick, events);
    const double first = world.position(body)->y;
    world.step(kTick, events);
    const double second = world.position(body)->y;

    ASSERT_LT(second, first) << "the body must have moved between steps";

    engine::DebugRenderer debug;
    debug.setEnabled(true);

    // Halfway between the two states.
    const double expected = first + (second - first) * 0.5;

    EXPECT_CALL(renderer, drawWireSphere(_, _, _))
        .WillOnce([&](platform::Point3 center, float, platform::Color) {
            EXPECT_NEAR(center.y, expected, 1e-9);
        });

    debug.draw(renderer, world, 0.5);
}

TEST(DebugRenderer, AlphaOfZeroDrawsThePreviousState) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    auto events = idleDispatcher();

    engine::BodyDef def;
    def.type = engine::BodyType::Dynamic;
    def.position = engine::Point3{ 0.0, 100.0, 0.0 };
    def.shape = engine::SphereShape{ 0.5f };
    const auto body = world.createBody(def);

    world.step(kTick, events);
    const double previous = world.position(body)->y;
    world.step(kTick, events);

    engine::DebugRenderer debug;
    debug.setEnabled(true);

    EXPECT_CALL(renderer, drawWireSphere(_, _, _))
        .WillOnce([&](platform::Point3 center, float, platform::Color) {
            EXPECT_NEAR(center.y, previous, 1e-9);
        });

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, DrawingDoesNotChangeSimulationState) {
    // Presentation observes; it never decides. Drawing twice must leave the
    // world byte-identical, or a rule has leaked into the renderer.
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    auto events = idleDispatcher();

    engine::BodyDef def;
    def.type = engine::BodyType::Dynamic;
    def.position = engine::Point3{ 0.0, 10.0, 0.0 };
    def.shape = engine::SphereShape{ 0.5f };
    const auto body = world.createBody(def);
    world.step(kTick, events);

    engine::DebugRenderer debug;
    debug.setEnabled(true);

    const double before = world.position(body)->y;
    debug.draw(renderer, world, 0.5);
    debug.draw(renderer, world, 0.5);
    const double after = world.position(body)->y;

    EXPECT_DOUBLE_EQ(before, after);
    EXPECT_EQ(world.bodyCount(), 1);
}

// --- Draw modes -------------------------------------------------------------

TEST(DebugRenderer, DefaultsToWireframe) {
    engine::DebugRenderer debug;
    EXPECT_EQ(debug.drawMode(), engine::DebugDrawMode::Wireframe);
}

TEST(DebugRenderer, WireframeModeDrawsOnlyWireframes) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    addOneOfEachShape(world);

    engine::DebugRenderer debug;
    debug.setEnabled(true);
    debug.setDrawMode(engine::DebugDrawMode::Wireframe);

    EXPECT_CALL(renderer, drawWireSphere(_, _, _)).Times(1);
    EXPECT_CALL(renderer, drawWireBox(_, _, _)).Times(1);
    EXPECT_CALL(renderer, drawSolidSphere(_, _, _)).Times(0);
    EXPECT_CALL(renderer, drawSolidBox(_, _, _)).Times(0);

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, SolidModeDrawsOnlySolids) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    addOneOfEachShape(world);

    engine::DebugRenderer debug;
    debug.setEnabled(true);
    debug.setDrawMode(engine::DebugDrawMode::Solid);

    EXPECT_CALL(renderer, drawSolidSphere(_, _, _)).Times(1);
    EXPECT_CALL(renderer, drawSolidBox(_, _, _)).Times(1);
    EXPECT_CALL(renderer, drawWireSphere(_, _, _)).Times(0);
    EXPECT_CALL(renderer, drawWireBox(_, _, _)).Times(0);

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, BothModeDrawsSolidsAndWireframes) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    addOneOfEachShape(world);

    engine::DebugRenderer debug;
    debug.setEnabled(true);
    debug.setDrawMode(engine::DebugDrawMode::Both);

    EXPECT_CALL(renderer, drawSolidSphere(_, _, _)).Times(1);
    EXPECT_CALL(renderer, drawSolidBox(_, _, _)).Times(1);
    EXPECT_CALL(renderer, drawWireSphere(_, _, _)).Times(1);
    EXPECT_CALL(renderer, drawWireBox(_, _, _)).Times(1);

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, BothModeDrawsTheSolidBeforeTheWireframe) {
    // Order is the whole point of this mode: a wireframe drawn under a solid is
    // hidden by it, which would make the mode indistinguishable from Solid.
    MockRenderer renderer;
    engine::PhysicsWorld world;

    engine::BodyDef sphere;
    sphere.type = engine::BodyType::Static;
    sphere.shape = engine::SphereShape{ 0.5f };
    world.createBody(sphere);

    engine::DebugRenderer debug;
    debug.setEnabled(true);
    debug.setDrawMode(engine::DebugDrawMode::Both);

    ::testing::InSequence sequence;
    EXPECT_CALL(renderer, beginScene(_));
    EXPECT_CALL(renderer, drawSolidSphere(_, _, _));
    EXPECT_CALL(renderer, drawWireSphere(_, _, _));
    EXPECT_CALL(renderer, endScene());

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, CyclingTheModeVisitsEveryModeAndReturnsToTheStart) {
    // One key cycles all three, so it must be a closed loop with no dead end.
    engine::DebugRenderer debug;
    ASSERT_EQ(debug.drawMode(), engine::DebugDrawMode::Wireframe);

    debug.cycleDrawMode();
    EXPECT_EQ(debug.drawMode(), engine::DebugDrawMode::Solid);

    debug.cycleDrawMode();
    EXPECT_EQ(debug.drawMode(), engine::DebugDrawMode::Both);

    debug.cycleDrawMode();
    EXPECT_EQ(debug.drawMode(), engine::DebugDrawMode::Wireframe);
}

TEST(DebugRenderer, ChangingModeDoesNotChangeWhetherItIsEnabled) {
    // The two toggles are independent: cycling the mode while hidden must not
    // reveal the view, and vice versa.
    engine::DebugRenderer debug;
    debug.setEnabled(false);

    debug.cycleDrawMode();
    debug.cycleDrawMode();

    EXPECT_FALSE(debug.isEnabled());
}

TEST(DebugRenderer, SolidModeStillDrawsNothingWhenDisabled) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    addOneOfEachShape(world);

    engine::DebugRenderer debug;
    debug.setEnabled(false);
    debug.setDrawMode(engine::DebugDrawMode::Solid);

    EXPECT_CALL(renderer, drawSolidSphere(_, _, _)).Times(0);
    EXPECT_CALL(renderer, drawSolidBox(_, _, _)).Times(0);

    debug.draw(renderer, world, 0.0);
}

// --- Solver overlay ---------------------------------------------------------

TEST(DebugRenderer, SolverOverlayIsOffByDefault) {
    engine::DebugRenderer debug;
    EXPECT_FALSE(debug.isSolverOverlayEnabled());
}

TEST(DebugRenderer, SolverOverlayDrawsNothingExtraWhenOff) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    addOneOfEachShape(world);

    engine::DebugRenderer debug;
    debug.setEnabled(true);
    debug.setSolverOverlayEnabled(false);

    // Solver output arrives as segments and points; the interpolated view uses
    // neither, so their absence is a clean signal that the overlay stayed off.
    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(0);
    EXPECT_CALL(renderer, drawPoint(_, _, _)).Times(0);

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, SolverOverlayDrawsContactsWhenBodiesTouch) {
    // The whole point of the overlay: contact points come from Box3D and cannot
    // be derived from our own record of the world.
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    auto events = idleDispatcher();

    engine::BodyDef floorDef;
    floorDef.type = engine::BodyType::Static;
    floorDef.position = engine::Point3{ 0.0, 0.0, 0.0 };
    floorDef.shape = engine::BoxShape{ engine::Vec3{ 10.0f, 0.5f, 10.0f } };
    world.createBody(floorDef);

    engine::BodyDef ballDef;
    ballDef.type = engine::BodyType::Dynamic;
    ballDef.position = engine::Point3{ 0.0, 2.0, 0.0 };
    ballDef.shape = engine::SphereShape{ 0.5f };
    world.createBody(ballDef);

    // Land the ball, but do not let it settle. A 1m fall takes about 27 ticks;
    // 40 leaves it freshly in contact and still awake.
    //
    // This matters: Box3D puts resting bodies to sleep, and a sleeping body
    // stops reporting contacts. Stepping 120 ticks here produced zero solver
    // output and looked exactly like a broken overlay.
    for (int i = 0; i < 40; ++i) {
        world.step(kTick, events);
    }

    engine::DebugRenderer debug;
    debug.setEnabled(true);
    debug.setSolverOverlayEnabled(true);

    int solverPrimitives = 0;
    ON_CALL(renderer, drawPoint(_, _, _)).WillByDefault([&](auto...) { ++solverPrimitives; });
    ON_CALL(renderer, drawLine(_, _, _)).WillByDefault([&](auto...) { ++solverPrimitives; });

    debug.draw(renderer, world, 0.0);

    EXPECT_GT(solverPrimitives, 0) << "a resting contact produced no solver output";
}

TEST(DebugRenderer, SolverOverlayIsIndependentOfDrawMode) {
    // Two separate toggles. Cycling the draw mode must not disturb the overlay.
    engine::DebugRenderer debug;
    debug.setSolverOverlayEnabled(true);

    debug.cycleDrawMode();
    debug.cycleDrawMode();

    EXPECT_TRUE(debug.isSolverOverlayEnabled());
}

TEST(DebugRenderer, SolverOverlayRespectsTheMasterToggle) {
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;
    addOneOfEachShape(world);

    engine::DebugRenderer debug;
    debug.setEnabled(false);
    debug.setSolverOverlayEnabled(true);

    EXPECT_CALL(renderer, beginScene(_)).Times(0);
    EXPECT_CALL(renderer, drawLine(_, _, _)).Times(0);
    EXPECT_CALL(renderer, drawPoint(_, _, _)).Times(0);

    debug.draw(renderer, world, 0.0);
}

TEST(DebugRenderer, StaticAndDynamicBodiesAreVisuallyDistinct) {
    // Being able to tell level geometry from an actor at a glance is most of
    // this view's value.
    NiceMock<MockRenderer> renderer;
    engine::PhysicsWorld world;

    engine::BodyDef staticDef;
    staticDef.type = engine::BodyType::Static;
    staticDef.shape = engine::SphereShape{ 0.5f };
    world.createBody(staticDef);

    engine::BodyDef dynamicDef;
    dynamicDef.type = engine::BodyType::Dynamic;
    dynamicDef.position = engine::Point3{ 5.0, 0.0, 0.0 };
    dynamicDef.shape = engine::SphereShape{ 0.5f };
    world.createBody(dynamicDef);

    engine::DebugRenderer debug;
    debug.setEnabled(true);

    std::vector<platform::Color> colors;
    EXPECT_CALL(renderer, drawWireSphere(_, _, _))
        .Times(2)
        .WillRepeatedly([&](platform::Point3, float, platform::Color color) {
            colors.push_back(color);
        });

    debug.draw(renderer, world, 0.0);

    ASSERT_EQ(colors.size(), 2u);
    EXPECT_NE(colors[0], colors[1]) << "static and dynamic bodies drew identically";
}

} // namespace

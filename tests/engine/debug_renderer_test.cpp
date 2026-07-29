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
};

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

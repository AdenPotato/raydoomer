// The first-person camera.
//
// Pure maths: given an eye position and a look angle, produce a camera. No
// state, no device, so it is fully testable headless - which is the point of
// keeping it out of the render backend.
//
// The camera is presentation. Its position derives from the player's simulated
// position, and nothing in the rules may read it back
// (presentation_protocol.md).

#include <gtest/gtest.h>

#include "engine/camera.h"

#include <cmath>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi;

platform::Vec3 lookDirection(const platform::Camera& camera) {
    return platform::Vec3{
        static_cast<float>(camera.target.x - camera.position.x),
        static_cast<float>(camera.target.y - camera.position.y),
        static_cast<float>(camera.target.z - camera.position.z),
    };
}

TEST(Camera, SitsAtTheEyePositionItIsGiven) {
    const auto camera = engine::firstPersonCamera(platform::Point3{ 1.0, 2.0, 3.0 },
                                                  0.0f, 0.0f, 90.0f);
    EXPECT_EQ(camera.position, (platform::Point3{ 1.0, 2.0, 3.0 }));
}

TEST(Camera, YawZeroLooksAlongPositiveZ) {
    // Matches the movement controller's convention: forward is +Z at yaw 0.
    // If these two disagree, walking forward and looking forward diverge, which
    // is disorienting in a way that is hard to attribute.
    const auto camera = engine::firstPersonCamera(platform::Point3{}, 0.0f, 0.0f, 90.0f);
    const auto direction = lookDirection(camera);

    EXPECT_NEAR(direction.x, 0.0f, 1e-5f);
    EXPECT_NEAR(direction.y, 0.0f, 1e-5f);
    EXPECT_NEAR(direction.z, 1.0f, 1e-5f);
}

TEST(Camera, YawTurnsHorizontallyInTheSameDirectionAsMovement) {
    // A quarter turn must send the view the same way a quarter turn sends
    // movement. Getting the sign wrong here means looking left walks right.
    const auto camera =
        engine::firstPersonCamera(platform::Point3{}, static_cast<float>(kPi / 2.0), 0.0f, 90.0f);
    const auto direction = lookDirection(camera);

    EXPECT_NEAR(direction.x, 1.0f, 1e-5f);
    EXPECT_NEAR(direction.z, 0.0f, 1e-5f);
}

TEST(Camera, PositivePitchLooksUp) {
    const auto camera = engine::firstPersonCamera(platform::Point3{}, 0.0f, 0.5f, 90.0f);
    EXPECT_GT(lookDirection(camera).y, 0.0f);
}

TEST(Camera, NegativePitchLooksDown) {
    const auto camera = engine::firstPersonCamera(platform::Point3{}, 0.0f, -0.5f, 90.0f);
    EXPECT_LT(lookDirection(camera).y, 0.0f);
}

TEST(Camera, TheLookDirectionIsAlwaysUnitLength) {
    // The target sits one metre ahead, so the direction is a unit vector. A
    // shorter or longer one would make FOV behave oddly and any future use of
    // it as an aim ray subtly wrong.
    for (double yaw = -4.0; yaw <= 4.0; yaw += 0.37) {
        for (double pitch = -1.5; pitch <= 1.5; pitch += 0.31) {
            const auto camera = engine::firstPersonCamera(
                platform::Point3{}, static_cast<float>(yaw), static_cast<float>(pitch), 90.0f);
            const auto d = lookDirection(camera);
            const float length = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
            EXPECT_NEAR(length, 1.0f, 1e-4f) << "yaw " << yaw << " pitch " << pitch;
        }
    }
}

TEST(Camera, LookingStraightUpDoesNotProduceADegenerateCamera) {
    // Straight up makes the view direction parallel to the world up vector,
    // which collapses the usual basis. The caller clamps pitch to avoid it, but
    // the maths must not produce NaN if it ever arrives.
    const auto camera = engine::firstPersonCamera(platform::Point3{}, 0.0f,
                                                  static_cast<float>(kPi / 2.0), 90.0f);
    const auto d = lookDirection(camera);

    EXPECT_FALSE(std::isnan(d.x));
    EXPECT_FALSE(std::isnan(d.y));
    EXPECT_FALSE(std::isnan(d.z));
}

TEST(Camera, CarriesTheFieldOfViewItIsGiven) {
    // FOV is a tunable, so it must pass through rather than being fixed here.
    const auto camera = engine::firstPersonCamera(platform::Point3{}, 0.0f, 0.0f, 103.0f);
    EXPECT_FLOAT_EQ(camera.fovDegrees, 103.0f);
}

TEST(Camera, EyePositionIsOffsetAboveTheBodyCentre) {
    // The camera belongs at eye level, not at the body's centre of mass, or the
    // player appears to be looking out of their own chest.
    const platform::Point3 body{ 0.0, 1.0, 0.0 };
    const auto eye = engine::eyePosition(body, /*eyeHeight=*/0.6f);

    EXPECT_DOUBLE_EQ(eye.x, body.x);
    EXPECT_DOUBLE_EQ(eye.z, body.z);
    // Float tolerance, not double: the offset is a float, so promoting it gives
    // 0.60000002... Asserting double equality against a double literal here
    // tests the compiler's rounding rather than the code.
    EXPECT_NEAR(eye.y, body.y + 0.6, 1e-6);
}

TEST(Camera, IsAPureFunctionOfItsInputs) {
    // Same inputs, same camera, every time. A camera that drifts would make
    // golden-image tests unrepeatable.
    const auto a = engine::firstPersonCamera(platform::Point3{ 1.0, 2.0, 3.0 }, 0.7f, -0.2f, 90.0f);
    const auto b = engine::firstPersonCamera(platform::Point3{ 1.0, 2.0, 3.0 }, 0.7f, -0.2f, 90.0f);

    EXPECT_EQ(a.position, b.position);
    EXPECT_EQ(a.target, b.target);
    EXPECT_FLOAT_EQ(a.fovDegrees, b.fovDegrees);
}

} // namespace

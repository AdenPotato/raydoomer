// Billboard orientation.
//
// This is the part of sprite rendering that is *maths* rather than drawing, so
// it lives in engine/ and gets real tests. Buried in the raylib backend it
// would be unreachable from a headless suite, and every later sprite bug would
// be diagnosed by squinting at the screen.
//
// Two separate jobs live here:
//   - orienting a quad to face the camera (this file's main concern)
//   - choosing which directional frame to show, from the angle between the
//     viewer and the actor's facing

#include <gtest/gtest.h>

#include "engine/billboard.h"

#include <cmath>
#include <numbers>

namespace {

constexpr double kPi = std::numbers::pi;

// The right vector of a Y-axis billboard should be perpendicular to the
// direction from sprite to camera, and level with the horizon.
TEST(Billboard, RightVectorIsPerpendicularToTheViewDirection) {
    const platform::Point3 sprite{ 0.0, 0.0, 0.0 };
    const platform::Point3 camera{ 10.0, 0.0, 0.0 };

    const platform::Vec3 right = engine::billboardRight(sprite, camera);

    // Camera is along +X, so the sprite's right runs along Z.
    EXPECT_NEAR(right.x, 0.0f, 1e-5f);
    EXPECT_NEAR(std::abs(right.z), 1.0f, 1e-5f);
}

TEST(Billboard, RightVectorIsAlwaysLevelWithTheHorizon) {
    // Yaw only. Rolling a sprite to face a pitched camera looks wrong for this
    // style, and #21 calls it out explicitly.
    const platform::Point3 sprite{ 0.0, 0.0, 0.0 };

    for (double height = -20.0; height <= 20.0; height += 5.0) {
        const platform::Point3 camera{ 7.0, height, 3.0 };
        const platform::Vec3 right = engine::billboardRight(sprite, camera);
        EXPECT_NEAR(right.y, 0.0f, 1e-5f) << "camera height " << height;
    }
}

TEST(Billboard, RightVectorIsNormalised) {
    const platform::Point3 sprite{ 1.0, 2.0, 3.0 };
    const platform::Point3 camera{ -4.0, 9.0, 2.0 };

    const platform::Vec3 right = engine::billboardRight(sprite, camera);
    const float length =
        std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);

    EXPECT_NEAR(length, 1.0f, 1e-5f);
}

TEST(Billboard, ACameraDirectlyAboveDoesNotProduceANaNOrientation) {
    // The degenerate case: the sprite-to-camera vector is straight up, so its
    // horizontal projection is zero and the obvious cross product collapses.
    // Returning garbage here would put NaNs into vertex data and blank the
    // screen, which is a miserable thing to debug.
    const platform::Point3 sprite{ 0.0, 0.0, 0.0 };
    const platform::Point3 camera{ 0.0, 10.0, 0.0 };

    const platform::Vec3 right = engine::billboardRight(sprite, camera);

    EXPECT_FALSE(std::isnan(right.x));
    EXPECT_FALSE(std::isnan(right.y));
    EXPECT_FALSE(std::isnan(right.z));

    const float length =
        std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    EXPECT_NEAR(length, 1.0f, 1e-5f) << "degenerate case produced a non-unit vector";
}

TEST(Billboard, ACameraAtTheSpritePositionDoesNotProduceANaNOrientation) {
    const platform::Point3 sprite{ 5.0, 5.0, 5.0 };
    const platform::Vec3 right = engine::billboardRight(sprite, sprite);

    EXPECT_FALSE(std::isnan(right.x));
    const float length =
        std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    EXPECT_NEAR(length, 1.0f, 1e-5f);
}

// --- Directional frame selection --------------------------------------------

TEST(Billboard, FacingTheViewerSelectsTheFrontFrame) {
    // Actor facing +Z, viewer in front of it at +Z: frame 0, the front.
    const int frame = engine::directionalFrame(/*actorYawRadians=*/0.0f,
                                               /*viewerAngleRadians=*/0.0f,
                                               /*frameCount=*/8);
    EXPECT_EQ(frame, 0);
}

TEST(Billboard, FacingAwaySelectsTheBackFrame) {
    // Half a turn away, with 8 frames, is frame 4.
    const int frame = engine::directionalFrame(0.0f, static_cast<float>(kPi), 8);
    EXPECT_EQ(frame, 4);
}

TEST(Billboard, FrameSelectionIsAlwaysInRange) {
    // Angles wrap, and the arithmetic must wrap with them. An out-of-range index
    // here is a read past the end of the sprite atlas.
    for (int frameCount : { 1, 4, 8, 16 }) {
        for (double angle = -8.0; angle <= 8.0; angle += 0.13) {
            const int frame =
                engine::directionalFrame(0.0f, static_cast<float>(angle), frameCount);
            EXPECT_GE(frame, 0) << "angle " << angle << " count " << frameCount;
            EXPECT_LT(frame, frameCount) << "angle " << angle << " count " << frameCount;
        }
    }
}

TEST(Billboard, FrameSelectionUsesTheRelativeAngleNotTheAbsoluteOne) {
    // Rotating actor and viewer together must not change which frame shows -
    // it is the angle *between* them that matters.
    const int reference = engine::directionalFrame(0.0f, 0.0f, 8);

    for (double turn = 0.0; turn < 2.0 * kPi; turn += 0.4) {
        const int rotated = engine::directionalFrame(static_cast<float>(turn),
                                                     static_cast<float>(turn), 8);
        EXPECT_EQ(rotated, reference) << "turn " << turn;
    }
}

TEST(Billboard, ASingleFrameSpriteAlwaysSelectsFrameZero) {
    for (double angle = -4.0; angle <= 4.0; angle += 0.37) {
        EXPECT_EQ(engine::directionalFrame(0.0f, static_cast<float>(angle), 1), 0);
    }
}

TEST(Billboard, FrameBoundariesAreCentredNotOffset) {
    // With 8 frames each covers 45 degrees, and frame 0 should be centred on
    // "directly facing" - spanning -22.5 to +22.5 - rather than starting there.
    // Off-by-half-a-sector is the classic bug and it makes sprites appear to
    // snap early as an actor turns.
    constexpr double kEighth = 2.0 * kPi / 8.0;

    EXPECT_EQ(engine::directionalFrame(0.0f, static_cast<float>(kEighth * 0.4), 8), 0);
    EXPECT_EQ(engine::directionalFrame(0.0f, static_cast<float>(-kEighth * 0.4), 8), 0);
    EXPECT_EQ(engine::directionalFrame(0.0f, static_cast<float>(kEighth * 0.6), 8), 1);
}

} // namespace

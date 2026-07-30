// The renderer seam's own contract.
//
// The interface is declarations, so what is testable here is the semantics
// around it: that handles behave, that the recording double is a faithful stand
// in, and that scene and overlay scopes pair. Everything downstream - #18, #21,
// #23 - is written against these guarantees.

#include <gtest/gtest.h>

#include "support/recording_renderer.h"

#include <array>
#include <cstdint>

namespace {

std::array<platform::Vertex, 3> triangle() {
    return { platform::Vertex{ { 0, 0, 0 }, { 0, 1, 0 }, { 0, 0 } },
             platform::Vertex{ { 1, 0, 0 }, { 0, 1, 0 }, { 1, 0 } },
             platform::Vertex{ { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1 } } };
}

// --- Handles ----------------------------------------------------------------

TEST(RendererSeam, ADefaultMeshHandleIsInvalid) {
    // Generation 0 means "never referred to anything", so a zeroed struct can
    // never accidentally address slot 0.
    const platform::MeshHandle handle;
    EXPECT_EQ(handle.generation, 0u);
}

TEST(RendererSeam, ADefaultTextureHandleIsInvalid) {
    const platform::TextureHandle handle;
    EXPECT_EQ(handle.generation, 0u);
}

TEST(RendererSeam, CreatedHandlesAreDistinct) {
    support::RecordingRenderer renderer;
    const std::array<uint16_t, 3> indices{ 0, 1, 2 };

    const auto first = renderer.createMesh(triangle(), indices);
    const auto second = renderer.createMesh(triangle(), indices);

    EXPECT_NE(first, second);
    EXPECT_NE(first.generation, 0u);
    EXPECT_NE(second.generation, 0u);
}

TEST(RendererSeam, HandlesCompareByValue) {
    const platform::TextureHandle a{ 3, 1 };
    const platform::TextureHandle b{ 3, 1 };
    const platform::TextureHandle differentGeneration{ 3, 2 };

    EXPECT_EQ(a, b);
    EXPECT_NE(a, differentGeneration);
}

// --- Resource lifetime ------------------------------------------------------

TEST(RendererSeam, CreatingAndDestroyingResourcesBalances) {
    // Whoever creates destroys. A consumer that leaks will show up here as a
    // non-zero count, which is the cheapest possible leak test.
    support::RecordingRenderer renderer;
    const std::array<uint16_t, 3> indices{ 0, 1, 2 };
    const std::array<uint8_t, 4> pixel{ 255, 0, 0, 255 };

    const auto mesh = renderer.createMesh(triangle(), indices);
    const auto texture = renderer.createTexture(1, 1, pixel);
    EXPECT_EQ(renderer.liveMeshes, 1);
    EXPECT_EQ(renderer.liveTextures, 1);

    renderer.destroyMesh(mesh);
    renderer.destroyTexture(texture);
    EXPECT_EQ(renderer.liveMeshes, 0);
    EXPECT_EQ(renderer.liveTextures, 0);
}

TEST(RendererSeam, DestroyingADefaultHandleIsHarmless) {
    // A consumer that cleans up unconditionally must not drive the count
    // negative just because one resource was never created.
    support::RecordingRenderer renderer;

    renderer.destroyMesh(platform::MeshHandle{});
    renderer.destroyTexture(platform::TextureHandle{});

    EXPECT_EQ(renderer.liveMeshes, 0);
    EXPECT_EQ(renderer.liveTextures, 0);
}

// --- Scopes -----------------------------------------------------------------

TEST(RendererSeam, SceneScopesPair) {
    support::RecordingRenderer renderer;

    renderer.beginScene(platform::Camera{});
    EXPECT_EQ(renderer.sceneDepth, 1);
    renderer.endScene();
    EXPECT_EQ(renderer.sceneDepth, 0);
}

TEST(RendererSeam, OverlayScopesPair) {
    support::RecordingRenderer renderer;

    renderer.begin2D();
    EXPECT_EQ(renderer.overlayDepth, 1);
    renderer.end2D();
    EXPECT_EQ(renderer.overlayDepth, 0);
}

// --- The recording double ---------------------------------------------------

TEST(RendererSeam, TheRecorderCapturesMeshDrawsInOrder) {
    support::RecordingRenderer renderer;
    const std::array<uint16_t, 3> indices{ 0, 1, 2 };
    const auto mesh = renderer.createMesh(triangle(), indices);

    platform::Transform first;
    first.position = platform::Point3{ 1.0, 0.0, 0.0 };
    platform::Transform second;
    second.position = platform::Point3{ 2.0, 0.0, 0.0 };

    renderer.drawMesh(mesh, first, platform::TextureHandle{}, platform::Color{});
    renderer.drawMesh(mesh, second, platform::TextureHandle{}, platform::Color{});

    ASSERT_EQ(renderer.meshDraws.size(), 2u);
    EXPECT_EQ(renderer.meshDraws[0].transform.position.x, 1.0);
    EXPECT_EQ(renderer.meshDraws[1].transform.position.x, 2.0);
}

TEST(RendererSeam, TheRecorderCapturesSpriteDraws) {
    support::RecordingRenderer renderer;

    renderer.drawSprite(platform::TextureHandle{ 1, 1 }, platform::Point3{ 3.0, 1.0, 0.0 },
                        platform::Vec2{ 2.0f, 2.0f }, platform::Rect{ 0, 0, 0.5f, 1.0f },
                        platform::Color{ 255, 255, 255, 255 });

    ASSERT_EQ(renderer.spriteDraws.size(), 1u);
    EXPECT_EQ(renderer.spriteDraws[0].position.x, 3.0);
    EXPECT_EQ(renderer.spriteDraws[0].uvRect.width, 0.5f);
}

TEST(RendererSeam, TheRecorderCapturesTextWithItsContent) {
    // Copies the string rather than holding the view: a caller passing a
    // temporary would otherwise leave a dangling reference in the log.
    support::RecordingRenderer renderer;

    renderer.drawText(std::string("health: 73"), platform::Vec2{ 10.0f, 10.0f }, 20.0f,
                      platform::Color{});

    ASSERT_EQ(renderer.textDraws.size(), 1u);
    EXPECT_EQ(renderer.textDraws[0].text, "health: 73");
}

TEST(RendererSeam, TheViewportSizeIsControllable) {
    // Layout code must be testable at the smallest supported resolution, which
    // is where overflow shows up (presentation_protocol.md).
    support::RecordingRenderer renderer;
    renderer.viewport = platform::Vec2{ 640.0f, 480.0f };

    EXPECT_EQ(renderer.viewportSize(), (platform::Vec2{ 640.0f, 480.0f }));
}

TEST(RendererSeam, ClearingTheRecorderKeepsResourceCounts) {
    // Clearing the draw log between frames must not look like every resource
    // was freed.
    support::RecordingRenderer renderer;
    const std::array<uint8_t, 4> pixel{ 0, 0, 0, 255 };
    (void)renderer.createTexture(1, 1, pixel);
    renderer.drawRect(platform::Rect{ 0, 0, 10, 10 }, platform::Color{});

    renderer.clear();

    EXPECT_TRUE(renderer.rectDraws.empty());
    EXPECT_EQ(renderer.liveTextures, 1);
}

} // namespace

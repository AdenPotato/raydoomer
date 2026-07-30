#pragma once

#include <gmock/gmock.h>

#include "platform/renderer.h"

namespace support {

/// gmock double for the full renderer interface.
///
/// @remarks
/// Shared rather than redeclared per test file: the interface has fifteen
/// methods, and a copy in each file means every interface change breaks several
/// places at once for no benefit.
///
/// Use this when a test wants **expectations** - that a specific call happened,
/// with specific arguments, in a specific order. Use @ref RecordingRenderer when
/// it just wants the log.
class MockRenderer : public platform::Renderer {
public:
    MOCK_METHOD(platform::MeshHandle, createMesh,
                (std::span<const platform::Vertex> vertices,
                 std::span<const uint16_t> indices),
                (override));
    MOCK_METHOD(void, destroyMesh, (platform::MeshHandle mesh), (override));
    MOCK_METHOD(platform::TextureHandle, createTexture,
                (int width, int height, std::span<const uint8_t> pixels), (override));
    MOCK_METHOD(void, destroyTexture, (platform::TextureHandle texture), (override));

    MOCK_METHOD(void, beginScene, (const platform::Camera& camera), (override));
    MOCK_METHOD(void, endScene, (), (override));
    MOCK_METHOD(void, drawMesh,
                (platform::MeshHandle mesh, const platform::Transform& transform,
                 platform::TextureHandle texture, platform::Color tint),
                (override));
    MOCK_METHOD(void, drawSprite,
                (platform::TextureHandle texture, platform::Point3 position,
                 platform::Vec2 size, platform::Rect uvRect, platform::Color tint),
                (override));

    MOCK_METHOD(void, drawWireBox,
                (platform::Point3 center, platform::Vec3 halfExtents,
                 platform::Color color),
                (override));
    MOCK_METHOD(void, drawWireSphere,
                (platform::Point3 center, float radius, platform::Color color),
                (override));
    MOCK_METHOD(void, drawSolidBox,
                (platform::Point3 center, platform::Vec3 halfExtents,
                 platform::Color color),
                (override));
    MOCK_METHOD(void, drawSolidSphere,
                (platform::Point3 center, float radius, platform::Color color),
                (override));
    MOCK_METHOD(void, drawLine,
                (platform::Point3 from, platform::Point3 to, platform::Color color),
                (override));
    MOCK_METHOD(void, drawPoint,
                (platform::Point3 position, float size, platform::Color color),
                (override));

    MOCK_METHOD(void, begin2D, (), (override));
    MOCK_METHOD(void, end2D, (), (override));
    MOCK_METHOD(platform::Vec2, viewportSize, (), (const, override));
    MOCK_METHOD(void, drawRect, (platform::Rect area, platform::Color color), (override));
    MOCK_METHOD(void, drawTexturedRect,
                (platform::TextureHandle texture, platform::Rect area,
                 platform::Rect uvRect, platform::Color tint),
                (override));
    MOCK_METHOD(void, drawText,
                (std::string_view text, platform::Vec2 position, float size,
                 platform::Color color),
                (override));
};

} // namespace support

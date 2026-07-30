#pragma once

#include "platform/renderer.h"

#include <string>
#include <vector>

namespace support {

/// A renderer that records what it was asked to draw.
///
/// @remarks
/// For tests that want to inspect the whole draw list rather than assert on
/// individual calls - "how many meshes were drawn", "were sprites sorted back to
/// front", "did the HUD stay inside the viewport". Use @ref MockRenderer when
/// the question is about a specific call instead.
///
/// It also hands out real, distinct handles, so a consumer's resource lifetime
/// can be exercised end to end without a device.
class RecordingRenderer final : public platform::Renderer {
public:
    struct MeshDraw {
        platform::MeshHandle mesh;
        platform::Transform transform;
        platform::TextureHandle texture;
        platform::Color tint;
    };

    struct SpriteDraw {
        platform::TextureHandle texture;
        platform::Point3 position;
        platform::Vec2 size;
        platform::Rect uvRect;
        platform::Color tint;
    };

    struct RectDraw {
        platform::Rect area;
        platform::Color color;
    };

    struct TextDraw {
        std::string text;
        platform::Vec2 position;
        float size;
        platform::Color color;
    };

    // --- Recorded output ----------------------------------------------------

    std::vector<platform::Camera> scenes;
    std::vector<MeshDraw> meshDraws;
    std::vector<SpriteDraw> spriteDraws;
    std::vector<RectDraw> rectDraws;
    std::vector<TextDraw> textDraws;

    int liveMeshes = 0;
    int liveTextures = 0;
    int sceneDepth = 0;
    int overlayDepth = 0;

    /// Set this to control what layout code sees.
    platform::Vec2 viewport{ 1280.0f, 720.0f };

    void clear() {
        scenes.clear();
        meshDraws.clear();
        spriteDraws.clear();
        rectDraws.clear();
        textDraws.clear();
    }

    // --- Resources ----------------------------------------------------------

    platform::MeshHandle createMesh(std::span<const platform::Vertex>,
                                    std::span<const uint16_t>) override {
        ++liveMeshes;
        return platform::MeshHandle{ nextMeshIndex_++, 1 };
    }

    void destroyMesh(platform::MeshHandle mesh) override {
        if (mesh.generation != 0) {
            --liveMeshes;
        }
    }

    platform::TextureHandle createTexture(int, int, std::span<const uint8_t>) override {
        ++liveTextures;
        return platform::TextureHandle{ nextTextureIndex_++, 1 };
    }

    void destroyTexture(platform::TextureHandle texture) override {
        if (texture.generation != 0) {
            --liveTextures;
        }
    }

    // --- 3D scene -----------------------------------------------------------

    void beginScene(const platform::Camera& camera) override {
        scenes.push_back(camera);
        ++sceneDepth;
    }

    void endScene() override { --sceneDepth; }

    void drawMesh(platform::MeshHandle mesh, const platform::Transform& transform,
                  platform::TextureHandle texture, platform::Color tint) override {
        meshDraws.push_back(MeshDraw{ mesh, transform, texture, tint });
    }

    void drawSprite(platform::TextureHandle texture, platform::Point3 position,
                    platform::Vec2 size, platform::Rect uvRect,
                    platform::Color tint) override {
        spriteDraws.push_back(SpriteDraw{ texture, position, size, uvRect, tint });
    }

    // --- Debug primitives ---------------------------------------------------

    void drawWireBox(platform::Point3, platform::Vec3, platform::Color) override {}
    void drawWireSphere(platform::Point3, float, platform::Color) override {}
    void drawSolidBox(platform::Point3, platform::Vec3, platform::Color) override {}
    void drawSolidSphere(platform::Point3, float, platform::Color) override {}
    void drawLine(platform::Point3, platform::Point3, platform::Color) override {}
    void drawPoint(platform::Point3, float, platform::Color) override {}

    // --- 2D overlay ---------------------------------------------------------

    void begin2D() override { ++overlayDepth; }
    void end2D() override { --overlayDepth; }

    platform::Vec2 viewportSize() const override { return viewport; }

    void drawRect(platform::Rect area, platform::Color color) override {
        rectDraws.push_back(RectDraw{ area, color });
    }

    void drawTexturedRect(platform::TextureHandle, platform::Rect area, platform::Rect,
                          platform::Color color) override {
        rectDraws.push_back(RectDraw{ area, color });
    }

    void drawText(std::string_view text, platform::Vec2 position, float size,
                  platform::Color color) override {
        textDraws.push_back(TextDraw{ std::string(text), position, size, color });
    }

private:
    uint32_t nextMeshIndex_ = 0;
    uint32_t nextTextureIndex_ = 0;
};

} // namespace support

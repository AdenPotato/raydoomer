#pragma once

#include "platform/renderer.h"
#include "platform/violation.h"

#include <memory>

namespace platform {

/// raylib-backed renderer.
///
/// @remarks
/// The only place drawing reaches the GPU. Compiled solely in the Windows
/// build; the headless test configuration never sees it, which is what
/// guarantees a test cannot accidentally draw.
///
/// Owns GPU resources, so it must outlive every handle it issued.
class RaylibRenderer final : public Renderer {
public:
    explicit RaylibRenderer(ViolationReporter onViolation = defaultViolationReporter());
    ~RaylibRenderer() override;

    RaylibRenderer(const RaylibRenderer&) = delete;
    RaylibRenderer& operator=(const RaylibRenderer&) = delete;

    MeshHandle createMesh(std::span<const Vertex> vertices,
                          std::span<const uint16_t> indices) override;
    void destroyMesh(MeshHandle mesh) override;
    TextureHandle createTexture(int width, int height,
                                std::span<const uint8_t> pixels) override;
    void destroyTexture(TextureHandle texture) override;

    void beginScene(const Camera& camera) override;
    void endScene() override;
    void drawMesh(MeshHandle mesh, const Transform& transform, TextureHandle texture,
                  Color tint) override;
    void drawSprite(TextureHandle texture, Point3 position, Vec2 size, Rect uvRect,
                    Color tint) override;

    void drawWireBox(Point3 center, Vec3 halfExtents, Color color) override;
    void drawWireSphere(Point3 center, float radius, Color color) override;
    void drawSolidBox(Point3 center, Vec3 halfExtents, Color color) override;
    void drawSolidSphere(Point3 center, float radius, Color color) override;
    void drawLine(Point3 from, Point3 to, Color color) override;
    void drawPoint(Point3 position, float size, Color color) override;

    void begin2D() override;
    void end2D() override;
    Vec2 viewportSize() const override;
    void drawRect(Rect area, Color color) override;
    void drawTexturedRect(TextureHandle texture, Rect area, Rect uvRect,
                          Color tint) override;
    void drawText(std::string_view text, Vec2 position, float size, Color color) override;

private:
    // Pimpl: keeps every raylib type out of this header, so the layer boundary
    // holds even for code that constructs the backend.
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace platform

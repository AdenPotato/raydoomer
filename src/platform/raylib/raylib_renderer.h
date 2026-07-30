#pragma once

#include "platform/renderer.h"

namespace platform {

/// raylib-backed renderer.
///
/// @remarks
/// The only place drawing actually reaches the GPU. Compiled solely in the
/// Windows build; the headless test configuration never sees it, which is what
/// guarantees a test cannot accidentally draw.
class RaylibRenderer final : public Renderer {
public:
    void beginScene(const Camera& camera) override;
    void endScene() override;
    void drawWireBox(Point3 center, Vec3 halfExtents, Color color) override;
    void drawWireSphere(Point3 center, float radius, Color color) override;
    void drawSolidBox(Point3 center, Vec3 halfExtents, Color color) override;
    void drawSolidSphere(Point3 center, float radius, Color color) override;
    void drawLine(Point3 from, Point3 to, Color color) override;
    void drawPoint(Point3 position, float size, Color color) override;
};

} // namespace platform

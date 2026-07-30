#include "platform/raylib/raylib_renderer.h"

#include <raylib.h>

namespace platform {
namespace {

// World positions are double; raylib takes float. The narrowing happens here, at
// the last possible moment, so precision is only lost on the way to the GPU
// rather than anywhere it could affect the simulation.
::Vector3 toRaylib(Point3 p) {
    return ::Vector3{ static_cast<float>(p.x), static_cast<float>(p.y),
                      static_cast<float>(p.z) };
}

::Color toRaylib(Color c) {
    return ::Color{ c.r, c.g, c.b, c.a };
}

constexpr int kSphereRings = 8;
constexpr int kSphereSlices = 8;

} // namespace

void RaylibRenderer::beginScene(const Camera& camera) {
    ::Camera3D raylibCamera{};
    raylibCamera.position = toRaylib(camera.position);
    raylibCamera.target = toRaylib(camera.target);
    raylibCamera.up = ::Vector3{ 0.0f, 1.0f, 0.0f };
    raylibCamera.fovy = camera.fovDegrees;
    raylibCamera.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(raylibCamera);
    // A ground grid gives the wireframes a sense of scale and orientation, which
    // they otherwise completely lack.
    DrawGrid(40, 1.0f);
}

void RaylibRenderer::endScene() {
    EndMode3D();
}

void RaylibRenderer::drawWireBox(Point3 center, Vec3 halfExtents, Color color) {
    // raylib takes full extents; ours are half.
    DrawCubeWires(toRaylib(center), halfExtents.x * 2.0f, halfExtents.y * 2.0f,
                  halfExtents.z * 2.0f, toRaylib(color));
}

void RaylibRenderer::drawWireSphere(Point3 center, float radius, Color color) {
    DrawSphereWires(toRaylib(center), radius, kSphereRings, kSphereSlices, toRaylib(color));
}

void RaylibRenderer::drawSolidBox(Point3 center, Vec3 halfExtents, Color color) {
    DrawCube(toRaylib(center), halfExtents.x * 2.0f, halfExtents.y * 2.0f,
             halfExtents.z * 2.0f, toRaylib(color));
}

void RaylibRenderer::drawSolidSphere(Point3 center, float radius, Color color) {
    DrawSphere(toRaylib(center), radius, toRaylib(color));
}

void RaylibRenderer::drawLine(Point3 from, Point3 to, Color color) {
    DrawLine3D(toRaylib(from), toRaylib(to), toRaylib(color));
}

void RaylibRenderer::drawPoint(Point3 position, float size, Color color) {
    // raylib has no 3D point primitive, so a small sphere stands in. Clamped
    // because Box3D asks for sizes in screen pixels for some markers, which
    // would be enormous interpreted as metres.
    constexpr float kMaxWorldSize = 0.15f;
    const float radius = size > kMaxWorldSize ? kMaxWorldSize : size;
    DrawSphere(toRaylib(position), radius, toRaylib(color));
}

} // namespace platform

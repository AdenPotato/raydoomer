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

} // namespace platform

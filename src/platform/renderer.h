#pragma once

#include "platform/math_types.h"

namespace platform {

/// Submission of drawing work to the graphics device.
///
/// @remarks
/// The only path to the GPU. Everything above talks to this interface, which is
/// what keeps the test suite runnable with no window - a test mocks it and
/// asserts on the draw calls rather than on pixels.
///
/// **This is the minimal seed of the renderer.** It carries exactly what the
/// debug view needs: a camera and wireframe primitives. Textured meshes,
/// sprites, and 2D overlay drawing arrive when the full seam is defined, and
/// that change is additive.
///
/// Coordinate system: right-handed, Y up, distances in metres. Angles in degrees.
class Renderer {
public:
    virtual ~Renderer() = default;

    /// Begins a 3D scene viewed through `camera`. Pairs with @ref endScene.
    virtual void beginScene(const Camera& camera) = 0;

    /// Ends the 3D scene. Must pair with @ref beginScene.
    virtual void endScene() = 0;

    /// Draws an axis-aligned wireframe box.
    /// @param center      World-space centre.
    /// @param halfExtents Half the size on each axis, so a unit cube is 0.5.
    virtual void drawWireBox(Point3 center, Vec3 halfExtents, Color color) = 0;

    /// Draws a wireframe sphere.
    virtual void drawWireSphere(Point3 center, float radius, Color color) = 0;
};

} // namespace platform

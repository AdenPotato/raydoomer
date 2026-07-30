#pragma once

#include "platform/math_types.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace platform {

/// An opaque reference to uploaded geometry.
///
/// @remarks
/// Generational, matching `engine::BodyHandle`: when a slot is recycled a bare
/// index would let an old handle silently address whatever took its place. A
/// default-constructed handle is never valid.
struct MeshHandle {
    uint32_t index = 0;
    uint32_t generation = 0;

    friend bool operator==(const MeshHandle&, const MeshHandle&) = default;
};

/// An opaque reference to an uploaded texture. Generational, as above.
struct TextureHandle {
    uint32_t index = 0;
    uint32_t generation = 0;

    friend bool operator==(const TextureHandle&, const TextureHandle&) = default;
};

/// One vertex of a mesh.
struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;

    friend bool operator==(const Vertex&, const Vertex&) = default;
};

/// Submission of drawing work to the graphics device.
///
/// @remarks
/// The only path to the GPU. Everything above talks to this interface, which is
/// what keeps the test suite runnable with no window: a test substitutes a
/// recording double and asserts on the calls rather than on pixels.
///
/// ### Conventions at this boundary
///
/// - **Coordinates** are right-handed, **Y up**, distances in **metres**.
/// - **Angles** are degrees where a field says so, radians otherwise. Every
///   parameter states which.
/// - **Winding** is counter-clockwise for front faces.
/// - **UVs** are normalised, origin top-left.
/// - **Screen space** is pixels, origin top-left.
/// - **Handle lifetime** is the caller's: whoever creates a mesh or texture
///   destroys it. A stale handle is detected and reported, never followed.
///
/// ### Three drawing paths, deliberately not unified
///
/// - **Meshes** are uploaded once and drawn by transform. Level geometry.
/// - **Sprites** are camera-facing quads recomputed per frame. Forcing them
///   through the mesh path would mean rebuilding vertices every frame, which is
///   the exact cost meshes exist to avoid.
/// - **2D** is screen-space and has no world transform at all.
///
/// Generalising the three into one abstraction would fit none of them well.
///
/// ### Batching
///
/// Each `drawMesh` is one draw call. That is fine for a handful of meshes and
/// ruinous for hundreds, so **the batching lives in what the caller uploads,
/// not in this interface**:
///
/// - **Level geometry** should be merged at load into one mesh per material,
///   rather than one mesh per brush. `createMesh` takes arbitrary geometry
///   precisely so a loader can do that, and a level of a few hundred brushes
///   then costs a few draw calls rather than a few hundred.
/// - **Sprites** go through raylib's internal batch, which coalesces runs
///   sharing a texture. Drawing them grouped by texture is therefore worth far
///   more than drawing them in spawn order.
/// - **Debug primitives** are not batched and are not meant to be.
///
/// **This is stated, not measured.** There is no level geometry yet, so any
/// number produced today would describe a synthetic scene rather than the game.
/// The real measurement belongs with the level renderer, whose acceptance
/// criteria already require a frame time taken at a realistic brush count.
class Renderer {
public:
    virtual ~Renderer() = default;

    // --- Resources ----------------------------------------------------------

    /// Uploads geometry to the GPU.
    /// @param vertices Vertex data. Copied; the caller's buffer is not retained.
    /// @param indices  Triangle indices, counter-clockwise front faces.
    /// @returns A handle, or a default-constructed one if upload failed.
    virtual MeshHandle createMesh(std::span<const Vertex> vertices,
                                  std::span<const uint16_t> indices) = 0;

    /// Releases geometry. Destroying an invalid handle is reported, not undefined.
    virtual void destroyMesh(MeshHandle mesh) = 0;

    /// Uploads a texture.
    /// @param pixels RGBA8, row-major, `width * height * 4` bytes.
    virtual TextureHandle createTexture(int width, int height,
                                        std::span<const uint8_t> pixels) = 0;

    /// Releases a texture. Destroying an invalid handle is reported, not undefined.
    virtual void destroyTexture(TextureHandle texture) = 0;

    // --- 3D scene -----------------------------------------------------------

    /// Begins a 3D scene viewed through `camera`. Pairs with @ref endScene.
    virtual void beginScene(const Camera& camera) = 0;

    /// Ends the 3D scene.
    virtual void endScene() = 0;

    /// Draws uploaded geometry.
    /// @param texture A default-constructed handle draws untextured.
    /// @param tint    Multiplied with the texture; white leaves it unchanged.
    virtual void drawMesh(MeshHandle mesh, const Transform& transform,
                          TextureHandle texture, Color tint) = 0;

    /// Draws a camera-facing quad.
    ///
    /// @param size   World-space width and height in metres.
    /// @param uvRect Sub-region of the texture, normalised. The whole texture is
    ///               `{0, 0, 1, 1}`.
    /// @note Orientation is computed by the implementation from the current
    ///       camera. The maths itself lives in `engine/billboard.h` so it can be
    ///       tested without a device.
    virtual void drawSprite(TextureHandle texture, Point3 position, Vec2 size,
                            Rect uvRect, Color tint) = 0;

    // --- Debug primitives ---------------------------------------------------
    //
    // Immediate mode and low volume, by design. Routing diagnostics through the
    // mesh path would make the tool harder to use than the thing it diagnoses.

    virtual void drawWireBox(Point3 center, Vec3 halfExtents, Color color) = 0;
    virtual void drawWireSphere(Point3 center, float radius, Color color) = 0;
    virtual void drawSolidBox(Point3 center, Vec3 halfExtents, Color color) = 0;
    virtual void drawSolidSphere(Point3 center, float radius, Color color) = 0;
    virtual void drawLine(Point3 from, Point3 to, Color color) = 0;
    virtual void drawPoint(Point3 position, float size, Color color) = 0;

    // --- 2D overlay ---------------------------------------------------------

    /// Begins screen-space drawing. Pairs with @ref end2D.
    /// @note Must be called outside a 3D scene, not nested inside one.
    virtual void begin2D() = 0;

    /// Ends screen-space drawing.
    virtual void end2D() = 0;

    /// @returns The drawable size in pixels, for laying out an overlay.
    virtual Vec2 viewportSize() const = 0;

    /// Draws a filled rectangle in screen space, pixels, origin top-left.
    virtual void drawRect(Rect area, Color color) = 0;

    /// Draws a textured rectangle in screen space.
    /// @param uvRect Sub-region of the texture, normalised.
    virtual void drawTexturedRect(TextureHandle texture, Rect area, Rect uvRect,
                                  Color tint) = 0;

    /// Draws text in screen space.
    /// @param size Cap height in pixels.
    virtual void drawText(std::string_view text, Vec2 position, float size,
                          Color color) = 0;
};

} // namespace platform

#pragma once

#include "platform/renderer.h"

namespace engine {

class PhysicsWorld;

/// How the debug view draws bodies.
enum class DebugDrawMode {
    Wireframe, ///< Outlines only. Everything is visible through everything else.
    Solid,     ///< Filled. Reads as shapes, but occludes what is behind.
    Both,      ///< Filled with the outline over it. Usually the most legible.
};

/// Draws the physics world as wireframes, from a fixed camera.
///
/// @remarks
/// A diagnostic view, not the game's renderer. It exists because the simulation
/// was previously only observable through assertions: bodies fell, contacts
/// fired, and nothing could be seen.
///
/// **Presentation only.** It reads simulation state and never writes it. If
/// removing this type would change what the game does, a rule has leaked into
/// the renderer (presentation_protocol.md).
///
/// Off by default. A diagnostic that cannot be switched off is not a diagnostic.
class DebugRenderer {
public:
    /// Enables or disables drawing. Disabled draws nothing at all - not even an
    /// empty scene.
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

    /// Whether the debug view starts enabled.
    ///
    /// @remarks
    /// On in a development build, off in a release build. A diagnostic view
    /// should never ship on by accident, and equally should never need to be
    /// switched on by hand every single run during development.
    static constexpr bool defaultEnabled() {
#ifdef NDEBUG
        return false;
#else
        return true;
#endif
    }

    /// How bodies are drawn. Independent of whether the view is enabled.
    void setDrawMode(DebugDrawMode mode) { drawMode_ = mode; }
    DebugDrawMode drawMode() const { return drawMode_; }

    /// Advances to the next draw mode, wrapping around.
    ///
    /// @remarks
    /// A closed loop with no dead end, because one key cycles all three at
    /// runtime.
    void cycleDrawMode();

    /// Whether the solver overlay is drawn on top of the interpolated shapes.
    ///
    /// @remarks
    /// Off by default. It shows what our own record of the world cannot -
    /// actual contact points and normals, straight from Box3D - at the cost of
    /// not being interpolated. Independent of @ref setDrawMode.
    void setSolverOverlayEnabled(bool enabled) { solverOverlay_ = enabled; }
    bool isSolverOverlayEnabled() const { return solverOverlay_; }

    /// Points the camera somewhere other than the default.
    void setCamera(const platform::Camera& camera) { camera_ = camera; }
    const platform::Camera& camera() const { return camera_; }

    /// Draws every live body in `world`.
    ///
    /// @param alpha Interpolation between the previous and current simulation
    ///        states, from the frame loop. Drawing raw current state stutters
    ///        whenever the tick and the frame disagree.
    void draw(platform::Renderer& renderer, const PhysicsWorld& world, double alpha) const;

private:
    bool enabled_ = defaultEnabled();
    bool solverOverlay_ = false;
    DebugDrawMode drawMode_ = DebugDrawMode::Wireframe;

    // Far enough back to see a body dropped from a few metres, angled so depth
    // reads. Deliberately fixed: a controllable camera is its own piece of work.
    platform::Camera camera_{
        platform::Point3{ 12.0, 8.0, 12.0 },
        platform::Point3{ 0.0, 1.0, 0.0 },
        60.0f,
    };
};

} // namespace engine

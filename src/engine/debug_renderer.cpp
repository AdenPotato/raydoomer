#include "engine/debug_renderer.h"

#include "engine/physics.h"

#include <variant>

namespace engine {
namespace {

// Static geometry and dynamic actors draw in different colours. Telling level
// geometry from a moving body at a glance is most of this view's value.
constexpr platform::Color kStaticColor{ 90, 110, 140, 255 };
constexpr platform::Color kDynamicColor{ 220, 90, 70, 255 };

// Solid fills use a dimmer version of the same hue, so the wireframe drawn over
// the top in Both mode still reads against it.
constexpr platform::Color dim(platform::Color color) {
    return platform::Color{ static_cast<uint8_t>(color.r / 2),
                            static_cast<uint8_t>(color.g / 2),
                            static_cast<uint8_t>(color.b / 2), color.a };
}

platform::Color colorFor(BodyType type) {
    return type == BodyType::Static ? kStaticColor : kDynamicColor;
}

bool drawsSolid(DebugDrawMode mode) {
    return mode == DebugDrawMode::Solid || mode == DebugDrawMode::Both;
}

bool drawsWireframe(DebugDrawMode mode) {
    return mode == DebugDrawMode::Wireframe || mode == DebugDrawMode::Both;
}

} // namespace

void DebugRenderer::cycleDrawMode() {
    switch (drawMode_) {
        case DebugDrawMode::Wireframe: drawMode_ = DebugDrawMode::Solid; break;
        case DebugDrawMode::Solid:     drawMode_ = DebugDrawMode::Both; break;
        case DebugDrawMode::Both:      drawMode_ = DebugDrawMode::Wireframe; break;
    }
}

void DebugRenderer::draw(platform::Renderer& renderer, const PhysicsWorld& world,
                         double alpha) const {
    if (!enabled_) {
        return;
    }

    renderer.beginScene(camera_);

    world.forEachBody(alpha, [&](const PhysicsWorld::BodyView& body) {
        const platform::Color color = colorFor(body.type);

        // Solid first, then the wireframe over it. The reverse order would hide
        // the outline behind the fill, making Both indistinguishable from Solid.
        if (const auto* sphere = std::get_if<SphereShape>(&body.shape)) {
            if (drawsSolid(drawMode_)) {
                renderer.drawSolidSphere(body.position, sphere->radius, dim(color));
            }
            if (drawsWireframe(drawMode_)) {
                renderer.drawWireSphere(body.position, sphere->radius, color);
            }
        } else if (const auto* box = std::get_if<BoxShape>(&body.shape)) {
            if (drawsSolid(drawMode_)) {
                renderer.drawSolidBox(body.position, box->halfExtents, dim(color));
            }
            if (drawsWireframe(drawMode_)) {
                renderer.drawWireBox(body.position, box->halfExtents, color);
            }
        }
    });

    // Drawn last so contact markers sit over the shapes rather than inside
    // them. Not interpolated - this is the solver's own current state.
    if (solverOverlay_) {
        world.debugDrawSolver(renderer, PhysicsWorld::SolverDebugOptions{});
    }

    renderer.endScene();
}

} // namespace engine

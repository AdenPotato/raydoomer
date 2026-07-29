#include "engine/debug_renderer.h"

#include "engine/physics.h"

#include <variant>

namespace engine {
namespace {

// Static geometry and dynamic actors draw in different colours. Telling level
// geometry from a moving body at a glance is most of this view's value.
constexpr platform::Color kStaticColor{ 90, 110, 140, 255 };
constexpr platform::Color kDynamicColor{ 220, 90, 70, 255 };

platform::Color colorFor(BodyType type) {
    return type == BodyType::Static ? kStaticColor : kDynamicColor;
}

} // namespace

void DebugRenderer::draw(platform::Renderer& renderer, const PhysicsWorld& world,
                         double alpha) const {
    if (!enabled_) {
        return;
    }

    renderer.beginScene(camera_);

    world.forEachBody(alpha, [&](const PhysicsWorld::BodyView& body) {
        const platform::Color color = colorFor(body.type);

        // The shape variant decides the primitive. A new shape type will fail to
        // compile here rather than silently drawing nothing.
        if (const auto* sphere = std::get_if<SphereShape>(&body.shape)) {
            renderer.drawWireSphere(body.position, sphere->radius, color);
        } else if (const auto* box = std::get_if<BoxShape>(&body.shape)) {
            renderer.drawWireBox(body.position, box->halfExtents, color);
        }
    });

    renderer.endScene();
}

} // namespace engine

#include "engine/physics.h"

#include "engine/event_dispatch.h"

#include <box3d/box3d.h>

#include <string>
#include <vector>

namespace engine {
namespace {

/// Locked for determinism. Box3D's multithreaded solver can reorder constraint
/// resolution between runs, which engine_protocol.md treats as a defect rather
/// than a tolerance to widen. Enabling threading is a decision to revisit with a
/// measurement (locked_decisions.md).
constexpr uint32_t kWorkerCount = 1;

/// Solver iterations per step. More is more accurate and slower; 4 is Box3D's
/// own default and there is no measurement yet justifying anything else.
constexpr int kSubStepCount = 4;

b3Vec3 toBox3d(Vec3 v) {
    return b3Vec3{ v.x, v.y, v.z };
}

b3Pos toBox3d(Point3 p) {
    return b3Pos{ p.x, p.y, p.z };
}

Point3 fromBox3d(b3Pos p) {
    return Point3{ p.x, p.y, p.z };
}

} // namespace

/// One body slot. Recycled on destruction, with the generation bumped so a
/// handle to the previous occupant stops resolving.
struct PhysicsWorld::Impl {
    struct Slot {
        b3BodyId body{};
        uint32_t generation = 0;
        bool alive = false;

        // Kept so the debug view can describe a body without reaching into Box3D.
        Shape shape{};
        BodyType type = BodyType::Dynamic;

        // The position before the most recent step. Presentation blends between
        // this and the current one; without it, drawing stutters.
        Point3 previousPosition{};
    };

    b3WorldId world{};
    std::vector<Slot> slots;
    std::vector<uint32_t> freeSlots;
    ViolationReporter onViolation;
    int liveBodies = 0;

    void report(const std::string& message) const {
        if (onViolation) {
            onViolation(message);
        }
    }

    const Slot* resolve(BodyHandle handle, const char* operation) const {
        if (handle.generation == 0 || handle.index >= slots.size()) {
            report(std::string("stale or invalid body handle passed to ") + operation);
            return nullptr;
        }
        const Slot& slot = slots[handle.index];
        if (!slot.alive || slot.generation != handle.generation) {
            report(std::string("stale body handle passed to ") + operation +
                   ": the body it referred to has been destroyed");
            return nullptr;
        }
        return &slot;
    }

    /// Maps a Box3D body back to its handle. The slot index is stored in the
    /// body's user data at creation, so a contact event can name our handles
    /// rather than leaking Box3D ids upward.
    BodyHandle handleFor(b3BodyId body) const {
        const auto stored = reinterpret_cast<uintptr_t>(b3Body_GetUserData(body));
        const auto index = static_cast<uint32_t>(stored);
        if (index >= slots.size() || !slots[index].alive) {
            return BodyHandle{};
        }
        return BodyHandle{ index, slots[index].generation };
    }
};

PhysicsWorld::PhysicsWorld(Vec3 gravity, ViolationReporter onViolation)
    : impl_(std::make_unique<Impl>()) {
    impl_->onViolation = std::move(onViolation);

    b3WorldDef def = b3DefaultWorldDef();
    def.gravity = toBox3d(gravity);
    def.workerCount = kWorkerCount;

    impl_->world = b3CreateWorld(&def);
}

PhysicsWorld::~PhysicsWorld() {
    // Every acquire has a matching release. Destroying the world releases its
    // bodies, so slots need no individual teardown.
    b3DestroyWorld(impl_->world);
}

BodyHandle PhysicsWorld::createBody(const BodyDef& def) {
    b3BodyDef bodyDef = b3DefaultBodyDef();
    bodyDef.type = (def.type == BodyType::Static) ? b3_staticBody : b3_dynamicBody;
    bodyDef.position = toBox3d(def.position);
    if (def.lockRotation) {
        // A character must not tip. Locking all three angular axes is what keeps
        // the player upright when it brushes geometry.
        bodyDef.motionLocks.angularX = true;
        bodyDef.motionLocks.angularY = true;
        bodyDef.motionLocks.angularZ = true;
    }

    const b3BodyId body = b3CreateBody(impl_->world, &bodyDef);

    // A dynamic body with no shape has no mass, and the solver skips it
    // entirely - it sits at its spawn position forever.
    b3ShapeDef shapeDef = b3DefaultShapeDef();
    shapeDef.density = def.density;
    // Box3D disables contact events per shape by default. Without this the
    // simulation collides correctly and reports nothing, which is a genuinely
    // confusing failure: the ball lands, and no event ever fires.
    shapeDef.enableContactEvents = def.reportsContacts;
    if (def.friction >= 0.0f) {
        shapeDef.baseMaterial.friction = def.friction;
    }

    if (const auto* sphere = std::get_if<SphereShape>(&def.shape)) {
        const b3Sphere geometry{ b3Vec3{ 0.0f, 0.0f, 0.0f }, sphere->radius };
        (void)b3CreateSphereShape(body, &shapeDef, &geometry);
    } else if (const auto* box = std::get_if<BoxShape>(&def.shape)) {
        // The hull is fully cloned by Box3D, so a local is safe here.
        const b3BoxHull hull =
            b3MakeBoxHull(box->halfExtents.x, box->halfExtents.y, box->halfExtents.z);
        (void)b3CreateHullShape(body, &shapeDef, &hull.base);
    }

    // Claim a slot, reusing a free one where possible.
    uint32_t index = 0;
    if (!impl_->freeSlots.empty()) {
        index = impl_->freeSlots.back();
        impl_->freeSlots.pop_back();
    } else {
        index = static_cast<uint32_t>(impl_->slots.size());
        impl_->slots.emplace_back();
    }

    Impl::Slot& slot = impl_->slots[index];
    slot.body = body;
    slot.alive = true;
    slot.shape = def.shape;
    slot.type = def.type;
    slot.previousPosition = def.position;
    // Generations start at 1 so a default-constructed handle is never valid.
    slot.generation += 1;

    b3Body_SetUserData(body, reinterpret_cast<void*>(static_cast<uintptr_t>(index)));

    impl_->liveBodies += 1;
    return BodyHandle{ index, slot.generation };
}

void PhysicsWorld::destroyBody(BodyHandle handle) {
    const Impl::Slot* found = impl_->resolve(handle, "destroyBody");
    if (found == nullptr) {
        return;
    }

    Impl::Slot& slot = impl_->slots[handle.index];
    b3DestroyBody(slot.body);
    slot.alive = false;
    // The generation is bumped on reuse rather than here, so a handle to this
    // slot stops resolving the moment `alive` goes false.
    impl_->freeSlots.push_back(handle.index);
    impl_->liveBodies -= 1;
}

bool PhysicsWorld::isValid(BodyHandle handle) const {
    if (handle.generation == 0 || handle.index >= impl_->slots.size()) {
        return false;
    }
    const Impl::Slot& slot = impl_->slots[handle.index];
    return slot.alive && slot.generation == handle.generation;
}

std::optional<Point3> PhysicsWorld::position(BodyHandle handle) const {
    const Impl::Slot* slot = impl_->resolve(handle, "position");
    if (slot == nullptr) {
        return std::nullopt;
    }
    return fromBox3d(b3Body_GetPosition(slot->body));
}

void PhysicsWorld::step(float dt, EventDispatcher& events) {
    // Snapshot before advancing, so presentation has two states to blend.
    for (Impl::Slot& slot : impl_->slots) {
        if (slot.alive) {
            slot.previousPosition = fromBox3d(b3Body_GetPosition(slot.body));
        }
    }

    b3World_Step(impl_->world, dt, kSubStepCount);

    // Contacts are read out and republished as engine events. Gameplay learns
    // that two bodies touched without ever seeing a Box3D type.
    const b3ContactEvents contacts = b3World_GetContactEvents(impl_->world);
    for (int i = 0; i < contacts.beginCount; ++i) {
        const b3ContactBeginTouchEvent& touch = contacts.beginEvents[i];
        const BodyHandle a = impl_->handleFor(b3Shape_GetBody(touch.shapeIdA));
        const BodyHandle b = impl_->handleFor(b3Shape_GetBody(touch.shapeIdB));
        if (a.generation == 0 || b.generation == 0) {
            continue; // one side was destroyed this step
        }
        events.publish(ContactBegan{ a, b });
    }
}

std::optional<Vec3> PhysicsWorld::linearVelocity(BodyHandle handle) const {
    const Impl::Slot* slot = impl_->resolve(handle, "linearVelocity");
    if (slot == nullptr) {
        return std::nullopt;
    }
    const b3Vec3 v = b3Body_GetLinearVelocity(slot->body);
    return Vec3{ v.x, v.y, v.z };
}

void PhysicsWorld::setLinearVelocity(BodyHandle handle, Vec3 velocity) {
    const Impl::Slot* slot = impl_->resolve(handle, "setLinearVelocity");
    if (slot == nullptr) {
        return;
    }
    b3Body_SetLinearVelocity(slot->body, b3Vec3{ velocity.x, velocity.y, velocity.z });
}

std::optional<PhysicsWorld::RayHit> PhysicsWorld::raycastClosest(Point3 origin,
                                                                 Vec3 translation) const {
    const b3RayResult result =
        b3World_CastRayClosest(impl_->world, toBox3d(origin),
                               b3Vec3{ translation.x, translation.y, translation.z },
                               b3DefaultQueryFilter());

    // Box3D uses 1-based shape ids, so index1 == 0 means nothing was hit.
    if (result.shapeId.index1 == 0) {
        return std::nullopt;
    }

    return RayHit{
        impl_->handleFor(b3Shape_GetBody(result.shapeId)),
        fromBox3d(result.point),
        Vec3{ result.normal.x, result.normal.y, result.normal.z },
        result.fraction,
    };
}

std::optional<Point3> PhysicsWorld::interpolatedPosition(BodyHandle handle, double alpha) const {
    const Impl::Slot* slot = impl_->resolve(handle, "interpolatedPosition");
    if (slot == nullptr) {
        return std::nullopt;
    }

    const Point3 current = fromBox3d(b3Body_GetPosition(slot->body));
    const Point3 previous = slot->previousPosition;
    return Point3{
        previous.x + (current.x - previous.x) * alpha,
        previous.y + (current.y - previous.y) * alpha,
        previous.z + (current.z - previous.z) * alpha,
    };
}

void PhysicsWorld::forEachBody(double alpha,
                               const std::function<void(const BodyView&)>& visit) const {
    // Index order, which is stable: slots are only ever appended or recycled in
    // place, so two identical runs visit bodies in the same order
    // (engine_protocol.md - stable iteration order).
    for (uint32_t index = 0; index < impl_->slots.size(); ++index) {
        const Impl::Slot& slot = impl_->slots[index];
        if (!slot.alive) {
            continue;
        }

        const Point3 current = fromBox3d(b3Body_GetPosition(slot.body));
        const Point3 previous = slot.previousPosition;

        visit(BodyView{
            BodyHandle{ index, slot.generation },
            Point3{
                previous.x + (current.x - previous.x) * alpha,
                previous.y + (current.y - previous.y) * alpha,
                previous.z + (current.z - previous.z) * alpha,
            },
            slot.shape,
            slot.type,
        });
    }
}

namespace {

/// Bridges Box3D's C callbacks to our renderer.
///
/// A pointer to this is handed to Box3D as the opaque `context`, so every
/// callback can reach the renderer without a global.
struct SolverDrawBridge {
    platform::Renderer* renderer;
};

platform::Renderer& rendererFrom(void* context) {
    return *static_cast<SolverDrawBridge*>(context)->renderer;
}

/// b3HexColor packs RGB into a single integer.
platform::Color fromHex(b3HexColor hex) {
    const auto value = static_cast<uint32_t>(hex);
    return platform::Color{
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>(value & 0xFF),
        255,
    };
}

Point3 pointFrom(b3Pos p) {
    return Point3{ p.x, p.y, p.z };
}

void drawSegment(b3Pos p1, b3Pos p2, b3HexColor color, void* context) {
    rendererFrom(context).drawLine(pointFrom(p1), pointFrom(p2), fromHex(color));
}

void drawPoint(b3Pos p, float size, b3HexColor color, void* context) {
    rendererFrom(context).drawPoint(pointFrom(p), size, fromHex(color));
}

void drawSphere(b3Pos p, float radius, b3HexColor color, float /*alpha*/, void* context) {
    rendererFrom(context).drawWireSphere(pointFrom(p), radius, fromHex(color));
}

void drawBounds(b3AABB aabb, b3HexColor color, void* context) {
    const Point3 center{
        (static_cast<double>(aabb.lowerBound.x) + aabb.upperBound.x) * 0.5,
        (static_cast<double>(aabb.lowerBound.y) + aabb.upperBound.y) * 0.5,
        (static_cast<double>(aabb.lowerBound.z) + aabb.upperBound.z) * 0.5,
    };
    const Vec3 halfExtents{
        (aabb.upperBound.x - aabb.lowerBound.x) * 0.5f,
        (aabb.upperBound.y - aabb.lowerBound.y) * 0.5f,
        (aabb.upperBound.z - aabb.lowerBound.z) * 0.5f,
    };
    rendererFrom(context).drawWireBox(center, halfExtents, fromHex(color));
}

void drawCapsule(b3Pos p1, b3Pos p2, float radius, b3HexColor color, float /*alpha*/,
                 void* context) {
    // Approximated as a segment plus end caps. No capsule shapes exist yet;
    // this is here so the callback is never null.
    platform::Renderer& renderer = rendererFrom(context);
    const platform::Color c = fromHex(color);
    renderer.drawLine(pointFrom(p1), pointFrom(p2), c);
    renderer.drawWireSphere(pointFrom(p1), radius, c);
    renderer.drawWireSphere(pointFrom(p2), radius, c);
}

void drawTransform(b3WorldTransform transform, void* context) {
    // Origin marker only. Drawing the axes needs the quaternion rotated into
    // basis vectors, which is more than a diagnostic needs today.
    rendererFrom(context).drawPoint(pointFrom(transform.p), 0.1f,
                                    platform::Color{ 255, 255, 0, 255 });
}

void drawString(b3Pos p, const char* /*text*/, b3HexColor color, void* context) {
    // No world-space text yet. Marked with a point so the information is not
    // silently lost.
    rendererFrom(context).drawPoint(pointFrom(p), 0.05f, fromHex(color));
}

void drawUserShape(void* /*userShape*/, b3WorldTransform transform, b3HexColor color,
                   void* context) {
    // Only reached if debug shape callbacks were registered, which they are not.
    rendererFrom(context).drawPoint(pointFrom(transform.p), 0.1f, fromHex(color));
}

void drawBox(b3Vec3 extents, b3WorldTransform transform, b3HexColor color, void* context) {
    // Rotation is dropped: the renderer only draws axis-aligned boxes today.
    // Correct for the level geometry we author, and wrong the moment a rotated
    // body exists - which is why oriented boxes belong with the real renderer,
    // not this diagnostic.
    rendererFrom(context).drawWireBox(pointFrom(transform.p),
                                      Vec3{ extents.x, extents.y, extents.z },
                                      fromHex(color));
}

} // namespace

void PhysicsWorld::debugDrawSolver(platform::Renderer& renderer,
                                   const SolverDebugOptions& options) const {
    SolverDrawBridge bridge{ &renderer };

    // Every callback is supplied, including ones this project has no use for.
    // The header says null functions are skipped; in practice leaving any unset
    // segfaults, so the contract is "provide them all".
    b3DebugDraw draw{};
    draw.context = &bridge;
    draw.DrawSegmentFcn = &drawSegment;
    draw.DrawPointFcn = &drawPoint;
    draw.DrawSphereFcn = &drawSphere;
    draw.DrawCapsuleFcn = &drawCapsule;
    draw.DrawBoundsFcn = &drawBounds;
    draw.DrawBoxFcn = &drawBox;
    draw.DrawTransformFcn = &drawTransform;
    draw.DrawStringFcn = &drawString;
    draw.DrawShapeFcn = &drawUserShape;

    // Scales Box3D uses when drawing forces and joints. Left at zero these can
    // produce degenerate geometry.
    draw.forceScale = 1.0f;
    draw.jointScale = 1.0f;

    // Culling bounds. Without a generous volume here everything is culled and
    // the overlay silently draws nothing.
    constexpr float kFar = 1.0e6f;
    draw.drawingBounds = b3AABB{ b3Vec3{ -kFar, -kFar, -kFar }, b3Vec3{ kFar, kFar, kFar } };

    draw.drawShapes = options.shapes;
    draw.drawContacts = options.contacts;
    draw.drawContactNormals = options.contactNormals;
    draw.drawBounds = options.bounds;
    draw.drawMass = options.centerOfMass;

    // All collision categories.
    b3World_Draw(impl_->world, &draw, UINT64_MAX);
}

int PhysicsWorld::bodyCount() const {
    return impl_->liveBodies;
}

uint32_t PhysicsWorld::workerCount() const {
    return kWorkerCount;
}

} // namespace engine

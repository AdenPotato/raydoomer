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

    const b3BodyId body = b3CreateBody(impl_->world, &bodyDef);

    // A dynamic body with no shape has no mass, and the solver skips it
    // entirely - it sits at its spawn position forever.
    b3ShapeDef shapeDef = b3DefaultShapeDef();
    shapeDef.density = def.density;
    // Box3D disables contact events per shape by default. Without this the
    // simulation collides correctly and reports nothing, which is a genuinely
    // confusing failure: the ball lands, and no event ever fires.
    shapeDef.enableContactEvents = def.reportsContacts;

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

int PhysicsWorld::bodyCount() const {
    return impl_->liveBodies;
}

uint32_t PhysicsWorld::workerCount() const {
    return kWorkerCount;
}

} // namespace engine

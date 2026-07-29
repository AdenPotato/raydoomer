#pragma once

#include "engine/violation.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <variant>

namespace engine {

class EventDispatcher;

/// A direction, extent, or gravity vector. Single precision is ample.
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    friend bool operator==(const Vec3&, const Vec3&) = default;
};

/// A world-space position.
///
/// @remarks
/// Double precision, mirroring Box3D's own split: translation is double, and
/// rotation stays float because it never needs the extra range. Narrowing this
/// to float would silently cost precision far from the origin, which is exactly
/// the case large levels produce.
struct Point3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    friend bool operator==(const Point3&, const Point3&) = default;
};

/// An opaque reference to a physics body.
///
/// @remarks
/// A generational index, not a pointer or a bare slot number. When a body is
/// destroyed its slot is recycled, and a bare index would let an old handle
/// silently address whatever took its place - a use-after-free wearing a
/// handle's clothing. The generation makes that detectable, which is what
/// engine_protocol.md means by "a stale handle is detected and reported, never
/// followed".
///
/// Gameplay cannot dereference or free this. A default-constructed handle is
/// never valid.
struct BodyHandle {
    uint32_t index = 0;
    uint32_t generation = 0; ///< 0 means "never referred to a body".

    friend bool operator==(const BodyHandle&, const BodyHandle&) = default;
};

enum class BodyType {
    Static,  ///< Never moves. Level geometry.
    Dynamic, ///< Moved by the solver.
};

struct SphereShape {
    float radius = 0.5f;
};

struct BoxShape {
    Vec3 halfExtents{ 0.5f, 0.5f, 0.5f };
};

using Shape = std::variant<SphereShape, BoxShape>;

struct BodyDef {
    Point3 position;
    BodyType type = BodyType::Dynamic;
    Shape shape = SphereShape{};

    /// Mass per unit volume. Ignored for static bodies.
    float density = 1.0f;

    /// Whether this body wants contacts involving it to be published.
    ///
    /// @note **Pairwise, not per-body.** A contact is reported if *either* body
    ///       in the pair wants it, so opting out only takes effect when both
    ///       sides do. That is the useful semantic: a projectile still learns it
    ///       hit a wall that does not care about being hit.
    ///
    /// @note Defaults to true. Box3D disables contact events per shape by
    ///       default, and a body that silently never reports a touch is a far
    ///       more surprising default than one that costs a little to track.
    bool reportsContacts = true;
};

/// Published when two bodies begin touching.
///
/// @remarks
/// Physics reports; the game decides what a contact means. A callback into
/// gameplay would invert the dependency and reach across the engine boundary.
struct ContactBegan {
    BodyHandle a;
    BodyHandle b;
};

/// The physics world, wrapping Box3D.
///
/// @remarks
/// Nothing above this type sees a Box3D type, and no Box3D header is reachable
/// from `src/game/` - the layer-boundary CI gate enforces that. If replacing the
/// physics engine would touch gameplay, this wrapper is too thin
/// (engine_api_protocol.md).
///
/// **Stepped once per fixed tick, never per rendered frame.** Determinism-
/// affecting configuration - the worker count in particular - is owned here and
/// locked, not exposed as a caller's option.
class PhysicsWorld {
public:
    explicit PhysicsWorld(Vec3 gravity = Vec3{ 0.0f, -9.8f, 0.0f },
                          ViolationReporter onViolation = defaultViolationReporter());
    ~PhysicsWorld();

    // Owns a Box3D world; copying would double-free it.
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    /// Creates a body and attaches its shape.
    /// @note A dynamic body needs a shape to have mass. Without one the solver
    ///       skips it entirely and it never moves.
    BodyHandle createBody(const BodyDef& def);

    /// Destroys a body. Destroying an already-destroyed handle is reported
    /// rather than undefined.
    void destroyBody(BodyHandle handle);

    /// @returns true if the handle refers to a live body.
    bool isValid(BodyHandle handle) const;

    /// @returns The body's world position, or `nullopt` if the handle is stale.
    ///          A stale lookup also reports a violation.
    std::optional<Point3> position(BodyHandle handle) const;

    /// Advances the simulation and publishes contact events.
    /// @param dt     Always the fixed tick, never a frame delta.
    /// @param events Where contacts are published.
    void step(float dt, EventDispatcher& events);

    /// @returns How many live bodies exist.
    int bodyCount() const;

    /// @returns The solver's worker count. Locked at 1 for determinism.
    uint32_t workerCount() const;

private:
    // Pimpl: keeps every Box3D type out of this header, so nothing above the
    // engine can even name one.
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace engine

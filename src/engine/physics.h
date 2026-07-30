#pragma once

#include "platform/violation.h"
#include "platform/math_types.h"
#include "platform/renderer.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <variant>

namespace engine {

// Lives in platform/ so the renderer seam can use it too; aliased here so
// callers still write engine::ViolationReporter.
using platform::ViolationReporter;
using platform::defaultViolationReporter;

class EventDispatcher;

// Vec3 and Point3 live in `platform/` rather than here.
//
// The dependency runs game -> engine -> platform, so a type the renderer
// interface needs in its signatures cannot be owned by the engine: platform
// would then depend on engine, inverting the layering. They are aliased into
// this namespace so callers still write engine::Vec3.
using platform::Vec3;
using platform::Point3;

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

    /// Coulomb friction coefficient of the body's surface, in [0, 1].
    ///
    /// @note **A character controller wants 0.** It implements its own friction
    ///       as a tunable; leaving surface friction on means two friction models
    ///       fight each other, the player never reaches its configured max
    ///       speed, and neither model can be tuned meaningfully.
    ///       Negative leaves the library default in place.
    float friction = -1.0f;

    /// Prevents the body rotating.
    ///
    /// @note Essential for a character: without it the player's shape tips over
    ///       the first time it touches anything, and the camera goes with it.
    bool lockRotation = false;

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

    /// @returns The body's position blended between the previous and current
    ///          simulation states, or `nullopt` if the handle is stale.
    /// @param alpha Progress toward the next tick, in [0, 1). At 0 this is the
    ///        previous state; approaching 1 it approaches the current one.
    ///
    /// @note This is what presentation draws from. Drawing raw current state
    ///       visibly stutters whenever the tick and the frame disagree, which at
    ///       144fps against a 60Hz simulation is most frames.
    std::optional<Point3> interpolatedPosition(BodyHandle handle, double alpha) const;

    /// @returns The body's linear velocity in metres per second, or `nullopt`
    ///          if the handle is stale.
    std::optional<Vec3> linearVelocity(BodyHandle handle) const;

    /// Sets linear velocity directly.
    ///
    /// @note Direct velocity rather than force is the right tool for a character
    ///       controller: a shooter wants crisp response, and driving a character
    ///       with forces through a solver gives the mushy, mass-dependent feel
    ///       that forces are good at and this is not.
    void setLinearVelocity(BodyHandle handle, Vec3 velocity);

    /// What a ray hit.
    struct RayHit {
        BodyHandle body;
        Point3 point;
        Vec3 normal;
        float fraction = 0.0f; ///< Along the ray, in [0, 1].
    };

    /// Casts a ray and returns the closest hit.
    ///
    /// @param origin      Where the ray starts.
    /// @param translation Direction **and length**; not normalised.
    /// @returns The closest hit, or `nullopt` if nothing was struck.
    std::optional<RayHit> raycastClosest(Point3 origin, Vec3 translation) const;

    /// A read-only view of one body, for inspection.
    struct BodyView {
        BodyHandle handle;
        Point3 position; ///< Already interpolated by the supplied alpha.
        Shape shape;
        BodyType type;
    };

    /// Visits every live body.
    ///
    /// @remarks
    /// An inspection API, used by the debug view. Gameplay does not enumerate
    /// bodies - it holds handles to the ones it created.
    ///
    /// Takes a callback rather than returning a container so that walking the
    /// world allocates nothing, which matters because this runs in the render
    /// path (engine_protocol.md - nothing allocates in the hot path).
    void forEachBody(double alpha, const std::function<void(const BodyView&)>& visit) const;

    /// What the solver debug overlay should draw.
    ///
    /// @remarks
    /// Shapes default to **off**: the interpolated view already draws those, and
    /// more smoothly. What this overlay adds is everything our own record of the
    /// world cannot show, because it comes from the solver rather than from us.
    struct SolverDebugOptions {
        bool shapes = false;         ///< Solver's own shape outlines. Not interpolated.
        bool contacts = true;        ///< Actual contact points.
        bool contactNormals = true;  ///< Direction of each contact.
        bool bounds = false;         ///< Broadphase AABBs.
        bool centerOfMass = false;   ///< Mass and centre of mass of dynamic bodies.
    };

    /// Draws the solver's own view of the world through `renderer`.
    ///
    /// @remarks
    /// Authoritative in a way the interpolated view is not: it reports what
    /// Box3D actually holds, so a mismatch between our record of a shape and the
    /// solver's is visible here and nowhere else.
    ///
    /// Drawn at the current simulation transform, **not interpolated**. For a
    /// diagnostic overlay that is the right trade - truth over smoothness.
    void debugDrawSolver(platform::Renderer& renderer, const SolverDebugOptions& options) const;

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

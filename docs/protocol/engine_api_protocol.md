# Engine API Protocol

The job description for work that **crosses the engine/game boundary** - changing the interface gameplay calls, or the platform interface the engine calls.

Engine-layer rules are [engine_protocol.md](engine_protocol.md); gameplay rules are [gameplay_protocol.md](gameplay_protocol.md); content and loaders are [content_protocol.md](content_protocol.md); the TDD loop and shared conventions are [core_protocol.md](core_protocol.md).

> **Why this needs its own protocol:** unlike a published HTTP API, a change here **fails silently and everywhere at once**. There is no version negotiation, no deploy skew, no consumer running the old shape. Everything recompiles against the new meaning immediately, and code that still compiles can be subtly wrong.

---

## The seam is the interface, not the implementation

The dependency runs one way: `game -> engine -> platform`. Each layer depends on the one below **through its interface**, never through its internals.

- **Gameplay calls the engine API and nothing below it.** No platform calls, no driver handles, no direct filesystem access, no wall-clock reads, and no third-party library the engine wraps. If gameplay includes `box3d.h` or `raylib.h`, the boundary has been breached.
- **The engine never calls into gameplay.** Where it must hand control back, it does so through a callback, an event, or an interface the engine defines and gameplay implements.
- **Engine code carries no game nouns.** `Player`, `Enemy`, and `Inventory` do not appear in a renderer, an allocator, or a physics wrapper. A subsystem that needs a game concept to work needs a more general parameter instead.
- **Handles cross the boundary, not raw pointers.** Gameplay receives an opaque handle it cannot dereference or free. A stale handle is detected and reported, never followed.

> **What not to do:** do not reach around the boundary because the engine API cannot express what you need. **That is an engine change, raised as one.** Reimplementing an engine capability inside a gameplay system is the same violation with extra steps.

---

## Additive by default

- **Add functions and parameters; do not repurpose existing ones.** Changing what a call *does* while keeping its signature is the worst available option: nothing fails to compile and every caller is now quietly wrong.
- **Deprecate before removing.** Mark it, migrate the callers, then delete, in that order. Removal and migration land in the **same change** (Enforcement Rule 10, no dead code).
- **A behavior change is a flagged change.** When a change alters behavior existing callers rely on, say so explicitly and name the affected systems. Do not leave the blast radius for someone else to discover.
- **Semantics are documented at the boundary** - units, coordinate handedness, ownership, and whether a call is valid mid-tick belong in the docstring, not in a caller's memory.

> **Gotcha:** "it still compiles" is not evidence of compatibility here. Changing a function from metres to units, or from radians to degrees, compiles perfectly and breaks every caller silently. Units and handedness are part of the contract.

---

## Both sides are test-first

A change that crosses the boundary writes failing tests first, on **each** side it touches:

1. **Engine side:** a unit test over the new interface with plain inputs and outputs. No window, no device, no clock.
2. **Gameplay side:** a simulation test that exercises the rule through the interface, stepping fixed ticks with an injected clock and seeded RNG.

Both sides land in the same change, since both live in this repo. There is no deploy skew to coordinate and no reason to split them.

> **Order matters:** agree the interface shape first, write the failing tests, then implement to green. An interface designed after its implementation tends to leak the implementation.

---

## Wrapping a third-party library

The engine wraps external libraries (raylib, Box3D) so the rest of the codebase depends on **our** interface rather than theirs.

- **The wrapper is a real boundary, not a passthrough.** It exposes what this game needs, in this project's vocabulary and units, rather than re-exporting the library's surface.
- **The library's types do not escape the wrapper.** A `b3BodyId` or a raylib `Texture2D` in a gameplay signature means the wrapper failed.
- **Swapping the library should be an engine-local change.** If replacing the physics engine would touch `game/`, the wrapper is too thin.
- **Configuration that affects determinism is owned by the wrapper and locked** - Box3D's worker count is a determinism decision, not a caller's option ([locked_decisions.md](../reference/locked_decisions.md)).

> **What not to do:** do not add a wrapper function that exists only to pass through a library call the game does not need yet. Build the specific case; the abstraction is earned by real use (Enforcement Rule 5).

---

## Errors at the boundary

- **Validate once, at the boundary.** The public engine call checks its inputs; internals trust what they were handed.
- **The engine reports failure; the game decides the response.** Return a typed result. Whether a missing texture is fatal is a game decision.
- **Assert on programmer error, handle operational error.** A violated invariant is loud in a development build. A missing file or a lost device is expected and handled.

---

## When you are doing engine API work, you own

- A one-way dependency, provable by inspection: no engine header above its layer, no game noun below it.
- An additive change, or an explicit deprecation with its callers migrated in the same change.
- Documented semantics at the boundary: units, handedness, ownership, validity.
- A failing test on **both** sides before the implementation.
- A wrapper thick enough that swapping the underlying library stays engine-local.
- An explicit flag when behavior changes underneath an existing caller.

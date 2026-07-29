# Runtime Architecture

**Scope:** the core runtime architecture for `doomer` - a desktop C++ shooter with a Diablo-like loot loop. This document fulfils `ADE-5` ("Define core runtime architecture") and establishes the boundaries between runtime, gameplay systems, content integration, and save/load.

**This is MVP structure, not a general-purpose reusable engine.** Where a choice traded generality for getting the loop playable, that is deliberate and noted.

**Governing protocols:** [engine_protocol.md](../protocol/engine_protocol.md) (the layer the game is built on), [gameplay_protocol.md](../protocol/gameplay_protocol.md) (the rules of the game), [game_test_protocol.md](../protocol/game_test_protocol.md) (how any of it is tested). Stack decisions locked here are registered in [locked_decisions.md](../reference/locked_decisions.md).

---

## Decision summary

| Area          | Decision                                                                                   |
|---------------|--------------------------------------------------------------------------------------------|
| Base library  | raylib 5.x - window, input, audio, 3D rendering. The game owns its own main loop.          |
| Language      | C++23                                                                                      |
| Build         | CMake + FetchContent                                                                       |
| Target        | Windows, cross-compiled with MinGW-w64 (verified, ADE-22)                                  |
| Dev host      | WSL2; cross-compile to Windows, run the .exe natively on the host                          |
| Tests         | GoogleTest + GoogleMock                                                                    |
| Entity model  | Object-oriented hierarchy with injected context                                            |
| Update order  | Explicit ordered calls in `World::tick`                                                    |
| Events        | Immediate dispatch, with three mandatory guardrails                                        |
| Simulation    | Fixed 60 Hz step; presentation uncapped and interpolated                                   |
| Data format   | JSON (nlohmann/json) for tunables, content, and saves                                      |
| Save model    | Checkpoint at level boundaries                                                             |
| Physics       | Box3D, single-threaded, stepped once per fixed tick. Physics-authoritative transforms      |
| Levels        | Brush lists in JSON; brushes become static Box3D colliders at load                         |
| Actors        | 2D billboard sprites                                                                       |
| Assets        | WAD/lump format-targeted loader; FreeDoom for distribution                                 |
| Audio         | In scope - SFX and music                                                                   |

---

## Inherited constraints

These are already binding via the protocols. They are recorded here because the architecture is shaped by them, not because this document decides them.

| Constraint                                               | Source                        |
|----------------------------------------------------------|-------------------------------|
| Injected clock - nothing reads the wall clock            | game_test_protocol.md, seam 1 |
| Seeded, injected RNG - no global random source           | game_test_protocol.md, seam 2 |
| Headless simulation - rules step with no window          | game_test_protocol.md, seam 3 |
| No hidden global state - context passed explicitly       | game_test_protocol.md, seam 4 |
| Fixed-step simulation, variable-rate presentation        | engine_protocol.md            |
| Stable iteration order wherever output is observable     | engine_protocol.md            |
| No allocation in the hot path                            | engine_protocol.md            |
| Engine code contains no game nouns; dependency runs one way | engine_protocol.md         |
| Engine reports typed failure; the game decides the response | engine_protocol.md         |
| Every tunable value lives in a data file, named          | gameplay_protocol.md          |

---

## Source layout

```
doomer/
├── src/
│   ├── platform/       ← OS seam: window, GPU, audio device, filesystem, clock
│   ├── engine/         ← math, physics (Box3D wrapper), render, input, resources, events
│   ├── game/           ← entities, systems, combat, loot, levels, save
│   └── main.cpp        ← bootstrap
├── tests/
│   ├── engine/         ← unit tests, no hardware
│   ├── game/           ← simulation tests, headless
│   └── golden/         ← golden-image references
├── data/               ← JSON tunables, content definitions, levels
├── assets/             ← committed art (placeholder, generated)
├── resources/          ← local Doom data, gitignored, never committed
└── CMakeLists.txt
```

The dependency direction is `game -> engine -> platform`, and it is one way. Because the tiers are directories, a violating `#include` is visible in the path itself during review.

`platform/` is the only tier permitted to touch the OS, a driver, or a device. Everything above it talks to an interface. This is what keeps the test suite runnable in CI.

---

## The frame loop

Simulation advances on a fixed step. Presentation runs free and interpolates between the two most recent simulation states.

| Constant            | Value        | Meaning                                            |
|---------------------|--------------|----------------------------------------------------|
| `SIM_TICK_HZ`       | 60           | Fixed simulation rate                              |
| `SIM_TICK_SECONDS`  | 1/60         | 16.67 ms per tick                                  |
| `FRAME_BUDGET_MS`   | 6.94         | Presentation target (144 fps)                      |
| `MAX_CATCHUP_TICKS` | 5            | Cap on ticks per frame, preventing a death spiral  |

These are **locked numbers** in the sense of `engine_protocol.md`: a change that blows one halts and is flagged rather than merged with a note. A performance claim without a measurement is a guess.

```
accumulator += frameDelta
ticks = 0
while accumulator >= SIM_TICK_SECONDS and ticks < MAX_CATCHUP_TICKS:
    world.tick(SIM_TICK_SECONDS, rng, events)
    accumulator -= SIM_TICK_SECONDS
    ticks += 1
alpha = accumulator / SIM_TICK_SECONDS
render(world, alpha)
```

Gameplay rules live in `tick`. Anything smoothed for the eye lives in `render` and must never feed back into the rules.

---

## Entity model

Entities are an object-oriented hierarchy: an `Entity` base with `Player`, `Enemy`, `Projectile`, and `Pickup` deriving from it.

This model is only compatible with the four seams under a constraint, and the constraint is not optional:

```cpp
class Entity {
public:
    virtual void update(float dt, Rng& rng, EventQueue& events) = 0;
    virtual ~Entity() = default;
protected:
    BodyHandle body_;   // opaque; Box3D owns position and velocity
};
```

**Transforms are physics-authoritative.** Box3D owns position and velocity for anything physical; an entity holds an opaque `BodyHandle` and reads its transform through the engine API. There is one source of truth, so there is no per-tick sync step and no desync class of bug. The handle is opaque by design: gameplay cannot dereference or free it, and a stale handle is detected and reported rather than followed (`engine_protocol.md`).

The cost is that entity state is no longer purely plain data, so a simulation test builds a physics world. That world is still headless, so the tests still run in CI.

**Rules that make the hierarchy testable:**

- **Context is injected, never fetched.** Time, randomness, and the event sink arrive as parameters. An entity that reads a clock or reaches for a global RNG is a defect.
- **Entities never present.** No entity draws, plays a sound, or touches a device. It mutates its own state and emits events; presentation observes.
- **Ordering never lives inside an entity.** An entity knows how it behaves, not when it runs relative to other systems. That belongs to `World::tick`.
- **Prefer composition past the third variant.** Deep inheritance is where this model fails. When a fourth enemy variant wants to mix behaviors, add a component or a data-driven behavior field rather than another subclass layer.

**Known tradeoff:** virtual dispatch per entity per tick is a cost this model accepts, and state is easier to accidentally hide inside an object than in plain data. The rules above are what hold the seams open in spite of that. If entity counts grow to where dispatch shows up in a measurement, that measurement is the trigger to revisit, not a hunch.

---

## System update order

`World::tick` calls each system by name in a fixed, readable order. Order is stable by construction, which is what `engine_protocol.md` requires, and the whole sequence is visible in one place.

```cpp
void World::tick(float dt, Rng& rng, EventQueue& events) {
    stepInput(*this, dt);
    stepPlayer(*this, dt, events);
    stepEnemies(*this, dt, rng, events);
    stepProjectiles(*this, dt, events);
    stepPhysics(*this, dt, events);   // one Box3D step, emits contact events
    stepDamage(*this, events);
    stepPickups(*this, events);
    sweepDead(*this);
}
```

Each `step*` function iterates its collection and invokes the entity's `update`. The loop owns *when*; the class owns *how*. Reordering is a deliberate code change, reviewed as one.

`sweepDead` is the single point where entities are removed. See the event guardrails below for why.

---

## Event dispatch

Events dispatch **immediately**: publishing runs the handlers inline, right then. This gives zero-latency cause and effect and control flow that is followable in a debugger.

Immediate dispatch does not satisfy `engine_protocol.md`'s stable-iteration-order rule on its own. Three guardrails are therefore **requirements, not suggestions**:

1. **Static registration.** Handlers are registered once at startup, in a fixed and explicit order. There is no runtime subscribe. This makes handler order deterministic and reviewable.
2. **No structural mutation from a handler.** A handler may not add or remove entities. It marks an entity dead; `sweepDead` removes it at the end of the tick. This eliminates iterator invalidation.
3. **Reentrancy guard.** Dispatch maintains a depth counter and asserts loudly in development builds if a handler re-enters the system it was dispatched from. Event cycles (died -> drop -> pickup -> died) are exactly what the loot systems will produce, and a silent stack overflow is the worst way to find one.

Without all three, this design is non-compliant. A change that weakens one of them is a flagged change.

---

## Data and tunables

Every tunable is a named entry in a JSON file under `data/`, parsed with nlohmann/json. A numeric literal in gameplay logic is a defect (`gameplay_protocol.md`).

```json
{
  "shotgun": {
    "damage": 8.0,
    "pellets": 9,
    "fireIntervalSeconds": 0.85,
    "spreadDegrees": 12.0
  }
}
```

- **One name, one source.** A value read by several systems is defined once, not copied.
- **Structural constants are exempt** where they are genuinely not tunable. Say so in a comment when it is not obvious.
- **JSON has no comments.** Where a tuned value needs rationale, that rationale goes in the commit message or in a sibling `*.notes.md`, not in a stripped comment field.

---

## Save model

Saves are written at **level boundaries** - on level exit (`ADE-19`). Mid-level world state is never serialized.

| Persisted                | Not persisted                          |
|--------------------------|----------------------------------------|
| Player stats             | Enemy positions and state              |
| Inventory contents       | Projectiles in flight                  |
| Equipped items           | Mid-level timers                       |
| Level progress           | Any transient world state              |
| Currency                 |                                        |
| RNG seed                 |                                        |

This keeps the schema small and stable, and keeps the round-trip test cheap. The cost is accepted deliberately: dying mid-level loses that level's progress, and there is no quicksave.

**Every save file carries a version field.** Per `game_test_protocol.md`, save data requires both a round-trip test and a migration test from each supported older version. A schema change without a migration test is incomplete.

---

## Levels and collision

A level is a list of positioned brushes plus entity placements, in JSON:

```json
{
  "name": "e1m1",
  "brushes": [
    { "pos": [0, 0, 0], "size": [10, 3, 10] },
    { "pos": [10, 0, 2], "size": [4, 3, 6] }
  ],
  "spawn": [1, 0, 1],
  "entities": [
    { "type": "enemy.imp", "pos": [8, 0, 4] }
  ]
}
```

Brushes allow arbitrary room shapes, ramps, and varied ceiling heights, which a grid could not express.

**Collision and physics are Box3D** ([erincatto/box3d](https://github.com/erincatto/box3d), MIT, portable C17, no external dependencies). At level load each brush becomes a static collider; dynamic actors get bodies.

Box3D sits in `engine/`, behind the engine API. Gameplay never calls it directly, per `gameplay_protocol.md`: gameplay calls the engine API and nothing below it. If the engine wrapper cannot express what a system needs, that is an engine change raised as one.

**Configuration is locked for determinism:**

| Setting      | Value                   | Reason                                                               |
|--------------|-------------------------|----------------------------------------------------------------------|
| Worker count | 1 (single-threaded)     | Multithreaded solving can reorder constraint resolution between runs |
| Step         | `SIM_TICK_SECONDS`      | One step per fixed tick, never per rendered frame                    |

Box3D advertises multithreading and SIMD. Threading is deliberately off: `engine_protocol.md` requires stable iteration order, and `game_test_protocol.md` treats a flaky test as a determinism bug rather than something to retry or loosen. Enabling threading is a decision to revisit **with a measurement**, not a hunch.

**Actors are 2D billboard sprites,** not 3D models. Combined with 3D brush geometry this is Doom's own architecture, and it matches the sprite content available in `resources/`.

---

## Assets and licensing

The loader targets the **WAD/lump format**, never specific content. This distinction is load-bearing.

| Use          | Source                                                              |
|--------------|---------------------------------------------------------------------|
| Local dev    | Doom and Doom II data in `resources/` - gitignored, never committed |
| Distribution | FreeDoom - libre, drop-in compatible lump names and conventions     |

`resources/` contains commercial id Software and Bethesda copyrighted material. It is valid for local prototyping and it must never be committed or shipped. Because the loader targets the format, the swap to FreeDoom requires no code change.

**Before any distribution**, the FreeDoom swap must be verified. This is tracked, not assumed.

---

## Audio

Audio is **in scope**. raylib provides `LoadSound`/`PlaySound` for WAV and streaming for MIDI and MP3, and `resources/` already carries the sound lumps, MIDI soundtrack, and a soundfont.

Audio is a `platform/` capability behind the seam like any other device. Gameplay emits an event; a presentation-side handler plays the sound. No entity plays audio directly.

This is in scope specifically because `ADE-13` (damage feedback) and `ADE-14` (combat reactions) are partly audio problems, and a silent build makes those playtests much weaker.

---

## Testing

Per-layer expectations, from `game_test_protocol.md`:

| Layer                              | Test written first                                                                         |
|------------------------------------|--------------------------------------------------------------------------------------------|
| Engine math, input mapping         | Unit test over plain inputs and outputs. No window, no device, no clock.                   |
| Physics behavior                   | Simulation test: headless Box3D world, step N fixed ticks, assert within an epsilon.       |
| Level and save serialization       | Round-trip test, plus a migration test from each supported older version.                  |
| Gameplay rules                     | Simulation test: build state, step N fixed ticks with injected clock.                      |
| Randomized behavior (loot, spread) | Seeded RNG, assert the exact outcome; distribution assertion where spread is the behavior. |
| Rendering                          | Golden-image test against a committed reference, within a threshold.                       |
| Feel and balance                   | Not a test. Playtest with written acceptance criteria.                                     |

GoogleMock mocks the `platform/` interfaces, which is what keeps engine and gameplay tests hardware-free and therefore runnable in CI.

A flaky test here is a determinism bug and is treated as the finding, never retried or loosened.

---

## Non-goals

Explicitly deferred. Naming these is what stops speculative generality leaking in.

| Non-goal                    | Note                                                                                                                                 |
|-----------------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| Multiplayer and networking  | Single-player only. The fixed tick and seeded RNG would be the right foundation later, so deferring does not paint us into a corner. |
| Procedural level generation | All levels hand-authored as brush lists.                                                                                             |
| Level editor tooling        | Deferred for MVP, but an early spike evaluates authoring ergonomics after roughly three levels exist.                                |

---

## Open risks

| Risk                                                                                    | Action                                                                 |
|-----------------------------------------------------------------------------------------|------------------------------------------------------------------------|
| The game's real frame budget is unmeasured. ADE-22 measured a trivial scene, not a populated level | Re-measure once sprites, physics, and a HUD are in a level. |
| `ADE-21` mapping is underscoped - deriving a map from brush geometry is materially harder than a grid         | Rescope the issue before it is picked up.        |
| Doom assets cannot be distributed                                                      | FreeDoom swap verified before any release; `resources/` gitignored.     |
| Brush lists get painful to hand-author past a few rooms                                | Spike issue evaluates authoring ergonomics after roughly three levels.  |

---

## Related

- [engine_protocol.md](../protocol/engine_protocol.md) - engine layer rules
- [gameplay_protocol.md](../protocol/gameplay_protocol.md) - gameplay layer rules
- [game_test_protocol.md](../protocol/game_test_protocol.md) - test-first for game code
- [core_protocol.md](../protocol/core_protocol.md) - shared development core
- [locked_decisions.md](../reference/locked_decisions.md) - the canon
- [glossary.md](../reference/glossary.md) - shared vocabulary

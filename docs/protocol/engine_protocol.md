# Engine Protocol

The job description for **engine code**: the layer the game is built on, written by us rather than adopted. Platform access, the frame loop, memory, math, resource handling, rendering, audio, input.

Cross-cutting rules (folder structure, naming, code quality, env vars, commits, TDD loop, branching, docstrings) live in the shared core [core_protocol.md](core_protocol.md). Game rules and content-facing code live in [gameplay_protocol.md](gameplay_protocol.md). How any of this gets tested is [game_test_protocol.md](game_test_protocol.md).

> **Stack note:** written for a game built from code rather than on a premade engine, so "the engine" is part of this codebase and under your control. If the project sits on a third-party engine, the rules still apply to the layer you write against it - see [Using a premade engine](#using-a-premade-engine).

---

## Engine code knows nothing about the game

The engine provides capabilities; it never encodes what the game *is*.

- **No game nouns in engine code.** `Player`, `Enemy`, `Inventory`, `Level 3` do not appear in a renderer, an allocator, or an input backend. If a subsystem needs to know a game concept to work, the concept belongs on the gameplay side and the subsystem needs a more general parameter.
- **The dependency runs one way.** Engine code never calls into gameplay code. Where the engine needs to hand control back, it does so through a callback, an event, or an interface the engine defines and gameplay implements.
- **Content-shaped constants are not engine constants.** A hardcoded max entity count, a fixed number of weapon slots, or a specific asset path in engine code is a game decision wearing an engine costume.

> **What not to do:** do not special-case one game system inside a general subsystem "just for now". That branch never gets removed, and it makes the subsystem untestable in isolation because the test now needs a game to exist.

---

## The engine API is an internal contract

Everything above it depends on it, and unlike a published HTTP API a change here fails silently and everywhere at once.

- **Additive by default.** Add functions and parameters; do not repurpose the meaning of an existing one. Changing what a call *does* while keeping its signature is the worst available option - nothing fails to compile and every caller is now subtly wrong.
- **Deprecate before removing.** Mark it, migrate the callers, then delete, in that order. Removal and migration land in the same change (Enforcement Rule 10, no dead code).
- **Changing engine behavior is a flagged change.** Say so explicitly when a change alters behavior existing gameplay relies on, and name the systems affected. Do not leave the blast radius for someone to discover.
- **Semantics are documented at the boundary.** Units, coordinate handedness, ownership, and whether a call is valid mid-frame belong in the docstring, not in a caller's memory.

---

## Resource ownership and lifetime

- **Every resource has one owner,** named in the API. Buffers, textures, file handles, sockets, audio voices: it is explicit who releases it and when.
- **Every acquire has a matching release,** including on the error path. An early return that skips cleanup is a leak, not a shortcut.
- **Teardown is exercised, not assumed.** Create-and-destroy is a tested cycle; a subsystem that only works because the process exits is not finished.
- **Handles, not raw pointers, across the boundary.** Gameplay receives an opaque handle it cannot dereference or free. A stale handle is detected and reported, never followed.

---

## Platform access sits behind a seam

This is the rule that makes everything else testable.

- **Platform calls are isolated in a backend layer** - windowing, GPU, audio device, filesystem, clock, network. Subsystem logic above that layer talks to an interface, never the OS or a driver directly.
- **The logic half of every subsystem runs without hardware.** Culling, mixing math, input mapping, layout, and timing decisions are computable with no window, no GPU, and no audio device. Only the final submission touches the backend.
- **No hidden global state.** Singletons and file-scope mutable state make two tests share a world and make hot reload impossible. Pass context explicitly, even when there is only ever one of them.

> **Gotcha:** the moment a subsystem needs a live device to answer a question, its tests need one too, and they stop running in CI. Push the decision up into the logic half and leave the backend as a dumb executor.

---

## Time and determinism

- **The engine owns the clock and hands it out.** Subsystem logic receives time as a parameter; it never reads the wall clock itself. An injectable clock is a hard requirement, not a testing convenience ([game_test_protocol.md](game_test_protocol.md)).
- **Simulation advances on a fixed step; presentation runs on a variable one.** Keep the two separate and say which one a new subsystem belongs to.
- **Randomness is a seeded, injected source.** A subsystem that reaches for a global RNG cannot be reproduced, and any test over it is flaky by construction.
- **Iteration order is stable.** Anything that walks a collection and produces observable output does so in a defined order. Unordered container iteration is a determinism bug waiting for a different allocator.

---

## The frame loop is a budget

- **Nothing allocates in the hot path.** No per-frame heap allocation, string formatting, logging in the common case, map or table growth, or lookup-by-name. Allocate up front, reuse buffers, and pool what churns.
- **No blocking work in the loop.** File I/O, network calls, device queries, and lock contention move off the frame thread or behind an already-loaded cache.
- **Budgets are locked numbers.** Frame time, allocation count, and peak memory targets belong with the project's other locked values; a change that blows one halts and is flagged rather than merged with a note.
- **Measure before claiming.** A performance statement without a measurement is a guess. This is the Accuracy Bar applied to the frame loop: cite the number, or say you did not take one.

---

## Errors

- **Validate once, at the boundary.** The public engine call checks its inputs; internals trust what they were handed rather than re-checking at every level.
- **The engine reports failure; it does not decide the game's response.** Return a typed error or result. Whether a missing asset is fatal is a game decision.
- **Assert on programmer error, handle operational error.** A violated invariant should be loud in a development build. A missing file, a lost device, or a closed socket is expected and handled.

---

## Testing

Engine code is the most testable code in the project and gets **no TDD exception**: the logic is pure, the inputs are data, and the platform is behind a seam. Math, allocators, serialization, collision, input mapping, and timing are all written test-first per [core_protocol.md](core_protocol.md#test-first-development-tdd).

The one genuinely unobservable output is rendered pixels, which is what golden-image tests are for. Seams, time control, floating-point comparison, and the golden-image workflow are all defined in [game_test_protocol.md](game_test_protocol.md).

---

## Using a premade engine

If the project adopts a third-party engine, this document describes the layer *you* write against it, and the rules carry over with different names:

- "Engine code knows nothing about the game" becomes: shared framework and utility code carries no references to specific game content.
- "Platform access sits behind a seam" becomes: your logic does not call engine APIs directly where it can take plain data instead, so it stays testable outside the editor and outside play mode.
- "The frame loop is a budget" becomes: no allocation, string building, or scene-wide lookups in per-frame callbacks.
- Resource ownership becomes the engine's asset lifetime rules, which you follow rather than define.

---

## When you are doing engine work, you own

- A subsystem that is general, owns its resources, and names its lifetimes.
- The stability of the engine API for everything built on it, and an explicit flag when you change behavior underneath a caller.
- A logic half that runs with no hardware, so the tests run in CI.
- Determinism: injected clock, seeded randomness, stable iteration order.
- The frame budget, with a measurement rather than an opinion.

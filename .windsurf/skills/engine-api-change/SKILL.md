---
name: engine-api-change
description: Use when a change crosses the engine/game boundary. Adds or modifies an engine API test-first on both sides, additively.
---

Use when gameplay needs something the engine API does not expose, or an existing engine interface must change. Canonical rules: [engine_api_protocol.md](../../../docs/protocol/engine_api_protocol.md).

> A change here **fails silently and everywhere at once**. There is no version negotiation and no old consumer still on the previous shape - everything recompiles against the new meaning immediately, and code that still compiles can be quietly wrong.

## Steps (in order)

1. **Design the interface first**, before its implementation. Units, coordinate handedness, ownership, and mid-tick validity go in the docstring.
2. **Additive by default.** Add functions and parameters; never repurpose an existing one. A genuinely breaking change deprecates first: mark, migrate callers, delete - all in the same change.
3. **Engine side, test-first:** a unit test over the new interface with plain inputs and outputs. No window, no device, no clock. Run it red.
4. **Gameplay side, test-first:** a simulation test exercising the rule through the interface with an injected clock and seeded RNG. Run it red.
5. **Implement to green.** Both sides land in the same change.
6. **Flag any behavior change** and name the systems affected.

## Wrapping a third-party library

The wrapper exposes what this game needs in this project's vocabulary and units, not the library's surface. The library's types must not escape it - a `b3BodyId` or a raylib `Texture2D` in a gameplay signature means the wrapper failed. Determinism-affecting configuration is owned by the wrapper and locked.

## Checks
- [ ] interface designed and documented before implementation
- [ ] additive, or deprecated with callers migrated in the same change
- [ ] units, handedness, ownership, mid-tick validity documented
- [ ] failing test on **both** sides before implementation
- [ ] handles cross the boundary, not raw pointers; typed errors returned
- [ ] no engine or third-party header included above its layer
- [ ] behavior changes flagged with affected systems named
- [ ] build + tests green

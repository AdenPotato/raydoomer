---
name: engine-api-change
description: Change the engine/game boundary - add or modify an engine API, test-first on both sides, additively.
---

Use when a change crosses the engine/game boundary: gameplay needs something the engine API does not expose, or an existing engine interface must change. Canonical rules: [engine_api_protocol.md](../../../docs/protocol/engine_api_protocol.md).

> A change here **fails silently and everywhere at once**. There is no version negotiation and no old consumer still running the previous shape - everything recompiles against the new meaning immediately, and code that still compiles can be quietly wrong.

## Steps (in order)

1. **Design the interface first**, before its implementation. Name the units, the coordinate handedness, the ownership, and whether the call is valid mid-tick. Those belong in the docstring, not in a caller's memory.
2. **Additive by default.** Add functions and parameters; do not repurpose an existing one. If the change is genuinely breaking, deprecate first: mark it, migrate the callers, then delete - all in the same change (Enforcement Rule 10).
3. **Engine side, test-first:** a unit test over the new interface with plain inputs and outputs. No window, no device, no clock. Run it red.
4. **Gameplay side, test-first:** a simulation test exercising the rule through the interface, stepping fixed ticks with an injected clock and seeded RNG. Run it red.
5. **Implement to green.** Both sides land in the same change - they live in the same repo, so there is no skew to coordinate.
6. **Flag any behavior change** explicitly and name the systems affected. Do not leave the blast radius to be discovered.

## If you are wrapping a third-party library

- The wrapper exposes what this game needs in this project's vocabulary and units, not the library's surface.
- The library's types must not escape the wrapper. A `b3BodyId` or a raylib `Texture2D` in a gameplay signature means the wrapper failed.
- Determinism-affecting configuration is owned by the wrapper and locked, never a caller's option.

## Checks
- [ ] interface designed and documented before implementation
- [ ] additive, or deprecated with callers migrated in the same change
- [ ] units, handedness, ownership, and mid-tick validity documented at the boundary
- [ ] failing test on **both** sides before implementation
- [ ] handles cross the boundary, not raw pointers; typed errors returned, not decisions made
- [ ] no engine or third-party header included above its layer
- [ ] behavior changes flagged with affected systems named
- [ ] build + tests green

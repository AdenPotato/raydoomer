---
name: new-system
description: Add a gameplay or engine system with a failing simulation test first, tunables in data, and a place in the fixed-step tick order.
---

Use to add a system - a gameplay rule (combat, loot, enemy behavior, progression) or an engine subsystem. Canonical rules: [gameplay_protocol.md](../../../docs/protocol/gameplay_protocol.md) and [engine_protocol.md](../../../docs/protocol/engine_protocol.md); testing: [game_test_protocol.md](../../../docs/protocol/game_test_protocol.md). If the system needs something the engine API cannot express, run `/engine-api-change` first.

## Steps (in order)

1. **Tunables first:** every value a designer would change goes into a `data/` JSON file, named. If the system needs new tunables, add them before the code so the test reads them rather than literals. Run `/data-change` if the schema is new.
2. **Failing test first:** write the simulation test that captures the behavior - build the minimal state directly, step a known number of fixed ticks with the injected clock and a seeded RNG, assert the resulting state or the emitted events. Cover the tick boundary (the tick before, at, and after a threshold). Run it red.
3. **Write the system** as a `step*` function taking `(World&, float dt, Rng&, EventQueue&)` as it needs them. Rules operate on plain state and emit events; they never draw, play audio, or read the clock.
4. **Place it in the tick order** explicitly in `World::tick`. Ordering lives in the loop, never inside an entity.
5. **Implement to green.** Add docstrings to exported functions and classes.
6. **State what you verified and what you did not.** A green test covers the slice it covers; anything about feel goes to a playtest with written criteria.

## Checks
- [ ] failing simulation test shown before implementation
- [ ] no numeric literal in the logic; every tunable named in `data/`
- [ ] injected clock and seeded RNG; no global state; runs headless
- [ ] placed explicitly in the `World::tick` order
- [ ] tick-boundary cases asserted; floats compared within a deliberate epsilon
- [ ] behavior vs balance stated; no claimed feel outcome
- [ ] build + tests green

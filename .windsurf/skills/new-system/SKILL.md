---
name: new-system
description: Use when adding a gameplay or engine system. Scaffolds it with a failing simulation test first, tunables in data, and a place in the fixed-step tick order.
---

Add a system - a gameplay rule (combat, loot, enemy behavior, progression) or an engine subsystem. Canonical rules: [gameplay_protocol.md](../../../docs/protocol/gameplay_protocol.md) and [engine_protocol.md](../../../docs/protocol/engine_protocol.md); testing: [game_test_protocol.md](../../../docs/protocol/game_test_protocol.md). If the engine API cannot express what the system needs, use the `engine-api-change` skill first.

## Steps (in order)

1. **Tunables first:** every value a designer would change goes into a `data/` JSON file, named, before the code exists. Use the `data-change` skill if the schema is new.
2. **Failing test first:** a simulation test that builds the minimal state directly, steps a known number of fixed ticks with the injected clock and a seeded RNG, and asserts the resulting state or emitted events. Cover the tick before, at, and after any threshold. Run it red.
3. **Write the system** as a `step*` function taking `(World&, float dt, Rng&, EventQueue&)` as needed. Rules operate on plain state and emit events; they never draw, play audio, or read the clock.
4. **Place it explicitly in the `World::tick` order.** Ordering lives in the loop, never inside an entity.
5. **Implement to green.** Docstrings on exported functions and classes.
6. **State what you verified versus what you changed.** Feel goes to a playtest, never a claimed outcome.

## Checks
- [ ] failing simulation test shown before implementation
- [ ] no numeric literal in the logic; every tunable named in `data/`
- [ ] injected clock and seeded RNG; no global state; runs headless
- [ ] placed explicitly in the tick order
- [ ] tick boundaries asserted; floats within a deliberate epsilon
- [ ] behavior vs balance stated
- [ ] build + tests green

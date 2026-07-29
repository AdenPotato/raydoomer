---
trigger: model_decision
description: Engine subsystems and gameplay rules - frame loop, physics, collision, math, combat, enemies, loot, progression. Use when building or changing what the game does.
---

# Simulation role

Canonical rules: [engine_protocol.md](../../docs/protocol/engine_protocol.md) and [gameplay_protocol.md](../../docs/protocol/gameplay_protocol.md). Testing: [game_test_protocol.md](../../docs/protocol/game_test_protocol.md). Shared conventions: [core_protocol.md](../../docs/protocol/core_protocol.md). Crossing the engine/game boundary: [engine_api_protocol.md](../../docs/protocol/engine_api_protocol.md). Follow those documents; do not restate or contradict them. If a rule seems wrong, flag it.

## How you work
- **Test-first.** Engine logic gets a unit test over plain inputs; a gameplay rule gets a simulation test that builds state and steps a known number of fixed ticks. Show it fail, then implement to green.
- **The four seams are not optional:** injected clock, seeded injected RNG, headless stepping, no hidden global state.
- **Rules sit on the fixed step.** Everything continuous scales with delta time; never accumulate per frame. Anything smoothed for the eye is presentation and must not feed back.
- **Every tunable lives in data**, named, one source per value. A numeric literal in gameplay logic is a defect.
- **Gameplay calls the engine API and nothing below it.** If it cannot express what you need, that is an engine change raised as one.
- **Never claim a feel or timing outcome you did not observe.** State whether a change is behavior or balance.
- **House style:** never put a work-item id in source or comments (reference it in commits/PRs only); never use em dashes.

## Done means
Build + tests green, docstrings present, every blocking CI gate passes (run them before pushing), float assertions within a deliberate epsilon, no flaky tests retried or loosened, and the diff self-audited against [engine_protocol.md](../../docs/protocol/engine_protocol.md) and [gameplay_protocol.md](../../docs/protocol/gameplay_protocol.md) before opening the PR. Feel-tuned changes go to a playtest with written criteria.

> Once the source layout exists, switch this rule to `trigger: glob` with `globs: src/engine/**, src/game/**` so it activates automatically.

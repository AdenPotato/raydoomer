---
trigger: model_decision
description: Content definitions, loaders, resources, and save data - JSON schemas, WAD/level loading, resource lifetime, save migrations. Use when changing the data the game is made of.
---

# Content role

Canonical rules: [content_protocol.md](../../docs/protocol/content_protocol.md). Resource lifetime and the platform seam: [engine_protocol.md](../../docs/protocol/engine_protocol.md). Testing: [game_test_protocol.md](../../docs/protocol/game_test_protocol.md). Shared conventions: [core_protocol.md](../../docs/protocol/core_protocol.md). Crossing the engine/game boundary: [engine_api_protocol.md](../../docs/protocol/engine_api_protocol.md). Follow those documents; do not restate or contradict them. If a rule seems wrong, flag it.

## How you work
- **Test-first.** A schema gets a fixture test (valid parses, malformed returns the typed error). A loader gets a round-trip test. Save data gets a round-trip **and** a migration test from each supported older version.
- **Validate once, at the boundary.** Report a typed error naming what failed. Never crash on bad input; never silently default a missing field.
- **Load by format, never by specific content.** That separation is what makes the FreeDoom swap a no-code change.
- **Invariants live in code.** JSON enforces nothing, so every content rule is one you write and test, including cross-reference checks after all content loads.
- **Resources have one named owner**, a matching release on every path, and an exercised teardown. Gameplay gets handles, never pointers.
- **Every save carries a version field**, and a schema change ships with its migration and that migration's test.
- **Fixtures are synthetic and committed.** Never read `resources/` from a test - it is gitignored and absent in CI.
- **House style:** never put a work-item id in source or comments; never use em dashes.

## Done means
Build + tests green, docstrings present, every blocking CI gate passes (run them before pushing), no test depending on `resources/`, and the diff self-audited against [content_protocol.md](../../docs/protocol/content_protocol.md) before opening the PR.

> Once the source layout exists, switch this rule to `trigger: glob` with `globs: src/game/save/**, src/engine/resources/**, data/**` so it activates automatically.

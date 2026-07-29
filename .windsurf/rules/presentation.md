---
trigger: model_decision
description: Rendering, camera, sprites, HUD, and audio output. Use when building or changing anything the player sees or hears.
---

# Presentation role

Canonical rules: [presentation_protocol.md](../../docs/protocol/presentation_protocol.md). Visual tokens: [style_guide.md](../../docs/design/style_guide.md). Testing: [game_test_protocol.md](../../docs/protocol/game_test_protocol.md). Shared conventions: [core_protocol.md](../../docs/protocol/core_protocol.md). Crossing the engine/game boundary: [engine_api_protocol.md](../../docs/protocol/engine_api_protocol.md). Follow those documents; do not restate or contradict them. If a rule seems wrong, flag it.

## How you work
- **Test-first.** Billboard angle, frame selection, sort order, interpolation, and HUD layout are plain logic with a unit test first. Rendered output gets a golden-image test.
- **Presentation observes; it never decides.** No gameplay rule lives here, and nothing written here may write back into simulation state.
- **Draw from interpolated state** blended by the render alpha, never raw current state.
- **Tokens, never literals.** Color, spacing, radius, and type come from the style guide. A raw hex or pixel literal is a defect.
- **Audio is triggered by events**, played by a presentation handler; device access stays in `platform/`.
- **Accessibility is a constraint:** never color alone, contrast that holds against the game world, text legible at the smallest supported resolution.
- **Never claim a visual outcome you did not observe.**
- **House style:** never put a work-item id in source or comments; never use em dashes.

## Done means
Build + tests green, docstrings present, every blocking CI gate passes (run them before pushing), goldens deterministic and small with any re-bake explained in the commit, and the diff self-audited against [presentation_protocol.md](../../docs/protocol/presentation_protocol.md) before opening the PR. Visual quality beyond a golden goes to a playtest.

> Once the source layout exists, switch this rule to `trigger: glob` with `globs: src/engine/render/**, src/game/hud/**` so it activates automatically.

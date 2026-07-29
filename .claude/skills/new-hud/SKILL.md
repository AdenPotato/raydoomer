---
name: new-hud
description: Add a HUD element or presentation feature with a failing test first, style tokens, and accessibility checks, per presentation_protocol.md.
---

Use to add a HUD element, an overlay, or a presentation feature. Canonical rules: [presentation_protocol.md](../../../docs/protocol/presentation_protocol.md); tokens and taxonomy: [style_guide.md](../../../docs/design/style_guide.md).

## Steps (in order)

1. **Failing test first:** write the test capturing the behavior. Layout logic gets a unit test at several viewport sizes asserting computed positions; state-driven display gets a test over plain inputs. Run it red.
2. **Build it:**
   - Read simulation state; never write it. Draw from interpolated state where the value moves.
   - Tokens via the style guide - **no hardcoded colors or magic dimensions**.
   - Scale with the viewport; never hardcode pixel positions for one resolution.
   - Extract repeated element data to a config table and render from one map.
3. **Accessibility, as a hard constraint:** never color alone as a state indicator; contrast that holds against the bright game world, not a flat background; text legible at the smallest supported resolution.
4. **Implement to green**, then verify the smallest supported resolution. Add a golden-image test if the element has meaningful rendered output; keep the scene deterministic (fixed camera, fixed seed, fixed simulated time).
5. **Run the smoke check** by launching the game if it runs on this host. Best-effort - skip with a note if the render path is unavailable; the failing-then-green test in step 1 is the hard requirement.

## Checks
- [ ] failing test shown before implementation
- [ ] tokens only (no hardcoded colors or dimensions); scales with the viewport
- [ ] reads simulation state, never writes it
- [ ] color never the sole state indicator; contrast verified against the world
- [ ] smallest supported resolution verified
- [ ] any golden reference is deterministic and small
- [ ] build + tests green; no claimed visual outcome you did not observe

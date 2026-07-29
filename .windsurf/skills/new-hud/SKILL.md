---
name: new-hud
description: Use when adding a HUD element, overlay, or presentation feature. Scaffolds it with a failing test first, style tokens, and accessibility checks.
---

Add a HUD element, overlay, or presentation feature. Canonical rules: [presentation_protocol.md](../../../docs/protocol/presentation_protocol.md); tokens and taxonomy: [style_guide.md](../../../docs/design/style_guide.md).

## Steps (in order)

1. **Failing test first:** layout logic gets a unit test at several viewport sizes asserting computed positions; state-driven display gets a test over plain inputs. Run it red.
2. **Build it:** read simulation state and never write it; draw from interpolated state where the value moves; tokens only, no hardcoded colors or dimensions; scale with the viewport; extract repeated element data to a config table.
3. **Accessibility as a hard constraint:** never color alone as a state indicator; contrast that holds against the bright game world; text legible at the smallest supported resolution.
4. **Implement to green**, verify the smallest supported resolution, and add a deterministic golden-image test if the element has meaningful rendered output.
5. **Smoke check** by launching the game if the render path works on this host. Best-effort; skip with a note if unavailable.

## Checks
- [ ] failing test shown before implementation
- [ ] tokens only; scales with the viewport
- [ ] reads simulation state, never writes it
- [ ] color never the sole state indicator; contrast verified against the world
- [ ] smallest supported resolution verified
- [ ] goldens deterministic and small
- [ ] build + tests green; no claimed visual outcome

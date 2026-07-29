---
name: new-screen
description: Use when adding a UI screen or component. Scaffolds it with a failing render test first, theme tokens, and the project's smoke check.
---

Canonical rules: [frontend_protocol.md](../../../docs/protocol/frontend_protocol.md); tokens + taxonomy: [style_guide.md](../../../docs/design/style_guide.md). For data, see [integration_protocol.md](../../../docs/protocol/integration_protocol.md).

## Steps (in order)

1. **Failing test first:** write the component/render test capturing the behavior, mocking at the data-layer seam (the query hook or a request mock) with fixtures typed from the API client (matching the service's documented API). Run it red.
2. **Build it:**
   - A **screen** goes in the router's route folder; feature logic in `features/<name>/`.
   - Tokens via the theme mechanism - **no hardcoded colors or magic dimensions**.
   - Pad for safe-area insets; wrap overflow in a scroll container; virtualized lists for data; overlays pinned outside the scroll area.
3. **Implement to green**, then verify the smallest device/viewport and run the project's smoke check - using the editor's preview/browser tooling if it has any, otherwise by running the app manually. The smoke check is best-effort (skip with a note if there is no way to render it); the failing-then-green test in step 1 is the hard requirement.
4. If the screen needs new service data, use the `new-contract` skill first.

## Checks
- [ ] failing render test shown before implementation
- [ ] tokens only (no hardcoded colors); safe-area handled; lists virtualized
- [ ] smoke check passes (preview tooling or a manual run); smallest viewport verified
- [ ] lint + typecheck + tests green

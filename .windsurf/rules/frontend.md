---
trigger: model_decision
description: UI / client surface work - screens, components, navigation, theming, and data wiring. Use when building or changing anything a user sees or interacts with.
---

# Frontend role

Canonical rules: [frontend_protocol.md](../../docs/protocol/frontend_protocol.md). Shared conventions (folder structure, naming, code quality, commits, TDD loop): [core_protocol.md](../../docs/protocol/core_protocol.md). Anything crossing the service<->UI boundary: [integration_protocol.md](../../docs/protocol/integration_protocol.md). Visual tokens + component taxonomy: [style_guide.md](../../docs/design/style_guide.md). Follow those documents; do not restate or contradict them. If a rule seems wrong, flag it.

## How you work
- **Test-first.** Write the failing component/render test that captures the behavior, show it fail, then implement to green.
- **Styling:** tokens only via the theme mechanism - never a hardcoded color or magic dimension; pad for safe-area insets; virtualized lists for data lists; verify the smallest device/viewport.
- **Navigation:** one routing system; prefer native platform primitives over hand-rolled gesture/paging code.
- **Data:** type everything from the SPA's typed API client; talk to the service through that single client; mock at the data-layer seam. Hand contract changes to the integration role.
- Keep changes surgical. Remove replaced code in the same change (no dead code).
- **House style:** never put a work-item id in source or comments (reference it in commits/PRs only); never use em dashes.

## Done means
Lint + typecheck + tests green, every blocking CI gate passes (run them before pushing), and the diff self-audited against [frontend_protocol.md](../../docs/protocol/frontend_protocol.md) before opening the PR. Manual QA is a separate step ([qa_protocol.md](../../docs/protocol/qa_protocol.md)).

> Once your source layout is fixed, you can switch this rule to `trigger: glob` with `globs:` set to your UI paths so it activates automatically on those files.

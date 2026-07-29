# Frontend Protocol

The job description for UI / client work. This document holds frontend-specific rules, examples, and gotchas. Cross-cutting rules (folder structure, naming, code quality, env vars, commits, TDD loop, branching, docstrings) live in the shared core [core_protocol.md](core_protocol.md). The service<->UI contract seam lives in [integration_protocol.md](integration_protocol.md). Visual tokens and component taxonomy live in [style_guide.md](../design/style_guide.md). Manual QA is [qa_protocol.md](qa_protocol.md).

> **Stack note:** the principles below are framework-agnostic. Fill in your client framework's specifics (router, bundler, restart/reload workflow, test runner) where this document references "your" tooling.

---

## Navigation

- **One routing system.** Pick the framework's native router and use it consistently; do not mix routing libraries. Mock the router in component tests that navigate.
- **Prefer native/platform primitives over hand-rolled ones** for gestures, paging, and scrolling. A hand-rolled re-implementation of a behavior the platform already provides is harder to get right and to maintain; reach for it only when the primitive is genuinely insufficient, and record why.

## Styling and theming

- **All color / spacing / radius / type values come from design tokens** - a single source of truth (a theme module or a config). Read them through the framework's theming mechanism; never hardcode a raw color or dimension in a component.
- **No hardcoded colors or magic dimensions.** A raw hex / `rgb()` / pixel literal in a component bypasses theming and any contrast guarantees. If a scan rule enforces this, note whether it runs in CI only vs. the local lint command, and verify by inspection where it does not run locally.
- Component taxonomy (button variants, etc.) and the token tables are canonical in [style_guide.md](../design/style_guide.md). Do not redefine them here.

## Component standards

Beyond the token rules above, components follow a short checklist. Mechanical rules (no hardcoded dimensions, every interactive element carries an accessibility role, no repeated inline style objects) are best enforced by a lint/convention check; the judgment items are on you:

- **Build styles from the theme factory**, splitting static (color-free) styles from theme-dependent ones so the dynamic part is recomputed only when the theme changes.
- **Tokens, never literals** - spacing, radius, icon sizes, control dimensions, animation durations, z-index, and elevation all come from named scales.
- **Emphasis via the type scale**, never an inline font-weight in a component style.
- **Memoize pure presentational components** paired with stable handler props - memoization does nothing while a parent passes fresh inline closures.
- **Extract list/menu data to a sibling config**, not inline constants in the rendering file; render from one map.
- **A shared section/header primitive** owns repeated headers, so the style has one place to change.

## Layout and safe areas

- **Pad for device safe-area insets** where the platform has them (notches, system bars). Never hardcode status-bar / nav-bar heights.
- **Flex-fit by default; scroll where content can overflow.** Wrap content that can exceed the viewport in a scroll container - a fixed container clips overflow and the off-screen content becomes unreachable. Use a virtualized list for data lists, never a plain scroll container. Absolutely-positioned overlays sit outside the scroll area, with scroll content padded below them so the last row clears.

> **Gotcha:** verify the **smallest supported device/viewport** first - it overflows where larger ones fit, and a missing scroll container only shows up there.

## State management

- **Local UI state** for modals, inputs, toggles, component-scoped state.
- **Optimistic UI** via the framework's built-in mechanism. Do not reach for a client store to solve optimistic updates.
- **No global client state store** without explicit approval. If you feel you need one, the component boundary is probably wrong - discuss first.
- Server data is **not** UI state - it lives in the data layer (below).

## Data layer

- **A dedicated server-state library** is the data layer (caching, refetch, retry). Type responses with the SPA's **typed API client**, which mirrors the service's documented API (generated from protobuf once that is adopted).
- **A single typed client** is the one crossing point to the service: it attaches the Keycloak token, unwraps the response envelope, and surfaces a typed error. Base URL + any mock toggle come from config, never hardcoded.
- **Mock-first:** the UI iterates against a contract-satisfying mock; the mock -> live cutover must not touch the UI. The contract seam is owned by [integration_protocol.md](integration_protocol.md).

## Auth

- The auth provider's client SDK attaches the token to every request; the server verifies it. **Never trust a client-sent user id** - identity comes from the verified token.

## Accessibility

- Every interactive element has a visible focus/pressed state. Icons without a visible label need an accessibility label.
- **Color is never the sole indicator of state** - always pair with text or an icon.
- WCAG AA is a hard constraint (4.5:1 normal text, 3:1 large text / UI components). Full a11y rules: [style_guide.md](../design/style_guide.md).

## Testing

- Use the framework's component test runner + a render/interaction testing library. Mock at the **data-layer seam** (the query hook or a request mock), never a real datastore or auth client - the UI talks to the service, not the database.
- Type every fixture from the API client so it matches the documented API and cannot drift.
- Component tests are required for any component with non-trivial conditional rendering or state. Pure helpers get unit tests. Reset mocks between tests.

```tsx
import { render, screen } from "<your-testing-library>"
import { Header } from "../Header"

// Stub the data hook with a fixture typed from the API client.
jest.mock("../useData", () => ({
  useData: () => ({ data: [], isLoading: false }),
}))

describe("Header", () => {
  it("renders the columns", () => {
    render(<Header />)
    expect(screen.getByText("First")).toBeTruthy()
  })
})
```

> **What not to do:** do not mock the datastore or a real auth client in a UI test, and do not hit a live API. The only seam is the data hook / request mock against the documented API.

## Platform and release constraints

- **Version your API and keep changes backward-compatible** where clients cannot be force-updated (native apps, long-lived sessions) - old versions keep calling the API for a long time.
- **Document platform-specific behavior on discovery,** in the same change, so it is not rediscovered: UI/rendering quirks here; build/config/credential requirements in the project's platform-requirements doc.
- **Never copy real production data into dev** - it is PII. Use synthetic seed data at production scale.

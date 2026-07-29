# Core Protocol

---

## Overview

This is the **shared core**: development standards that apply across the whole codebase - the TDD loop, the branching model, folder structure, naming, code quality, env vars, commits, the PR checklist, and docstrings.

Role-specific rules, examples, and gotchas live in their own protocols:

| Protocol                                           | Owns                                             |
|----------------------------------------------------|--------------------------------------------------|
| [frontend_protocol.md](frontend_protocol.md)       | The UI / client surface                          |
| [backend_protocol.md](backend_protocol.md)         | The service + data layer                         |
| [integration_protocol.md](integration_protocol.md) | The API seam between services and consumers      |
| [engine_protocol.md](engine_protocol.md)           | Engine code - the layer the game is built on     |
| [gameplay_protocol.md](gameplay_protocol.md)       | Gameplay code - the rules of the game            |
| [game_test_protocol.md](game_test_protocol.md)     | Test-first as it applies to game code            |

Session lifecycle and work-item selection live in [session_protocol.md](session_protocol.md).

---

## Test-First Development (TDD)

**All new behavior is written test-first.** This is a hard requirement (see the [Enforcement Rules](session_protocol.md#enforcement-rules)).

### The loop

1. **Red** - write the test(s) that capture the expected behavior and run them; confirm they fail for the right reason. Show the failing run before writing implementation.
2. **Green** - write the minimum code to make them pass.
3. **Refactor** - clean up with the tests staying green.

### What "the test first" means per layer

| Layer                      | The test written first                                                                                                                                                                                                                                 |
|----------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Service endpoint           | A contract/integration test (request against the running app, real datastore via Testcontainers) asserting the documented API shape, envelope, and error codes.                                                                                        |
| API contract               | A provider contract/integration test pinning the documented response shape; cross-repo, a consumer-driven contract (e.g. Pact).                                                                                                                        |
| UI component / hook        | A render/behavior test, with the data layer mocked at the query seam against the documented API.                                                                                                                                                       |
| Integration (service + UI) | **Both sides, test-first**: the service contract/integration test (real datastore) *and* the consumer test against a mock matching the documented API. The HTTP API is the integration seam, so a drift on either side fails a test.                   |

### Documented exception

**Schema / index / mapping work** cannot be queried until the collection or index exists, so a pure red-first test is impossible. There, write the **spec-driven integration tests in the same change** (assert the model's invariants against a live datastore via Testcontainers) and drive to green. State explicitly whenever this exception applies. No other exceptions without a stated reason.

---

## Branching Model (`dev` integration)

The app is built on a long-lived **`dev`** integration branch; **`main`** stays the stable trunk. Any manual-QA notes live in [qa_protocol.md](qa_protocol.md). (Enforcement: [session_protocol.md](session_protocol.md#enforcement-rules) Rule 11.)

- **`main`** - stable trunk: docs, protocol, foundations (schema, API contract, auth). Kept clean.
- **`dev`** - app integration: data layer, screens, auth, manual testing. **All app work goes here.**
- **Sync `main -> dev`** regularly (`git checkout dev && git merge main`) so doc/foundation updates flow in.
- **Feature or bugfix branch off `dev`.** The agent works on a `feature/ADE-<number>` branch (any non-bug work) or a `bugfix/ADE-<number>` branch (a fix), where `ADE-<number>` is the work item's Linear issue id, e.g. `feature/ADE-270`.
- **Order of operations (critical):**
  1. The agent gets the branch green: lint + typecheck + tests (test-first), then pushes the branch (CI runs).
  2. **A reviewer / QA exercises the change** as required by [qa_protocol.md](qa_protocol.md). The agent does **not** open the PR or merge yet.
  3. **Review/QA passes** -> the agent opens the PR into `dev` (with results in the PR body).
  4. **PR CI green** -> the agent merges into `dev`. CI red -> the agent fixes and repeats.
- **`dev -> main` is maintainer-only.** A maintainer performs the promotion later, once accumulated `dev` work is stable. **The agent never merges to `main`** and never commits app work straight to `main`.
- A feature replacing an old one removes the old code on `dev` in the same change (Rule 10).

---

## Common Conventions (all surfaces)

### Repo & Folder Structure

**One repo = one game binary.** This repository builds a single desktop executable. There is no service, no container, and no HTTP seam. Full architecture: [runtime_architecture.md](../design/runtime_architecture.md).

```
doomer/
├── src/
│   ├── platform/                 ← OS seam: window, GPU, audio device, filesystem, clock
│   ├── engine/                   ← math, collision, render, input, resources, events
│   ├── game/                     ← entities, systems, combat, loot, levels, save
│   └── main.cpp                  ← bootstrap
├── tests/
│   ├── engine/                   ← unit tests, no hardware
│   ├── game/                     ← simulation tests, headless
│   └── golden/                   ← golden-image references
├── data/                         ← JSON tunables, content definitions, levels
├── assets/                       ← committed art (placeholder, generated)
├── resources/                    ← local Doom data, gitignored, never committed
└── CMakeLists.txt                ← CMake + FetchContent
```

(The repo also carries the vendored `docs/`, `.claude/`, `.windsurf/`.)

Conventions that hold throughout:

- **The dependency direction is one way:** `game -> engine -> platform`. Because the tiers are directories, a violating `#include` is visible in the path during review ([engine_protocol.md](engine_protocol.md)).
- **Only `platform/` touches the OS,** a driver, or a device. Everything above it talks to an interface, which is what keeps the test suite runnable without hardware.
- **Engine code contains no game nouns.** `Player`, `Enemy`, and `Inventory` never appear in a renderer, an allocator, or an input backend.
- **Co-location.** A helper used by one system lives with that system; promote it when a second system needs it.

### Naming Conventions

Adjust casing to your language's idiom; keep them consistent across the repo.

| Item                  | Convention              | Example                   |
|-----------------------|-------------------------|---------------------------|
| Components            | PascalCase              | `UserCard.tsx`            |
| Hooks                 | camelCase, `use` prefix | `useUserStatus.ts`        |
| Utilities / helpers   | camelCase               | `formatDate.ts`           |
| Routes / endpoints    | kebab-case              | `api/v1/reset-password`   |
| Feature folders       | kebab-case              | `features/onboarding/`    |
| Test files            | `*.test.*`              | `formatDate.test.ts`      |
| DB table types        | PascalCase singular     | `User`, `Order`           |
| DB columns            | snake_case              | `created_at`, `is_active` |
| Environment variables | SCREAMING_SNAKE_CASE    | `DATABASE_URL`            |
| Constants             | SCREAMING_SNAKE_CASE    | `MAX_RETRIES`             |

### Code Quality

Phrase these against your language; the principles are universal.

- **No untyped escapes.** Prefer the strictest typing your language offers; avoid unchecked casts. Where a cast is unavoidable, add an inline comment explaining why it is safe.
- **Boundary types are owned per deployable** - a consumer mirrors a provider's documented API behind its own typed client; do not inline ad-hoc request/response types in handlers or components ([integration_protocol.md](integration_protocol.md)).
- **Prefer explicit return types** on exported functions.
- **Strict mode is non-negotiable** - do not loosen the compiler/linter config to make code pass.
- **One inherited source of truth for compiler/linter config.** Per-project configs extend a shared base and keep only runtime-specific overrides; add a shared flag to the base, not to one project.
- **Guard indexed access.** Treat array/record lookups as possibly-absent and guard them before use rather than asserting non-null.

### Environment Variables

- Every new environment variable must be documented (name + one-line description) before the PR is opened, in whatever the project uses to track its configuration.
- No hardcoded secrets anywhere in the codebase - no exceptions.
- Variable names follow SCREAMING_SNAKE_CASE.
- Local secret/config files stay out of version control.

### Commit Messages - Conventional Commits

Format: `<type>(<scope>): <short description>`

| Type       | When to use                              |
|------------|------------------------------------------|
| `feat`     | New feature or behaviour                 |
| `fix`      | Bug fix                                  |
| `chore`    | Tooling, config, dependency updates      |
| `docs`     | Documentation only                       |
| `refactor` | Code change with no behaviour change     |
| `test`     | Adding or updating tests                 |
| `style`    | Formatting, whitespace - no logic change |

Scope is optional but recommended when the change is isolated (e.g., `feat(auth): add token refresh`). Breaking changes: append `!` after the type/scope and include a `BREAKING CHANGE:` footer. End every commit message with the project's configured attribution trailer.

### PR Checklist

Before opening a PR, all of the following must be completed.

- [ ] **Self-audit:** re-read the role protocol(s) for the work you did (frontend / backend / integration) and audit the diff against them - confirm the change conforms before opening the PR.
- [ ] A work item exists and is referenced in the PR body.
- [ ] **Tests were written first** (failing test before implementation) per [Test-First Development](#test-first-development-tdd) - or the documented schema exception is stated.
- [ ] Lint passes with zero errors.
- [ ] Tests pass with zero failures.
- [ ] No untyped escapes introduced; no unexplained casts.
- [ ] All new env vars documented (name + description).
- [ ] Commit messages follow conventional commits format.
- [ ] If the data model changed: indexes/mappings are defined and applied idempotently, with the spec test in the same change.
- [ ] At least one test covers the happy path for any new feature or endpoint.
- [ ] **Coverage is whole-tree:** a new source file ships with a test, or a justified exclusion is added to the coverage config.

#### CI gates - all blocking

CI may run more than lint/test/typecheck, and **every gate blocks the merge**. A PR is not green until all pass; most have a local equivalent, so run it before pushing rather than discovering it in CI. Typical gates: typecheck (strict), coverage floors, dead-code scan, docstring presence, and any secret / supply-chain / container scans. Document your project's exact gate set here.

---

## Code Commenting and Docstrings

- All exported functions and classes should have detailed docstrings with appropriate annotations.
- When writing single-line and multi-line comments, do **not** reference work-item ids or link them in source code files.

**Example docstring (TypeScript / TSDoc shown; use your language's idiom):**
```ts
/**
 * Retrieves the current {@link GuestContextValue} from the surrounding {@link GuestProvider}.
 *
 * @remarks
 * This hook must be called within a component wrapped by a {@link GuestProvider}.
 * If invoked outside of the provider, an error is thrown to prevent undefined context usage.
 *
 * @returns The active guest context value supplied by the nearest {@link GuestProvider}.
 * @throws Error Thrown when the hook is used outside of a {@link GuestProvider}.
 * @see {@link GuestProvider}
 */
export function useGuest(): GuestContextValue {
  const ctx = useContext(GuestContext);
  if (!ctx) {
    throw new Error("useGuest must be used within a GuestProvider");
  }
  return ctx;
}
```

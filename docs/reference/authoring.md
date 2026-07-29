# Authoring: Linear Issues & Pull Request Summaries

How we write the two artifacts that bracket a tracked session: the **Linear issue** that defines the work (the `ADE-<number>` issue), and the **pull request (PR) summary** that describes the change. The [`/new-work-item`](../../.claude/skills/new-work-item/SKILL.md) skill uses the story template; the [`/session-end`](../../.claude/skills/session-end/SKILL.md) skill uses the PR template.

---

## Linear issue template

A good story is small, outcome-focused, and testable. Title -> statement -> context -> acceptance criteria -> scope -> done.

**Summary (title):** a concise outcome, prefixed by type so the branch is derivable - `Feature: <outcome>` or `Bug: <symptom>` (drives `feature/ADE-<n>` or `bugfix/ADE-<n>`).

**Story statement (feature):**
> As a `<role>`, I want `<capability>`, so that `<benefit>`.

**For a bug**, replace the statement with:
- **Observed:** what happens.
- **Expected:** what should happen.
- **Steps to reproduce:** numbered, minimal.
- **Environment:** build (commit or version) + platform; GPU and driver if relevant.

**Context / description:** the *why*, with links to designs, specs, or related issues. Keep it short - link rather than inline.

**Acceptance criteria:** the observable behaviors that make this done - a checklist, one box per behavior. These are the contract for the work and seed the PR's QA checklist ([qa_protocol.md](../protocol/qa_protocol.md)).
- [ ] `<observable behavior>`
- [ ] `<edge / error case>`

For a complex flow, write them as Gherkin instead:
> **Given** `<precondition>` **When** `<action>` **Then** `<outcome>`.

**Out of scope (non-goals):** what this story explicitly does not cover - prevents scope creep.

**Dependencies:** blocked-by / blocks `ADE-<n>`; any cross-repo coordination (a provider API change a consumer needs - see [engine_api_protocol.md](../protocol/engine_api_protocol.md)).

**Definition of Done:** acceptance criteria met; tests written test-first and green; any API change backward-compatible (or a new version); docs / locked decisions updated; merged to `dev`. The full gate is the [PR Checklist](../protocol/core_protocol.md#pr-checklist).

**Sizing & priority:** set the Linear **priority** (`Urgent` / `High` / `Medium` / `Low` / `No priority`) in `/new-work-item`; add an **estimate** if the team sizes work. Split anything that cannot be demoed in one sitting.

---

## Pull request (PR) summary template

The PR description is the durable record of *what changed and why*. Title -> summary -> issue link -> changes -> testing -> risk.

**Title:** conventional-commit shape + the Linear issue id - `<type>(<scope>): <description> [ADE-<n>]` (types in [core_protocol.md](../protocol/core_protocol.md#commit-messages---conventional-commits)).

**Body:**

```
## Summary
<1-3 sentences: what this change does and why.>

## Linear
Closes ADE-<n>

## Changes
- <notable change>
- <notable change>

## Testing
- <how it was verified: tests added, manual QA per qa_protocol>
- QA checklist: <inline checkboxes for app-facing changes, or "n/a">

## Risk & rollout
- Engine API change? (additive, or deprecated with callers migrated - engine_api_protocol) yes / no
- Data: content or save schema change / migration needed? yes / no
- Determinism: does this touch the tick, the RNG, or iteration order? yes / no
- Anything to watch after merge (frame budget, new tunables, asset dependencies).
```

UI change? Add screenshots or a short clip. End the body with the project's configured attribution trailer ([CLAUDE.md](../../CLAUDE.md#commit-messages-and-pull-requests)) - no other auto-generated footer.

**Rules:**
- **One PR per Linear issue** where practical; the title carries the identifier so Linear links the PR to the issue.
- Keep the **Summary** reviewer-facing (the *why*); the diff shows the *what*.
- The **Risk & rollout** section is load-bearing for this stack - services deploy independently to one cluster, so call out any backward-incompatible API or data change explicitly.
- Never an em dash; use a spaced hyphen, a comma, or parentheses.

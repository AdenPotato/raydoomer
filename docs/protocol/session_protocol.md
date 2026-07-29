# Session Protocol

---

## Session Workflow

### Session Types

**Tracked Session:**

- Work-Item Requirement
   - A Linear issue (in the `ADE` team) must exist before any branch is created - no exceptions.
   - Create one first if it does not already exist (`/new-work-item`).
- Work-Item Confirmation Requirement
   - Ask the developer which work item the session is against before proceeding.
- Branch Confirmation Requirement
   - Prompt the developer for the branch to open the session against.
- Branch Naming Requirement
   - Create a new branch off `dev` (before any changes) following the convention `feature/ADE-<number>` (any non-bug work) or `bugfix/ADE-<number>` (a fix), e.g. `feature/ADE-270`. `ADE-<number>` is the work item's Linear issue id; the `feature/` or `bugfix/` prefix marks the kind of work.
   - App work branches off `dev` (Branch & Merge Workflow); see the Branching Model in [core_protocol.md](core_protocol.md#branching-model-dev-integration).
- Changelog Requirement
   - At session end, prepend an entry to the **top** of `docs/changelog.md` in the rolling format: a **date line** `YYYY-MM-DD HH:MM TZ` (the author's local time + their timezone abbreviation, no session numbers - branch-merge-safe), then the **person** on the next line, then a `## Title` heading, then a **two-sentence body opening with the bracketed category tag** (`[feature]` | `[enhancement]` | `[bug]` | `[internal]`), then a `Tickets:` footer. The format rules live in the `## Changelog format` section at the bottom of `changelog.md`; the full template is in the `session-end` skill/workflow. Keep **max 20 entries**; drop the oldest.
- PR Confirmation Requirement
   - Commit all changes, then prompt the developer for the branch to open the PR against.

**Sandbox Session:**

- Work-Item Requirement
   - No tracked work item is required.
- Branch Requirement
   - No specific branch, no PR.
- Changelog Requirement
   - Exploration only - optional one-line changelog note at the developer's request.
- Session Promotion Requirement
   - Promote to Tracked at any point: the developer says "promote to Tracked" -> the agent creates the branch, continues as Tracked (`/promote`).

**Design Session:**

When initiating a design session (e.g., data model, verification model, API structure), adhere to the following guidelines.

1. Questionnaire Requirement
   - When creating a new design markdown file (`docs/design/*`), the agent must conduct a structured series of yes/no questions to resolve all relevant design decisions and edge cases.
   - No design document may be produced until the full questionnaire is completed and all answers are confirmed by the developer.
   - All questions must be asked one-by-one, not as a giant list.
   - When a question is asked, the agent should also indicate the number of questions remaining.
2. Sequential Answer Requirement
   - The developer must answer each question sequentially.
3. Clarification Requirement
   - The agent identifies unresolved branches or contradictions.
4. Question Completion Requirement
   - Only after all questions are answered does the agent generate the corresponding design markdown file in `docs/design/*`.
5. Content Verification Requirement
   - The resulting generated file must reflect all decisions made during the questionnaire.

---

## Session Start Checklist

1. Sync (Guarded)
   - Before reading context or creating any branch, fast-forward the integration branch, `fetch --prune`, and delete local branches whose work has merged.
   - Skip when resuming an in-progress feature branch or the working tree is dirty.
2. Session Type
   - The developer declares the session type (or the agent asks).
3. Select the Work
   - Surface the open, ready Linear issues from the `ADE` team, ordered by priority.
   - The developer picks the item(s) before any branch is created or work begins.
4. Read Relevant Markdown
   - The agent determines and reads the relevant markdown files under `docs/design/` related to the selected work (e.g. an entity index, the style guide).
5. Markdown File Confirmation
   - Confirm with the developer which markdown files are determined to be relevant prior to proceeding.

---

## Session End Checklist

1. Summary Requirement
   - Details what was decided, what was built, which work items were opened, which were completed.
2. File List Requirement
   - List each file changed with a one-line description.
3. Tracked Sessions
   - Insert the changelog summary for the session -> commit -> open the pull request.
4. Sandbox Session
   - Brief summary only; no changelog required unless the developer requests one.
5. Changelog Cleanup
   - If [changelog.md](../changelog.md) has exceeded 20 entries, remove the oldest, keeping 20 active.
6. Unresolved Item Requirement
   - Any item not fully resolved this session must become a tracked work item before session close.
   - No open-ended prose left in any doc or changelog entry - reference the work-item id only.
7. Next-Up Requirement
   - Close every tracked session with an explicit "Up Next" list of Linear issue identifiers in priority order, pulled from Linear.
8. Confirmation Requirement
   - All required document updates have been applied.
   - Any decision affecting guiding documents must be reflected before session close.

---

## Working with Agents + Skills

The role **agents** (`frontend`, `backend`, `integration`) and the scaffolding **skills** (`/new-screen`, `/new-endpoint`, `/new-contract`, `/db-change`) are how the work inside a Tracked session gets executed. This is the operating manual for that loop.

### What they are (and are not)

- **Agents are in-session subagents, not separate terminals.** You run one session in the repo. When it delegates to a role agent, that agent runs as a scoped child with its own context and reports back into the same session. There is no terminal-per-role, no agent that pulls its own ticket, and no agent that opens its own PR.
- **One agent owns one surface.** `frontend` = the UI / client surface; `backend` = the service + data layer; `integration` = the API seam (it delegates the deep service and UI work to the other two and owns only that both sides agree). Each agent defers to its protocol (`*_protocol.md`) rather than restating it.
- **Skills scaffold the test-first change.** A skill is the ordered checklist for adding a unit of work the right way: failing test first, tokens or contract honored, green. `/new-contract` runs before `/new-endpoint` or `/new-screen` whenever the change needs a new shared shape.

### The loop (one work item, pick to merge)

1. **Pick the work item** at session start and branch `feature/ADE-<number>` or `bugfix/ADE-<number>` off `dev`.
2. **Assign the matching agent** for the surface and hand it a *grounded* spec: real file paths, the tokens or contract to honor, and the exact test to write. Read enough of the codebase first that the spec names real files and conventions, not guesses.
3. **Agent runs the skill, test-first:** failing test shown, then implementation, then green (lint + typecheck + tests). The agent self-audits the diff against its protocol before declaring done.
4. **Orchestrator verifies independently** - re-run the tests; for a change that is *observable*, exercise it. Never merge on the agent's word alone.
5. **Push the branch** (CI runs); request review / QA per [qa_protocol.md](qa_protocol.md).
6. **Open the PR into `dev`**; CI green, merge to `dev`. `dev -> main` stays a separate, maintainer-only promotion.

### Gotchas (general failure modes)

1. **A lint/scan rule a protocol names may be CI-only.** Some gates run only in CI, not in the local lint command. An agent told to "run lint and confirm X" may not be able to execute that specific gate locally; verify by inspection and let CI run the gate.
2. **Read the test count, not just the suite count.** Some runners occasionally fail a *suite* at worker startup while every test passes. Re-run before concluding your change broke the suite.
3. **An explicit task instruction outranks a generic hook nudge.** If a hook fires mid-run and contradicts the task, follow the task. Precedence: explicit task instruction > generic hook prompt.
4. **A leaf primitive has no observable surface until something mounts it.** A component used nowhere yet is exercised first by its consumer. Do not add throwaway mounting code just to force a check - that violates the no-dead-code rule.

---

## File Inventory

Adapt the source-tree rows to your repo layout. The doc rows below are the framework's own files.

| File                                  | Purpose                                                                                              | Update rule                                                    |
|---------------------------------------|------------------------------------------------------------------------------------------------------|----------------------------------------------------------------|
| .claude/skills/<name>/SKILL.md        | Project-level skills, one folder each. Invoked with `/skill-name`.                                   | Add/update when session skills change. Version-controlled.     |
| .claude/agents/                       | Role subagents (`frontend`, `backend`, `integration`).                                               | Update when a role's surface or rules change.                  |
| CLAUDE.md                             | Auto-loaded every session. Behavior rules.                                                           | Surgical edits only. Never rewrite in full.                    |
| .windsurf/rules/                      | Editor rules: always-on core + per-surface role rules (model_decision).                              | Update when a behavior rule or role surface changes.           |
| .windsurf/workflows/                  | Slash-invoked session ceremonies (session-start, session-end, promote).                              | Update when a session-ceremony workflow changes.               |
| .windsurf/skills/                     | Scaffold skills (new-*, db-change); one SKILL.md per folder.                                         | Update when a scaffold skill changes.                          |
| README.md                             | The project brand; what this project is.                                                             | Update when project scope changes.                             |
| docs/README.md                        | Index/map of the docs tree (folders + per-file); navigation entry point.                             | Update when a doc or folder is added, moved, or removed.       |
| docs/protocol/session_protocol.md     | This file. Session rules and conventions.                                                            | Update when the workflow changes.                              |
| docs/protocol/core_protocol.md        | Shared dev core: TDD loop, branching, folder structure, naming, code quality, commits, docstrings.   | Update when a cross-cutting convention changes.                |
| docs/protocol/frontend_protocol.md    | UI / client surface rules: structure, theming, testing.                                              | Update when a frontend convention or token rule changes.       |
| docs/protocol/backend_protocol.md     | Service + data-layer rules: routes/handlers, data access, backend testing.                           | Update when a service or data-layer convention changes.        |
| docs/protocol/integration_protocol.md | The API seam: both-sides-test-first, typed client, mock->live cutover.                               | Update when the contract, client, or cutover approach changes. |
| docs/protocol/engine_protocol.md      | Engine code: game/engine boundary, resource lifetime, platform seam, determinism, frame budget.      | Update when an engine-layer convention changes.                |
| docs/protocol/gameplay_protocol.md    | Gameplay code: tunables in data, feel requests, behavior vs balance, frame-rate independence.        | Update when a gameplay convention changes.                     |
| docs/protocol/game_test_protocol.md   | Test-first for game code: per-layer tests, the four seams, golden images, the feel exception.        | Update when the game testing standard changes.                 |
| docs/protocol/qa_protocol.md          | Manual QA workflow, per-PR checklist convention, environment matrix.                                 | Update when the QA workflow or matrix changes.                 |
| docs/reference/locked_decisions.md    | Current decisions, locked items, open questions, project status.                                     | Update every Tracked session end.                              |
| docs/reference/glossary.md            | Shared vocabulary. Definitions only.                                                                 | Add when a new term is introduced or renamed.                  |
| docs/reference/integrations.md        | External-service runbook - setup steps and env vars per service.                                     | Update when a new external service is added or changed.        |
| docs/reference/authoring.md           | Linear issue + PR summary templates.                                                                 | Update when the story or PR format changes.                    |
| docs/changelog.md                     | Recent session log. 20-entry max.                                                                    | Insert at top via str_replace. Never rewrite.                  |
| docs/design/runtime_architecture.md| Core runtime architecture: loop, layers, entities, events, save, levels.                                           | Update when a runtime architecture decision changes.|
| docs/design/style_guide.md            | Visual language - colors, type, spacing, components, accessibility.                                  | Update when a token or UX rule changes.                        |
| docs/design/                          | Product and technical design specs.                                                                  | Create/update when a design decision changes.                  |

---

## Design Spec Lifecycle

A generic flow for keeping design docs and the shared vocabulary in sync when a domain concept (entity, feature, model) is added or removed.

### Adding a concept

1. **Verify a completed design doc exists** in `docs/design/` for it. If not, stop - schedule a Design session (`/new-design`) before proceeding.
2. **Read the design doc** before creating any spec file.
3. **Create the spec file** - definition + rules + links to related concepts. Derived from the design doc.
4. **Add it to any index** - one row in the overview table; add relationship lines if it has relations.
5. **Add references** to any related spec files.
6. **Add the term to `glossary.md`** - one-line definition + link to the new file.

### Removing a concept

1. **Grep `docs/` for all references** before touching any file. Full hit list first - no deletions until reviewed.
2. **Categorize hits** - real reference (must change) vs. incidental word match (leave alone).
3. **Confirm the list with the developer** before executing any changes.
4. **Delete the spec file**.
5. **Execute surgical edits in order:** index -> related spec files -> `glossary.md` -> `locked_decisions.md` -> any affected `docs/design/` files.
6. **Re-run grep** to confirm zero orphaned references remain before committing.

---

## Enforcement Rules

1. Locked Decision Requirement
   - If work contradicts a locked decision, the agent must flag it and halt until resolved.
2. Conflict Resolution Requirement
   - If a requirement is vague or contradicts another requirement, the agent must flag it before proceeding.
3. Pushback Requirement
   - Agreement is not a valid response to a decision conflict - pushback is required.
4. Unlock Process Requirement
   - The developer states the reason -> the agent flags downstream impact -> change made, documented, re-locked.
5. No Speculative Code
   - If the guiding documents do not dictate which library or implementation approach to use, the agent must ask the developer before proceeding.
6. Markdown File Update Requirement
   - Any decision made in a session that affects the project's guiding documents (locked_decisions.md, the protocols, glossary.md, or design/spec files) must be updated during the same session.
   - The agent must notify the developer when a decision requires a document change, and the session cannot close until the update is made.
   - Do not reference session numbers as justification when updating markdown files (except for changelog.md).
   - Do not reference work-item ids when updating markdown files (except for changelog.md).
7. Pull Request Creation Requirement
   - Confirm with the developer that all desired work items have been completed for the session before creating the pull request.
   - Do not create a pull request until confirmation has been provided.
8. Document Table Requirement
   - When updating a table in a markdown file, preserve the existing column formatting and spacing for readability without a markdown renderer.
9. Test-First Requirement
   - New behavior is written **test-first**: a failing test capturing the expected behavior must exist (and be shown failing) before the implementation is written. Applies to service endpoints, API contract logic, and UI components/hooks.
   - Schema/index/mapping work uses spec-driven integration tests written in the same change (the documented exception - you cannot query a collection or index until it exists). Any other exception requires a stated reason.
   - Full standard in [core_protocol.md](core_protocol.md#test-first-development-tdd).
10. No Dead Code on Replacement
    - When a feature, flow, screen, or approach **replaces** an existing one, the old code must be **removed in the same change**, safely (delete the files/routes, drop now-unused imports, deps, tests, and config). No commented-out blocks, no orphaned files, no superseded flow left behind "just in case".
    - "Safely" means: confirm nothing else references it (grep first), then remove; the change must build, lint, typecheck, and test green after removal.
11. Branch Promotion Authority (`main` is protected)
    - **The agent never merges to `main`** and never commits app work directly to `main`. All work lands on the long-lived **`dev`** integration branch.
    - `dev -> main` is a **separate, deliberate, maintainer-only promotion**, once accumulated `dev` work is stable.
    - **Follow the full flow without deviating** - it is defined once in the [Branching Model](core_protocol.md#branching-model-dev-integration).

# Claude Instructions

> **Template note:** this is a reusable, project-neutral operating framework. Replace `<project-name>` with your project name and the `<co-author-trailer>` placeholder with your preferred attribution line (or delete it). Everything else is written to work for any stack and any project - adapt the placeholders in the protocols to your toolchain as you go.

---

## Project

`<project-name>`. This file is auto-loaded every session and defines how Claude works in this repo. The work for a session is whatever the developer brings to it - a task, a ticket, an idea - tracked in whatever tool the team uses.

---

## Populate Initial Context

Claude reads the following files at the start of each new session.

- [changelog.md](docs/changelog.md)
- [session_protocol.md](docs/protocol/session_protocol.md)
- [core_protocol.md](docs/protocol/core_protocol.md)
- [locked_decisions.md](docs/reference/locked_decisions.md)
- [glossary.md](docs/reference/glossary.md)

---

## Skills

Project-level skills live in `.claude/skills/<name>/SKILL.md` (one folder per skill - a flat `.md` file will not be discovered). Invoke with `/skill-name`. Each skill's `description:` frontmatter is the canonical one-liner; this table is the index.

| Skill            | Purpose                                                                                 |
|------------------|-----------------------------------------------------------------------------------------|
| `/session-start` | Full session start checklist - sync, context load, select the work, confirm docs        |
| `/session-end`   | Full session end checklist - summary, changelog, commit, PR, next-up list               |
| `/pr-summary`    | Generate a PR description from the branch diff                                          |
| `/new-work-item` | Capture a new Linear issue (ADE team) with type and priority in one flow                |
| `/write-story`   | Draft or refine a Linear issue to the story template                                    |
| `/new-design`    | Structured questionnaire before generating any `docs/design/*` file                     |
| `/promote`       | Promote a sandbox session to tracked - create the branch, continue as tracked           |
| `/engine-api-change` | Change the engine/game boundary, test-first on both sides                           |
| `/new-system` | Scaffold a gameplay or engine system + failing simulation test first                       |
| `/new-hud` | Scaffold a HUD element + failing test first                                                   |
| `/data-change` | Content or save schema change + round-trip and migration tests                            |

## Agents

Role subagents live in `.claude/agents/`; delegate to them via the Agent tool. Each owns a surface and defers its rules to the matching protocol.

| Agent         | Use for                                                                                                                                                |
|---------------|--------------------------------------------------------------------------------------------------------------------------------------------------------|
| `presentation` | Render, camera, sprites, HUD, audio out ([presentation_protocol.md](docs/protocol/presentation_protocol.md))                                          |
| `simulation` | Engine subsystems + gameplay rules ([engine_protocol.md](docs/protocol/engine_protocol.md), [gameplay_protocol.md](docs/protocol/gameplay_protocol.md)) |
| `content` | Definitions, loaders, resources, saves ([content_protocol.md](docs/protocol/content_protocol.md))                                                          |

---

## Session Protocol

**Session Type - Declare First (or Claude will ask):**

- `Session type: Tracked, Design, or Sandbox`

Detailed rules for each session type are defined in [session_protocol.md](docs/protocol/session_protocol.md#session-types).

**Branch Naming - Environment Conflict Rule:**

If the session environment pre-assigns a branch name that does not follow the `feature/ADE-<number>` or `bugfix/ADE-<number>` convention defined in session_protocol.md, Claude must halt and flag the conflict before making any changes.

Do not proceed on the pre-assigned branch without explicit developer approval.

**Session Start:**

The session start checklist is defined in [session_protocol.md](docs/protocol/session_protocol.md#session-start-checklist).

**Session End:**

The session end checklist is defined in [session_protocol.md](docs/protocol/session_protocol.md#session-end-checklist).

---

## Project Documentation Map

Structure is documented in two places, by concern (these are two different inventories, not one thing split):

- **Docs inventory** - which markdown file does what + its update rule: [session_protocol.md - File Inventory](docs/protocol/session_protocol.md#file-inventory).
- **Code layout** - the source tree: [core_protocol.md - Folder Structure](docs/protocol/core_protocol.md#folder-structure).

A brief map of the `docs/` tree itself is in [docs/README.md](docs/README.md).

---

## Response Style

Answer the exact question asked, then stop. Default to the **shortest correct answer**.

- Asked for a command -> give the command (plus at most one line of context). No walls of text.
- No preamble, no restating the question, no summary of what you just did unless asked.
- Do **not** volunteer alternatives, caveats, tips, or "you could also..." unless asked or genuinely critical.
- Expand only when the developer asks for detail, or the task is genuinely complex (a real trade-off, a risk, a decision needing input).
- Prefer one short paragraph or a few bullets over sections and headers.

---

## Accuracy Bar (CRITICAL)

Be right, and be honest about uncertainty.

- **Verify against source before asserting.** Read the file, run the query, check the API - do not answer a factual question about this codebase, its config, or an external service from memory.
- **Cite the evidence.** Point to the file path + line/anchor, the command output, or the doc you are relying on, so a claim is checkable.
- **Flag uncertainty instead of confabulating.** "I am not sure - let me check" beats a confident wrong answer. If you cannot verify, say so.
- **When corrected, correct the record** (the doc, the memory, the protocol), not just the reply.

---

## Decisions and Continuity Enforcement

Decisions and continuity enforcement guidelines are defined in [session_protocol.md](docs/protocol/session_protocol.md#enforcement-rules).

---

## Commit Messages and Pull Requests

Configure an attribution trailer for the project and apply it consistently. Set `<co-author-trailer>` to your preferred line (e.g. `Co-authored with Claude`), or remove this section if you do not want one.

- Every commit message ends with the `<co-author-trailer>`.
- Every PR body ends with the `<co-author-trailer>`.
- Do not append any other auto-generated footer unless the developer asks for one.

---

## Writing Style

- **Never use the em dash (the `—` character) anywhere** - copy, code, comments, commit messages, PR bodies, docs, changelog. Use a spaced hyphen ` - `, a comma, a colon, or parentheses instead. This is a hard project style rule; apply it while writing, not as a cleanup pass.

---

## Branch & Merge Workflow (CRITICAL - do not deviate)

**`main` is protected. Claude never merges to `main`** and never commits app work straight to it. All work branches off the long-lived **`dev`** integration branch as `feature/ADE-<number>` or `bugfix/ADE-<number>` (the Linear issue id); `dev -> main` is a separate, deliberate promotion performed by a maintainer.

The full sequence (feature branch -> green + push -> review/QA -> PR -> merge to `dev`) is defined once in the [Branching Model](docs/protocol/core_protocol.md#branching-model-dev-integration); the review/QA gate and any manual-QA workflow live in [qa_protocol.md](docs/protocol/qa_protocol.md). Do not deviate.

---

## Git Safety (CRITICAL - never rewrite shared history)

Claude only ever **fast-forward pushes** (`git push`). Claude must **never** rewrite or destroy shared history without explicit, in-the-moment maintainer approval. Forbidden without that approval:

- **Force push** in any form: `git push --force`, `git push -f`, `git push --force-with-lease`.
- **History rewrite of a pushed branch:** `git rebase` of shared history, `git commit --amend` after a push, `git reset --hard` on a pushed branch, `git filter-branch`.
- **Deleting shared work:** `git push origin --delete <branch>`, or `git branch -D` of a branch others may have.

Why this is a hard rule: a force-push or rewrite changes history that already exists on `origin`, so every other clone diverges from the remote and committed work can be lost. There is almost never a legitimate reason to force-push a shared branch; if a rewrite genuinely seems necessary, **STOP and ask first**. A mechanical block on the force-push variants can be added in [`.claude/settings.json`](.claude/settings.json) (`permissions.deny`); it is a backstop, not a substitute for this rule.

---

## Test-First Development (TDD)

All new behavior is **test-first** - write the failing test that captures the behavior, show it fail, then implement to green. Applies to engine subsystems, gameplay rules, content schemas, and presentation logic; an engine API change is test-first on **both** sides. Documented exception: feel-tuned values and visual quality beyond what a golden image captures, which go to a playtest with written criteria. Full standard: [core_protocol.md](docs/protocol/core_protocol.md#test-first-development-tdd) and [game_test_protocol.md](docs/protocol/game_test_protocol.md); enforcement: [session_protocol.md](docs/protocol/session_protocol.md#enforcement-rules).

---

## File Access Rules

- Edit Style:
  - Surgical edits only - never full rewrites unless structurally required throughout.
- Make Changes:
  - State what you are about to change and why before making the change.
- Changelog File
  - Insert session entries directly via str_replace - never rewrite in full.

Full commit rules and enforcement rules: [session_protocol.md](docs/protocol/session_protocol.md#enforcement-rules).

---

## Files to Never Touch

Generated and vendored output should never be hand-edited by Claude. Adapt this list to your toolchain. Typical entries:

- Build output - `build/`, `out/`, `cmake-build-*/`, and CMake `_deps/`
- `node_modules/` and other dependency install directories
- Generated protobuf stubs (regenerated from the `.proto`, once adopted)

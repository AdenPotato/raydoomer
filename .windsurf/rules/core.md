---
trigger: always_on
---

# Core operating rules

**doomer** - a desktop first-person shooter with a Diablo-like loot loop, in C++23 on raylib and Box3D. The full working agreement lives in the shared docs; this rule is the always-on summary. Read the detail in the protocols under [docs/protocol/](../../docs/protocol).

## Start of session

Read these before non-trivial work: `docs/changelog.md`, `docs/protocol/session_protocol.md`, `docs/protocol/core_protocol.md`, `docs/reference/locked_decisions.md`, `docs/reference/glossary.md`. The full session lifecycle is the `/session-start` and `/session-end` workflows.

## Response style

Answer the exact question asked, then stop - shortest correct answer. No preamble, no restating the question, no unsolicited alternatives or summaries. Expand only when asked or when the task is genuinely complex.

## Accuracy

Verify against source before asserting - read the file, run the query, check the API; do not answer from memory about this codebase or its config. Cite the evidence (path + line, command output, doc). Flag uncertainty instead of confabulating. When corrected, fix the record (doc/rule), not just the reply.

## Writing style

**Never use the em dash (`—`) anywhere** - in copy, code, comments, commits, PRs, docs, or the changelog. Use a spaced hyphen ` - `, a comma, a colon, or parentheses.

## Test-first development (hard rule)

All new behavior is written test-first: a failing test that captures the behavior exists and is shown failing before implementation, then driven to green. Applies to engine subsystems, gameplay rules, content schemas, and presentation logic; an engine API change is test-first on both sides. Documented exception: feel-tuned values and visual quality beyond a golden image, which go to a playtest with written criteria. Full standard: [core_protocol.md](../../docs/protocol/core_protocol.md#test-first-development-tdd) and [game_test_protocol.md](../../docs/protocol/game_test_protocol.md).

## Branch & merge (do not deviate)

`main` is protected - never merge to `main` or commit app work straight to it. All work branches off the long-lived `dev` integration branch as `feature/ADE-<number>` or `bugfix/ADE-<number>` (the Linear issue id); `dev -> main` is a separate, maintainer-only promotion. Full model: [core_protocol.md](../../docs/protocol/core_protocol.md#branching-model-dev-integration).

## Git safety (never rewrite shared history)

Only ever fast-forward push (`git push`). Never force-push (`--force`/`-f`/`--force-with-lease`), rebase shared history, `commit --amend` after a push, `reset --hard` a pushed branch, or delete shared branches - without explicit, in-the-moment approval. If a rewrite seems necessary, STOP and ask.

## Decisions & enforcement

Locked decisions are canon: if work contradicts one, flag it and halt until it is unlocked, changed, documented, and re-locked. If a requirement is vague or self-contradictory, flag before proceeding - agreement is not a valid response to a conflict; pushback is required. Any decision that affects a guiding document (locked_decisions, the protocols, glossary, design specs) must be updated in the same session. Full rules: [session_protocol.md](../../docs/protocol/session_protocol.md#enforcement-rules).

## Edits & commits

Surgical edits only - never full rewrites unless structurally required. State what you are about to change and why before changing it. Insert changelog entries via surgical edit, never a full rewrite. Every commit message and PR body ends with `Co-authored with Claude`; no other auto-generated footer.

## Surfaces

Engine and gameplay work follows the `simulation` rule + [engine_protocol.md](../../docs/protocol/engine_protocol.md) and [gameplay_protocol.md](../../docs/protocol/gameplay_protocol.md); anything the player sees or hears follows the `presentation` rule + [presentation_protocol.md](../../docs/protocol/presentation_protocol.md); definitions, loaders, resources, and saves follow the `content` rule + [content_protocol.md](../../docs/protocol/content_protocol.md). Anything crossing the engine/game boundary obeys [engine_api_protocol.md](../../docs/protocol/engine_api_protocol.md) regardless of which rule is active.

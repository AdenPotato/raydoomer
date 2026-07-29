---
trigger: always_on
---

# Core operating rules

The full working agreement lives in the shared docs; this rule is the always-on summary. Read the detail in the protocols under [docs/protocol/](../../docs/protocol). Replace `<project-name>` and the `<co-author-trailer>` placeholder per project.

## Start of session

Read these before non-trivial work: `docs/changelog.md`, `docs/protocol/session_protocol.md`, `docs/protocol/core_protocol.md`, `docs/reference/locked_decisions.md`, `docs/reference/glossary.md`. The full session lifecycle is the `/session-start` and `/session-end` workflows.

## Response style

Answer the exact question asked, then stop - shortest correct answer. No preamble, no restating the question, no unsolicited alternatives or summaries. Expand only when asked or when the task is genuinely complex.

## Accuracy

Verify against source before asserting - read the file, run the query, check the API; do not answer from memory about this codebase or its config. Cite the evidence (path + line, command output, doc). Flag uncertainty instead of confabulating. When corrected, fix the record (doc/rule), not just the reply.

## Writing style

**Never use the em dash (`—`) anywhere** - in copy, code, comments, commits, PRs, docs, or the changelog. Use a spaced hyphen ` - `, a comma, a colon, or parentheses.

## Test-first development (hard rule)

All new behavior is written test-first: a failing test that captures the behavior exists and is shown failing before implementation, then driven to green. Applies to service endpoints, API contract logic, and UI components/hooks. Documented exception: schema/index/mapping work (you cannot query a collection or index until it exists) ships its spec test in the same change. Full standard: [core_protocol.md](../../docs/protocol/core_protocol.md#test-first-development-tdd).

## Branch & merge (do not deviate)

`main` is protected - never merge to `main` or commit app work straight to it. All work branches off the long-lived `dev` integration branch as `feature/ADE-<number>` or `bugfix/ADE-<number>` (the Linear issue id); `dev -> main` is a separate, maintainer-only promotion. Full model: [core_protocol.md](../../docs/protocol/core_protocol.md#branching-model-dev-integration).

## Git safety (never rewrite shared history)

Only ever fast-forward push (`git push`). Never force-push (`--force`/`-f`/`--force-with-lease`), rebase shared history, `commit --amend` after a push, `reset --hard` a pushed branch, or delete shared branches - without explicit, in-the-moment approval. If a rewrite seems necessary, STOP and ask.

## Decisions & enforcement

Locked decisions are canon: if work contradicts one, flag it and halt until it is unlocked, changed, documented, and re-locked. If a requirement is vague or self-contradictory, flag before proceeding - agreement is not a valid response to a conflict; pushback is required. Any decision that affects a guiding document (locked_decisions, the protocols, glossary, design specs) must be updated in the same session. Full rules: [session_protocol.md](../../docs/protocol/session_protocol.md#enforcement-rules).

## Edits & commits

Surgical edits only - never full rewrites unless structurally required. State what you are about to change and why before changing it. Insert changelog entries via surgical edit, never a full rewrite. Every commit message and PR body ends with the configured `<co-author-trailer>`; no other auto-generated footer.

## Surfaces

UI / client work follows the `frontend` rule + [frontend_protocol.md](../../docs/protocol/frontend_protocol.md); service + data work follows the `backend` rule + [backend_protocol.md](../../docs/protocol/backend_protocol.md); anything crossing the service<->UI boundary follows the `integration` rule + [integration_protocol.md](../../docs/protocol/integration_protocol.md).

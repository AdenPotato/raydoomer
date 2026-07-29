---
description: Start a new session - sync, read protocol context, select the work, prompt for session type, and confirm relevant design files before any work begins.
---

Execute the session start checklist in this exact order.

## Step 0 - Sync (guarded)

Bring the integration branch up to date and prune merged branches **before** reading context or creating any branch.

**Guard:** Skip this step if resuming an in-progress feature branch or the working tree is dirty (`git status --short` non-empty). Never pull into a feature branch mid-task - flag it and continue on the current branch.

If the working tree is clean:

```bash
git checkout dev
git pull --ff-only
git fetch --prune
```

Then delete local branches whose work has already merged. If the repo squash-merges, `git branch --merged` misses them - confirm a branch's work is merged before deleting, and use `git branch -D`.

## Step 1 - Read context files

Read these: `docs/changelog.md`, `docs/protocol/session_protocol.md`, `docs/protocol/core_protocol.md`, `docs/reference/locked_decisions.md`, `docs/reference/glossary.md`.

## Step 2 - Session type

If the developer has not declared the session type, ask: **Tracked, Design, or Sandbox?**

## Step 3 - Select the work

Surface the open, ready GitHub issues from this repository, ordered by priority, for the developer to choose from. Do not create a branch or make changes before the developer selects.

## Step 4 - Read relevant design markdown

Based on the selected work, determine which files under `docs/design/` are relevant (and any index file first). Use judgment - do not read every design file.

## Step 5 - Confirm with developer

State which markdown files were read and why. Ask the developer to confirm before proceeding.

Only after confirmation: for **Tracked sessions**, create the branch off `dev` as `feature/<issue>-<slug>` (any non-bug work) or `bugfix/<issue>-<slug>` (a fix), where `<issue>` is the work item's GitHub issue number.

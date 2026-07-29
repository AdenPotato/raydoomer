---
description: Promote an active Sandbox session to a Tracked session - verify or create a work item, create the correctly named branch, and continue as Tracked.
---

Use this workflow when the developer says "promote to tracked" during a Sandbox session.

## Step 1 - Identify or create the work item

Ask: **What work item is this session being promoted against?** If none exists yet, use the `new-work-item` skill (`@new-work-item`) first, then return here. Verify the issue exists and is open in Linear.

## Step 2 - Determine the branch name

Convention (off `dev`): `feature/ADE-<number>` for any non-bug work, `bugfix/ADE-<number>` for a fix. `ADE-<number>` is the work item's Linear issue id. Check recent branch names to confirm the pattern:

```bash
git branch -a | head -20
```

## Step 3 - Confirm the branch name with the developer

State the proposed branch name (e.g., `feature/ADE-154`) and ask the developer to confirm before creating it.

## Step 4 - Create the branch

```bash
git checkout -b feature/ADE-<number>   # or bugfix/ADE-<number>
git branch --show-current
```

Branch off `dev` per the Branching Model.

## Step 5 - Switch to Tracked mode

State: **"Session promoted to Tracked. Branch `<branch-name>` is active. All Tracked session rules now apply."** From here: every change is committed to this branch, a changelog entry is required at session end, a PR is opened at session end, and `/session-end` applies in full.

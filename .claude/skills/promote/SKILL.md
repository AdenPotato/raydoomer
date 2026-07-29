---
name: promote
description: Promote an active Sandbox session to a Tracked session - verify or create a work item, create the correctly named branch, and continue as Tracked.
---

Use this skill when the developer says "promote to tracked" during a Sandbox session.

## Step 1 - Identify or create the work item

Ask: **What work item is this session being promoted against?**

If none exists yet, run `/new-work-item` first to create one, then return here. Verify the issue exists and is open in GitHub Issues.

## Step 2 - Determine the branch name

Branch naming convention (off `dev`): `feature/<issue>-<slug>` for any non-bug work, `bugfix/<issue>-<slug>` for a fix. `<issue>` is the work item's GitHub issue number. Check recent branch names to confirm the pattern in use:

```bash
git branch -a | head -20
```

## Step 3 - Confirm the branch name with the developer

State the proposed branch name (e.g., `feature/33-player-controller`) and ask the developer to confirm before creating it.

## Step 4 - Create the branch

```bash
git checkout -b feature/<issue>-<slug>   # or bugfix/<issue>-<slug>
git branch --show-current
```

Branch off `dev` per the Branching Model.

## Step 5 - Switch to Tracked mode

State: **"Session promoted to Tracked. Branch `<branch-name>` is active. All Tracked session rules now apply."**

From this point:
- Every change is committed to this branch
- A changelog entry is required at session end
- A PR is opened at session end
- The session end checklist (`/session-end`) applies in full

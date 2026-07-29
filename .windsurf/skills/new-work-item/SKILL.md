---
name: new-work-item
description: Use when capturing a new unit of work as a GitHub issue (this repository). Drafts summary/type/priority and records the issue in one flow.
---

Record a new unit of work as a GitHub issue in this repository - something discussed, decided, or identified as needing follow-up.

## Step 1 - Draft from context, then confirm

Infer the **summary**, **description**, and **type** from the current session context, and draft them per the **GitHub issue template** ([authoring.md](../../../docs/reference/authoring.md)) - a story statement (or, for a bug, observed/expected/repro), brief context, and **acceptance criteria** (one checkbox per observable behavior).

- **Type** is `feature` (a Story/Task - any non-bug work) or `bugfix` (a Bug). It drives the branch prefix later: `feature/<issue>-<slug>` or `bugfix/<issue>-<slug>`.

Present the draft, then ask: (1) **Priority** - GitHub has no priority field, so it is a label: `priority:urgent`, `priority:high`, `priority:medium`, `priority:low`, or none; (2) **Surface label** - `simulation`, `presentation`, `content`, or `infra`; (3) **Parent epic**, if one applies. Do not proceed until the developer confirms the draft.

## Step 2 - Create the GitHub issue

Create the issue with the `gh` CLI from the built-in terminal:

```bash
gh issue create --title "<summary>" --label <surface> --label priority:<level> --body "<description>"
```

Capture the issue number it returns - that is what the branch and the PR's `Closes #<n>` will reference.

## Step 3 - Confirm

Show the developer the issue identifier (`<issue>`), its URL, and the recorded priority.

## Constraints

- The branch derives from the issue: `feature/<issue>-<slug>` (any non-bug work) or `bugfix/<issue>-<slug>` (a fix). Keep the type and key recorded so the branch is derivable.
- Do not invent issue links or relationships the GitHub repository is not set up for; record only what is supported.

---
name: new-work-item
description: Capture a new work item as a Linear issue (ADE team) with a type and priority, in one flow.
---

Use this to record a new unit of work as a Linear issue in the `ADE` team - something discussed, decided, or identified as needing follow-up.

## Step 1 - Draft from context, then confirm

Infer the **summary**, **description**, and **type** from the current session context, and draft them per the **Linear issue template** ([authoring.md](../../../docs/reference/authoring.md)) - a story statement (or, for a bug, observed/expected/repro), brief context, and **acceptance criteria** (one checkbox per observable behavior).

- **Type** is `feature` (a Story/Task - any non-bug work) or `bugfix` (a Bug). It drives the branch prefix later: `feature/ADE-<number>` or `bugfix/ADE-<number>`.

Present the drafted summary, description, and type to the developer, then ask:
1. **Priority** - the Linear priority: `Urgent`, `High`, `Medium`, `Low`, or `No priority` (the API takes `1`-`4`, `0` for none).
2. **Project / parent issue or cycle**, if one applies.

Do not proceed until the developer confirms the draft and provides a priority.

## Step 2 - Create the Linear issue

Create the issue in the `ADE` team via the Linear MCP tools (`save_issue`) with the confirmed summary, description, type, priority, and (if any) project/cycle. Capture the resulting identifier (`ADE-<number>`).

## Step 3 - Confirm

Show the developer the issue identifier (`ADE-<number>`), its URL, and the recorded priority.

## Constraints

- The branch derives from the issue: `feature/ADE-<number>` (any non-bug work) or `bugfix/ADE-<number>` (a fix). Keep the type and key recorded so the branch is derivable.
- Do not invent issue links or relationships the Linear workspace is not set up for; record only what is supported.

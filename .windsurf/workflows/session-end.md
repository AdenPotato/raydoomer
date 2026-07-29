---
description: End a session - produce a summary, update the changelog (tracked only), commit, update or open the PR, and generate an Up Next list from GitHub Issues.
---

Execute the session end checklist in this exact order.

## Step 1 - Session summary

State what was decided, what was built (files changed), which work items were opened, and which were completed.

## Step 2 - File list

List every file changed with a one-line description of what changed and why.

## Step 3 - Up Next list

Fetch the current open, ready GitHub issues sorted by priority from this repository. Hold the top 4-5 for the session summary; GitHub Issues is the canonical next-work source, so do not embed them in the changelog.

## Step 4 - Tracked sessions: changelog entry

Prepend a new entry to the **top** of `docs/changelog.md` (just below the file header, above the newest existing entry) using a surgical edit - never rewrite the file in full. The format rules live in the `## Changelog format` section at the **bottom** of `changelog.md`.

Stamp the time in the author's own local timezone: `date "+%Y-%m-%d %H:%M %Z"`. Use this format:
```
YYYY-MM-DD HH:MM TZ
<Person>

## <Title>

[<category>] <Two sentences describing what happened.>

Tickets: <work-item and PR references>
```

Rules: order is date line -> person -> `## <Title>` -> body -> `Tickets:`; the body opens with the bracketed category tag (`[feature]` · `[enhancement]` · `[bug]` · `[internal]`) then two sentences; never an em dash anywhere; always include the `Tickets:` footer.

## Step 5 - Changelog cleanup

If there are more than 20 entries after inserting the new one, remove the oldest. Max 20 at all times.

## Step 6 - Document update check

Any decision made this session that affects `locked_decisions.md`, `session_protocol.md`, `glossary.md`, or any `docs/design/*` file must have been updated this session. If not, flag it - the session cannot close until the update is made.

## Step 7 - Unresolved items

Any item not fully resolved this session must become a tracked work item before session close (use the `new-work-item` skill, `@new-work-item`). No open-ended prose left anywhere - reference the work-item id only.

## Step 8 - Commit

Stage and commit all changes. Format: `<type>(<scope>): <description>`. Every commit message ends with the configured attribution trailer. No other auto-generated footer.

## Step 9 - Pull request: update existing or create new (Tracked sessions)

Check whether a pull request (PR) is already open for the current branch. **If one exists**, update its body to the **PR summary template** ([authoring.md](../../docs/reference/authoring.md)). **If not**, ask the developer which branch to open against (default: `dev`), then create it - title `<type>(<scope>): <description> [#<n>]`, body to the **PR summary template** (Summary, GitHub Issues `Closes #<n>`, Changes, Testing, Risk & rollout), ending with the attribution trailer.

Record the final PR reference and update the changelog entry (Step 4) with it. Do not delete this session's branch here - merged-branch cleanup happens at the next session start.

## Step 10 - Confirmation

Confirm: document updates applied; changelog entry inserted with PR reference; commit created; PR updated or opened; Up Next list generated.

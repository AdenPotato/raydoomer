---
name: session-end
description: End a session - produce a summary, update the changelog (tracked only), commit, update or open the PR, and generate an Up Next list from Linear.
---

Execute the session end checklist in this exact order.

## Step 1 - Session summary

State:
- What was decided this session
- What was built (files changed)
- Which work items were opened
- Which work items were completed

## Step 2 - File list

List every file changed with a one-line description of what changed and why.

## Step 3 - Up Next list

Fetch the current open, ready Linear issues sorted by priority from the `ADE` team, via the Linear MCP tools (`list_issues`). Hold the top 4-5 for the session **summary**; Linear is the canonical next-work source, so do not embed them in the changelog.

## Step 4 - Tracked sessions: changelog entry

Prepend a new entry to the **top** of `docs/changelog.md` (just below the file header, above the newest existing entry) using a surgical edit (str_replace) - never rewrite the file in full. The format rules live in the `## Changelog format` section at the **bottom** of `changelog.md`.

Stamp the time in the author's **own** local timezone: `date "+%Y-%m-%d %H:%M %Z"`.

Use this exact format (no session numbers - date + person keep it branch-merge-safe):
```
YYYY-MM-DD HH:MM TZ
<Person>

## <Title>

[<category>] <Two sentences describing what happened.>

Tickets: <work-item and PR references>
```

Rules:
- **Order is date line -> person -> `## <Title>` -> body -> `Tickets:`.** Never an em dash anywhere.
- **The body opens with the category tag in brackets:** `[feature]` · `[enhancement]` · `[bug]` · `[internal]`, then the two-sentence summary.
- **Body is two sentences** describing what happened - the deeper detail lives in the PR/commits. No typed bullet list.
- `<Person>` is whoever ran the session (the git author); `<Title>` is a short human summary.
- Always include the `Tickets:` footer.

## Step 5 - Changelog cleanup

Count entries. If there are more than 20 after inserting the new one, remove the oldest (at the bottom). Max 20 at all times.

## Step 6 - Document update check

Verify: any decision made this session that affects `locked_decisions.md`, `session_protocol.md`, `glossary.md`, or any `docs/design/*` file must have been updated during this session. If not, flag it - the session cannot close until the update is made.

## Step 7 - Unresolved items

Any item not fully resolved this session must become a tracked work item before session close. No open-ended prose left anywhere - reference the work-item id only. Use `/new-work-item` to create one.

## Step 8 - Commit

Stage and commit all changes. Commit message format: `<type>(<scope>): <description>`. Every commit message ends with the project's configured attribution trailer (see CLAUDE.md). Do not append any other auto-generated footer.

## Step 9 - Pull request: update existing or create new (Tracked sessions)

First check whether a pull request (PR) is already open for the current branch (e.g. `git log dev..HEAD` to confirm what would ship, and your host's PR tooling to check for an open one).

**If a PR exists** - update its body to the **PR summary template** ([authoring.md](../../../docs/reference/authoring.md)).

**If no PR exists** - ask the developer which branch to open against (default: `dev`), then create it. Title it `<type>(<scope>): <description> [ADE-<n>]`; write the body to the **PR summary template** ([authoring.md](../../../docs/reference/authoring.md)) - Summary, Linear (`Closes ADE-<n>`), Changes, Testing, Risk & rollout - ending with the configured attribution trailer. Do not append any other auto-generated footer.

Record the final PR reference and update the changelog entry (Step 4) with it before the commit in Step 8 - or amend the entry if already committed.

**Branch cleanup:** Do not delete this session's branch here - its PR is open, not yet merged. Merged-branch cleanup happens automatically at the next session start.

## Step 10 - Confirmation

Confirm:
- All required document updates applied
- Changelog entry inserted with correct PR reference
- Commit created
- PR updated or opened
- Up Next list generated

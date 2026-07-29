---
name: pr-summary
description: Use to generate a pull-request description from the current branch diff, to the team's PR summary template.
---

Produce or refresh a PR body **any time** - not only at session close (the `session-end` workflow does this at the end). Canonical template: [authoring.md](../../../docs/reference/authoring.md#pull-request-pr-summary-template).

## Steps (in order)

1. Inspect what would ship: `git log dev..HEAD --oneline` and `git diff dev...HEAD --stat` (adjust the base branch if not `dev`).
2. **Write the body to the PR summary template:** Summary (the why, 1-3 sentences), GitHub Issues (`Closes #<n>`), Changes (bulleted), Testing (tests added + a QA checklist for app-facing changes), Risk & rollout (backward-compatible API change? new indexes / ES mapping / reindex? anything to watch on deploy).
3. **Title:** `<type>(<scope>): <description> [#<n>]` (conventional commits + the GitHub issue number).
4. End the body with the configured attribution trailer; no other auto-generated footer. Output the title + body for the developer, or apply it to the open PR.

## Checks
- [ ] title is conventional-commit + the `#<n>` key
- [ ] Summary is reviewer-facing (the why, not a diff restatement)
- [ ] Risk & rollout calls out backward-compat + data/index/mapping changes
- [ ] ends with the attribution trailer

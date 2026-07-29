---
name: write-story
description: Draft or refine a Linear issue to the team's story template - title, statement, context, acceptance criteria, scope, Definition of Done.
---

Use to author or tighten a Linear issue (new or existing) **without** creating/updating the issue itself - that is `/new-work-item`. Canonical template: [authoring.md](../../../docs/reference/authoring.md#linear-issue-template).

## Steps (in order)

1. Gather the intent from the developer or session context: the outcome, the role it serves, and the why.
2. **Draft to the story template:** typed summary (`Feature:` / `Bug:`); a story statement ("As a `<role>`, I want `<capability>`, so that `<benefit>`") or, for a bug, observed/expected/repro; brief context with links; **acceptance criteria** (one checkbox per observable behavior, or Gherkin for a complex flow); out-of-scope; dependencies; Definition of Done.
3. Flag anything too big to demo in one sitting - propose a split into separate stories.
4. Hand the result to `/new-work-item` to create the Linear issue, or paste it into the existing one.

## Checks
- [ ] typed title (drives the `feature/` or `bugfix/` branch)
- [ ] statement (or bug observed/expected/repro) + brief context
- [ ] acceptance criteria are observable and testable
- [ ] out-of-scope + dependencies noted; DoD points at the PR checklist

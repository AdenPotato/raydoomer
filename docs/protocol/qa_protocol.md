# QA Protocol (template)

How manual QA is run and recorded, on top of the automated in-repo gate (typecheck, lint, tests, build) that runs before any PR. This file is a **template** - fill in the bracketed parts for your project, or delete the sections that do not apply (a project leaning hard on automated tests may need very little here).

---

## When manual QA is required

| Change type                                                            | Manual QA                               |
|------------------------------------------------------------------------|-----------------------------------------|
| User-facing behavior (screens, auth, navigation, anything a user sees) | **Required before merge**               |
| Service-only change covered by integration tests                       | Not required - automated checks suffice |
| Docs, protocol, CI, tooling, types, refactors with no behavior change  | Not required - automated checks suffice |

If in doubt, treat it as user-facing.

**Where this gate sits (dev-branch model):** app work merges to **`dev`** via PR (CI-gated). A change needing manual QA is labeled **`needs-qa`** on `dev`; a reviewer exercises it and applies **`qa-passed`**. `qa-passed` is the prerequisite for the maintainer-only `dev -> main` promotion (see [core_protocol.md](core_protocol.md#branching-model-dev-integration) + [session_protocol.md](session_protocol.md#enforcement-rules) Rule 11). The agent never merges to `main`.

---

## Roles

`<List who runs QA and how coverage is split - e.g. by platform, by surface, or round-robin. A change may merge once its required coverage is checked off.>`

---

## Ready-to-test handoff (acceptance criteria)

When the agent has a feature branch green (CI passing) and ready for QA, **before** the reviewer exercises it, the agent posts the **acceptance criteria** as a checkbox list - the observable behaviors to verify (one box per behavior). The reviewer ticks each box. When QA passes, the PR carries the same checklist as the durable record.

---

## Per-PR QA checklist convention

Every user-facing PR carries a **QA checklist as checkboxes in the PR description**. The author seeds it; testers tick boxes and note environment next to each.

- On open, label the PR **`needs-qa`** - by convention it is not merged while this label is on.
- When the required coverage is checked off, swap the label to **`qa-passed`**; the PR is then mergeable.
- A failed case -> comment with environment + repro, leave `needs-qa`, fix, re-QA.

A reusable checklist shape (adapt per feature):

```
## QA checklist (manual)
- [ ] <setup / config step>
- [ ] <build / run step per environment>
- [ ] <test case>  - _tester, environment_
```

---

## Environment matrix (template)

Kept current as setups change. Fill in the columns relevant to your project (OS, devices, browsers, toolchain versions, accounts).

| Tester   | OS / machine | Toolchain    | Devices / browsers | Accounts |
|----------|--------------|--------------|--------------------|----------|
| `<name>` | `<os>`       | `<versions>` | `<targets>`        | `<...>`  |

---

## Test data

- **Standing test user / account** - `<describe how to reach auth-gated surfaces without real credentials, e.g. a provider test mode>`.
- **Mocks (no live dependencies):** `<describe the mock toggle and how to enter the app as a guest>`.
- **Never use production credentials or copy production data for QA** - dev/sandbox only; production data is PII.

---

## Recording & sign-off

- The PR's checked QA boxes (with environment notes) are the durable record - they live with the change.
- `qa-passed` label = sign-off that required coverage ran green.

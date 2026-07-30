# QA Protocol

How playtesting is run and recorded, on top of the automated gates that run before any PR.

This document exists because of a hard limit: **the agent cannot see or hear the game.** Everything about feel, timing and visual quality is invisible to it. `game_test_protocol.md` names that as the one documented exception to test-first, and this file is where the exception lands. A change routed here is not unverified - it is verified by a human, deliberately, against written criteria.

---

## When a playtest is required

| Change type                                                                                                  | Playtest                                  |
|--------------------------------------------------------------------------------------------------------------|-------------------------------------------|
| Anything feel-tuned - movement, weapons, camera, timing, pacing                                              | **Required before promotion**             |
| Anything visual beyond what a golden image captures                                                          | **Required before promotion**             |
| Audio                                                                                | **Required** - a mocked device proves nothing about how it sounds |
| A tuning value change, however small                                     | **Required** - a balance change silently invalidates every playtest before it |
| Pure logic covered by simulation tests - loaders, save data, maths                                           | Not required, automated checks suffice    |
| Docs, protocol, CI, tooling, refactors with no behaviour change                                              | Not required                              |

If in doubt, treat it as needing a playtest. The cost of one is a few minutes; the cost of shipping a movement change nobody moved with is discovering it three features later.

**Where this gate sits:** work merges to `dev` via a CI-gated PR. A change needing a playtest is labelled **`needs-qa`**; once exercised it gets **`qa-passed`**, which is a prerequisite for the maintainer-only `dev -> main` promotion ([core_protocol.md](core_protocol.md#branching-model-dev-integration), [session_protocol.md](session_protocol.md#enforcement-rules) Rule 11). The agent never merges to `main`.

---

## The agent cannot playtest

Stated plainly because it governs everything below.

- **Every feel claim from the agent is a hypothesis.** "The dash feels snappier" is not a result; "the dash cooldown is now 0.4s and a test asserts it" is.
- **A green test is not a working feature.** It is evidence about the slice it covers, and the agent must say which slice.
- **The agent separates what it verified from what it changed** in every PR touching this territory.

Confabulating a play experience is the single most damaging thing that can happen here, because it is the one claim nobody can check without stopping to run the game.

---

## Running a playtest

1. **Read the criteria first.** The PR carries a checklist of observable behaviours. Read it before playing, so you know what you are looking for rather than forming an impression and rationalising it.
2. **Play the specific thing.** Not a general session. If the change is about air control, jump repeatedly and move in the air.
3. **Tick or fail each box**, noting the environment beside it.
4. **Describe failures observably.** "Turning feels sluggish above 100fps" is actionable. "Feels off" is not.

---

## Reporting feel feedback

Vague feel feedback is the most common instruction in game development and the most dangerous to act on directly. Guessing produces a rewrite that changes ten things, nine of which were fine.

`gameplay_protocol.md` requires this shape, and it is worth following as a tester too:

1. **Restate the symptom observably** - what happens, when, and what you expected instead.
2. **Name the tunable you think is responsible**, if you have a guess.
3. **State the direction** - which value, moving which way.
4. **One change at a time.** Two at once and you cannot attribute the result.
5. **Re-playtest.** A balance change invalidates every playtest that came before it.

If no existing tunable can produce the requested change, that is the finding. The agent reports it and proposes the smallest new tunable rather than restructuring a system on a hunch.

---

## Per-PR checklist convention

Every playtest-requiring PR carries its checklist as checkboxes in the description. The agent seeds it from the issue's acceptance criteria; the tester ticks them.

- On open, label **`needs-qa`**. By convention it is not promoted while that label is on.
- When coverage is checked off, swap to **`qa-passed`**.
- A failure gets a comment with environment and repro steps; the label stays `needs-qa`.

```
## Playtest checklist
- [ ] <observable behaviour>  - _tester, build, resolution, refresh rate_
- [ ] <edge case>
- [ ] Frame time stayed within budget - _measured, not impression_
```

---

## Environment

What actually varies for this project, and therefore what is worth recording next to a result.

| Field         | Why it matters                                                                                                                                      |
|---------------|-----------------------------------------------------------------------------------------------------------------------------------------------------|
| Build preset  | `windows-debug` has asserts and the debug view; `windows-release` does not                                                                          |
| Commit        | So a result can be tied to an exact build                                                                                                           |
| GPU + driver  | Rendering differences and frame rate both track it                                                                                                  |
| Resolution    | HUD layout and readability depend on it; verify the smallest supported one                                                                          |
| Refresh rate  | **The one most often forgotten.** Simulation runs at a fixed 60Hz while presentation is uncapped, so a frame-rate dependency only shows above 60fps |
| Input device  | Mouse DPI and sensitivity change how movement and aiming feel                                                                                       |

> **Test above 60fps at least once per feel change.** The fixed-step loop exists to make the game behave identically at any frame rate. A bug in that is invisible at 60 and obvious at 144, and testing only at 60 is how it ships.

---

## Test data

- **Levels and content come from `data/`**, which is committed and synthetic. There is no production data and no accounts.
- **`resources/` holds copyrighted Doom data for local prototyping only.** It is gitignored and never ships. A playtest that is checking distribution readiness must run against FreeDoom instead.
- **Saves are versioned.** When testing a save-affecting change, keep a save from the previous version to exercise the migration.

---

## Recording and sign-off

- The PR's ticked checklist, with environment notes, is the durable record. It lives with the change.
- **`qa-passed` means the required coverage ran green on a real build**, not that it looked fine in a screenshot.
- A re-baked golden image is a reviewed change: say why in the commit, and treat an unexplained re-bake as a red flag ([game_test_protocol.md](game_test_protocol.md)).

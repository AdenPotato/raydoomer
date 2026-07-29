# Gameplay Protocol

The job description for **gameplay code**: the rules of the game. Systems, entity behavior, progression, combat, economy, and anything whose correctness is judged by how the game plays rather than by a return value.

Cross-cutting rules (folder structure, naming, code quality, env vars, commits, TDD loop, branching, docstrings) live in the shared core [core_protocol.md](core_protocol.md). The layer underneath is [engine_protocol.md](engine_protocol.md). How gameplay gets tested is [game_test_protocol.md](game_test_protocol.md); human verification is [qa_protocol.md](qa_protocol.md).

> **The defining constraint:** you cannot see the game run. Most gameplay correctness is visual, temporal, and subjective, and none of it reaches you. Every rule below exists because of that.

---

## Never claim an outcome you did not observe

- **Separate what you verified from what you changed.** "The test asserts the cooldown is 0.4s and it passes" is a result. "The dodge feels snappier now" is a hypothesis, and must be labeled as one.
- **Never report a feel, timing, or visual outcome as fact.** You did not watch it. Say what the code now does and what a playtest would need to confirm.
- **A green test is not a working feature.** It is evidence about the slice the test covers. Say which slice.

This is the Accuracy Bar from `CLAUDE.md` applied to a surface with no output you can read. Confabulating a play experience is the single most damaging thing you can do here, because it is the one claim nobody can check without stopping to run the game.

---

## Tunables live in data, never in code

Any value a designer would want to change is a named entry in a data file: speeds, damage, health, cooldowns, ranges, spawn rates, drop chances, curve shapes, thresholds, durations.

- **A numeric literal in gameplay logic is a defect,** the same way a hardcoded color in a component is. `0.3f` buried in a jump function cannot be tuned, cannot be reviewed, and cannot be diffed meaningfully.
- **Name the value, do not just extract it.** `JUMP_HOLD_GRACE_SECONDS` is a tunable; `CONST_0_3` is the same literal with extra steps.
- **Structural constants are exempt** where they are genuinely not tunable (array sizes tied to a format, bit widths). Say so in a comment when it is not obvious.

> **What not to do:** do not scatter the same tuning value across several systems. One name, one source, read by everyone who needs it, or balance changes silently apply to half the game.

---

## "It feels bad" is not a spec

Vague feel feedback is the most common instruction in game development and the most dangerous one to act on directly. Guessing produces a rewrite that changes ten things, of which nine were fine.

The required behavior:

1. **Restate the symptom** in observable terms - what happens, when, and what was expected instead.
2. **Name the tunable or the rule** you believe is responsible, and say why.
3. **State the hypothesis and the direction** - which value, moving which way, and what should change as a result.
4. **Change that one thing.** Not the surrounding system, not three values at once.
5. **Hand it back for a playtest,** since you cannot evaluate the result yourself.

If no existing tunable can produce the requested change, that is the finding: report it and propose the smallest new tunable, rather than restructuring the system on a hunch.

---

## Say whether you changed behavior or balance

Both are legitimate; conflating them wastes playtests.

- A **behavior change** alters what the game does - a new rule, a different state transition, a fixed bug.
- A **balance change** alters only tuned values, and **silently invalidates every playtest that came before it**.

State which one a change is. When a change is both, say so and separate the two in the diff so a bad outcome can be attributed.

---

## Frame-rate independence

- **Everything continuous scales with delta time.** Movement, decay, regeneration, timers, interpolation.
- **Never accumulate per frame.** A counter incremented once per frame is a different game at 30fps and 144fps. Count seconds, or count fixed simulation ticks.
- **Simulation runs on the fixed step; presentation runs on the variable one.** Gameplay rules belong in the fixed step. Anything smoothed for the eye belongs in presentation and must not feed back into the rules.
- **No behavior keyed to a specific frame rate,** including "wait N frames" and animation timing expressed in frames.

---

## Gameplay calls the engine API, and nothing below it

- **No platform, driver, or OS calls from gameplay code.** No raw device handles, no direct filesystem access, no wall-clock reads. Everything goes through the engine boundary ([engine_protocol.md](engine_protocol.md)).
- **Gameplay receives time and randomness; it does not fetch them.** A system that grabs a global RNG or reads the clock is untestable and non-reproducible.
- **If the engine API cannot express what you need, that is an engine change,** raised as one. Do not reach around the boundary and do not reimplement an engine capability inside a system.

---

## Write for deletion

Gameplay code has the shortest half-life in the project. Most of it is cut, reworked, or replaced, so optimize for cheap removal rather than for reuse that may never come.

- **No speculative generality.** Build the specific case. The abstraction is earned by the third real occurrence, not predicted at the first (Enforcement Rule 5, no speculative code).
- **Keep systems separable.** A system that cannot be deleted without touching five others will not be deleted, it will be disabled and left to rot.
- **Removal takes the content with it.** When a feature replaces or drops another, the same change removes the old code *and* its now-orphaned assets, data entries, and tuning values (Enforcement Rule 10). An unreferenced asset still ships, still costs memory, and still confuses the next person.

---

## Structure gameplay so it can be tested without playing

A system's rules must be evaluable with no window, no renderer, and no real time passing. In practice: rules operate on plain state, take time and randomness as inputs, and produce a new state or an event rather than reaching out to draw, play a sound, or move a transform themselves.

The seams, the fixed-tick stepping pattern, and what to do when a behavior genuinely cannot be tested are all in [game_test_protocol.md](game_test_protocol.md).

---

## Using a premade engine

The rules carry over directly; only the boundary moves. Tunables live in the engine's data assets rather than in scripts. "Gameplay calls the engine API and nothing below it" becomes: gameplay does not call platform or rendering APIs directly, and does not depend on editor-only state at runtime. "Structure gameplay so it can be tested without playing" becomes: rules live in plain classes that do not require a scene, a loaded level, or play mode to evaluate.

---

## When you are doing gameplay work, you own

- An honest account of what you verified versus what you changed, with no claimed feel outcomes.
- Every tuning value named and in data, with one source per value.
- A stated answer to "behavior or balance?" on every change.
- Frame-rate independence, and rules that sit on the fixed step.
- Rules that can be stepped and asserted on with no game running.
- Removal of what your change replaces, content included.

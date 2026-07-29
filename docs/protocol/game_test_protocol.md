# Game Test Protocol

How **test-first development applies to game code**. The TDD loop itself (red, green, refactor) and the hard requirement are defined once in [core_protocol.md](core_protocol.md#test-first-development-tdd); this document does not restate them. It answers the question that stalls agents on game work: *what is the failing test, when the thing you are building is motion?*

Engine rules are [engine_protocol.md](engine_protocol.md); gameplay rules are [gameplay_protocol.md](gameplay_protocol.md); human verification is [qa_protocol.md](qa_protocol.md).

> **The premise:** game code is not untestable, it is untestable *by default*. Written with the seams below, nearly all of it is ordinary deterministic logic. The genuinely untestable part is small, and it is named at the bottom of this document.

---

## What "the test written first" means per layer

This is the game-side companion to the layer table in [core_protocol.md](core_protocol.md#test-first-development-tdd).

| Layer                              | The test written first                                                                                                                        |
|------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| Engine logic (math, allocation, collision, input mapping) | A unit test over plain inputs and outputs. No window, no device, no clock.                                             |
| Serialization / asset format       | A **round-trip test**: write, read back, assert equality, plus a test that an older version still loads.                                      |
| Gameplay rule                      | A **simulation test**: build the state, step a known number of fixed ticks with an injected clock, assert the resulting state or events.      |
| Time-dependent behavior            | Same, with the tick count as the variable. Assert against elapsed simulated seconds, never wall-clock time.                                   |
| Randomized behavior                | Seed the injected RNG and assert the exact outcome, plus a distribution assertion over many seeded runs where the spread is the behavior.     |
| Save data                          | A round-trip test and a **migration test** from each supported older version.                                                                 |
| Rendering output                   | A **golden-image test** against a committed reference frame, within a documented threshold.                                                   |
| Feel and game balance              | **Not a test.** A playtest with written acceptance criteria ([qa_protocol.md](qa_protocol.md)). See the documented exception below.           |

---

## The four seams

Game code is testable when these four exist. They are requirements on how the code is written, not testing conveniences.

1. **An injected clock.** Nothing reads the wall clock. Time arrives as a parameter, so a test can advance it exactly.
2. **A seeded, injected RNG.** Nothing reaches for a global random source. A test seeds it and gets the same run every time.
3. **Headless simulation.** The rules can be stepped with no window, renderer, or audio device. Presentation observes the simulation; it is never where the rules live.
4. **No hidden global state.** Context is passed explicitly, so two tests in the same process cannot contaminate each other.

> **Gotcha:** the seams are cheap to build at the start and expensive to retrofit, because retrofitting means touching every call site that grabbed the clock or the RNG. Build them the first time a system needs time or randomness, not the tenth.

---

## Controlling time

- **Step, never sleep.** A test advances the simulation by an explicit number of fixed ticks. A test containing a sleep is wrong, slow, and flaky.
- **Assert on simulated seconds or tick counts,** which are exact, not on real elapsed time.
- **Cover the tick boundary.** Behavior at exactly the threshold tick is where off-by-one bugs live: assert the tick before, the tick at, and the tick after.
- **Long durations are cheap.** Stepping ten simulated minutes costs nothing when time is injected; test the timeout, the decay, and the wrap-around.

---

## Floating point

- **Never assert exact equality** on a value that has been through arithmetic. Compare within an epsilon appropriate to the magnitude.
- **Choose the epsilon deliberately** and keep it tight. A tolerance loose enough to hide a real regression is worse than no assertion.
- **Prefer asserting the invariant over the value** where you can: a normalized vector has length 1, energy does not increase, a position stays inside the bounds.

---

## Golden-image tests

The answer to "you cannot test rendering". Render a fixed scene headless, compare the output against a committed reference image within a threshold.

- **The scene must be deterministic** - fixed camera, fixed seed, fixed simulated time, no animation sampled from the wall clock.
- **Compare with a threshold, not byte equality.** Different drivers and different hardware produce small differences; the threshold absorbs that and nothing more.
- **A re-baked reference is a reviewed change.** Updating the golden image is how a rendering regression gets rubber-stamped into the repo. When the reference changes, say why in the commit, and treat an unexplained re-bake as a red flag in review.
- **Store references as tracked binary assets** and keep the set small. A hundred goldens nobody looks at is a hundred rubber stamps.

---

## The documented exception

**Feel-tuned values and visual quality beyond what a golden image captures** cannot have a failing test written first. There is no assertion for "the jump arc reads well" or "the hit lands with weight". This is the game-side sibling of the schema exception in [core_protocol.md](core_protocol.md#test-first-development-tdd).

When it applies:

- **State it explicitly, and say which category the change falls into** - tested behavior, or feel. A change is usually part one and part the other; separate them in the diff.
- **The mechanical part still gets a test.** "The dash applies for the configured duration and cancels on hit" is testable. Only "the dash feels right" is not.
- **Verification moves to a playtest** with written acceptance criteria ([qa_protocol.md](qa_protocol.md)), and the tuned value stays in data ([gameplay_protocol.md](gameplay_protocol.md)) so it can be changed without another code change.
- **No other exceptions without a stated reason.** "It is hard to test" is not one; it usually means a seam is missing.

---

## A flaky game test is a determinism bug

Do not retry it, loosen it, or mark it as known-flaky. Intermittent failure in game code almost always means real non-determinism that will eventually show up as a real bug: unstable iteration order, an unseeded random source, a wall-clock read, uninitialized state, or a frame-rate dependency.

Treat the flake as the finding. Track it and fix the cause.

---

## What not to do

- **Do not write a test that needs a window, a GPU, or an audio device** outside the golden-image set. It will not run in CI, and CI is where tests earn their keep.
- **Do not test the renderer by eyeballing a screenshot in review.** That is a playtest wearing a test's clothing; either commit a golden or call it a playtest.
- **Do not assert on frame counts.** Frames are not a unit of time.
- **Do not use real randomness or real time** to "make the test realistic". It makes the test unreproducible.
- **Do not build a whole level or load real content to test one rule.** Construct the minimal state directly; a test that needs the game's content coupled to it breaks every time the content changes.

---

## When you are writing game tests, you own

- The failing test before the implementation, per the layer table above.
- The seams that made it possible: injected clock, seeded RNG, headless stepping, no globals.
- An explicit statement of which part of a change is tested behavior and which part is feel.
- Deterministic tests, with any flake treated as a real defect.
- Golden references that are small, deterministic, and re-baked only with a stated reason.

# Glossary

**Rule:** This file is a true index - term + one-line definition + link to the canonical spec. No inline spec content.
**Rule:** Definitions must remain stable unless a term is renamed or expanded.

Sections reflect the locked stack (see [locked_decisions.md](locked_decisions.md)) and the runtime design ([runtime_architecture.md](../design/runtime_architecture.md)).

---

## Domain Terms

Your project's vocabulary - the nouns and verbs of the product. Each links to its canonical spec.

| Term             | Definition                                                                                             | Spec                                                                 |
|------------------|--------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------|
| Guiding documents| The markdown files defining project rules, decisions, vocabulary, and design. They govern all sessions.| session_protocol.md, locked_decisions.md, glossary.md, docs/design/* |
| Brush            | An axis-aligned box that forms a piece of level geometry.                                              | runtime_architecture.md                                              |
| World            | The root container holding all simulation state for the current level.                                 | runtime_architecture.md                                              |
| Tick             | One fixed 60 Hz simulation step. The unit of simulated time.                                           | runtime_architecture.md                                              |
| Entity           | A simulated actor: player, enemy, projectile, or pickup.                                               | runtime_architecture.md                                              |
| Billboard sprite | A 2D sprite that always faces the camera. How all actors are drawn.                                    | runtime_architecture.md                                              |
| Run              | One playthrough attempt through a sequence of levels.                                                  | runtime_architecture.md                                              |
| Affix            | A randomized stat modifier attached to an item.                                                        | ADE-16                                                               |
| Rarity tier      | An item quality band that governs affix count and magnitude.                                           | ADE-16                                                               |
| Tunable          | A named gameplay value living in a data file, never a literal in code.                                 | gameplay_protocol.md                                                 |
| Feel             | Subjective play quality. Verified by playtest, never by a test.                                        | game_test_protocol.md                                                |

---

## Technical Terms

| Term                 | Definition                                                       | Code equivalent                |
|----------------------|------------------------------------------------------------------|--------------------------------|
| Fixed timestep       | Simulation advances in equal 60 Hz steps, independent of framerate.| SIM_TICK_HZ = 60             |
| Interpolation alpha  | Fraction between the last two sim states, used to smooth drawing.  | alpha in render()            |
| Frame budget         | The locked per-frame time target. Blowing it halts and flags.      | FRAME_BUDGET_MS = 6.94       |
| Headless simulation  | Stepping the rules with no window, renderer, or audio device.      | World::tick in tests         |
| Injected clock       | Time arrives as a parameter; nothing reads the wall clock.         | dt parameter                 |
| Seeded RNG           | An injected, reproducible random source.                           | Rng&                         |
| Immediate dispatch   | Publishing an event runs its handlers inline, right then.          | EventQueue::publish          |
| Reentrancy guard     | A depth counter asserting a handler does not re-enter its system.  | dev-build assert             |
| Box3D                | The 3D physics engine. Run single-threaded for determinism.        | engine/physics               |
| Body handle          | An opaque reference to a Box3D body. Gameplay cannot dereference it.| BodyHandle                  |
| Platform seam        | The interface isolating OS and device calls from everything above. | src/platform/                |
| Unit test            | Validates a single function or class.                              | GoogleTest                   |
| Simulation test      | Builds state, steps N ticks, asserts the result.                   | GoogleTest, headless         |
| Golden image         | A committed reference frame compared within a threshold.           | tests/golden/                |
| Build tool           | Configures, fetches dependencies, and compiles the binary.         | CMake + FetchContent         |

---

## Presentation / HUD Terms

| Term         | Definition                                                         | Code equivalent      |
|--------------|--------------------------------------------------------------------|----------------------|
| Presentation | Everything drawn or played. Observes simulation, never feeds back. | render()             |
| HUD          | The overlay showing health, ammo, and inventory state.             | game/hud             |
| Design token | A reusable styling value for HUD elements.                         | style_guide.md       |
| Sprite sheet | A packed set of sprite frames loaded as one texture.               | engine/resources     |

---

## Data / Content Terms

| Term          | Definition                                                             | Code equivalent      |
|---------------|------------------------------------------------------------------------|----------------------|
| Content file  | A JSON file defining weapons, enemies, items, or drop tables.          | data/*.json          |
| Level file    | A JSON brush list plus spawn and entity placements.                    | data/levels/*.json   |
| Save file     | Versioned JSON persisted at a level boundary.                          | saves/*.json         |
| Migration     | Converting an older save version forward. Requires its own test.       | game/save            |
| WAD           | Doom's asset container format. The loader targets the format, not content. | engine/resources |
| Lump          | A single named asset inside a WAD.                                     | engine/resources     |
| FreeDoom      | Libre, drop-in compatible asset set used for distribution.             | resources/           |

---

## Operational Terms

| Term           | Definition                                       | Code equivalent          |
|----------------|--------------------------------------------------|--------------------------|
| Tracked session| Produces commits, changelog entries, and PRs.    | feature/bugfix ADE branch|
| Sandbox session| A non-committing exploratory session.            | no branch                |
| Locked decision| A decision needing an unlock process to change.  | locked_decisions.md      |
| Unlock process | The procedure for modifying a locked decision.   | protocol-defined         |
| Changelog      | A chronological record of session outcomes.      | changelog.md             |

---

## Distribution Terms

| Term         | Definition                                        | Code equivalent |
|--------------|---------------------------------------------------|-----------------|
| Binary       | The single desktop executable this repo builds.   | doomer.exe      |
| MinGW-w64    | The toolchain producing the Windows build.        | CMake toolchain |
| WSLg         | The WSL2 graphics layer used for local iteration. | dev host only   |
| Pod          | A running instance of a deployable.                | k8s pod        |
| Service      | Stable in-cluster endpoint for a deployable.       | k8s Service    |

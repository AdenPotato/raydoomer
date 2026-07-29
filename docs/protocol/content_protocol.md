# Content Protocol

The job description for **content and data work**: the definitions the game is made of, the loaders that read them, the resources they become, and the save data that persists.

Cross-cutting rules (folder structure, naming, code quality, commits, TDD loop, branching, docstrings) live in the shared core [core_protocol.md](core_protocol.md). Engine-layer rules are [engine_protocol.md](engine_protocol.md); the rules that consume this content are [gameplay_protocol.md](gameplay_protocol.md); the engine API seam is [engine_api_protocol.md](engine_api_protocol.md).

> **Stack:** content and save data are **JSON** (nlohmann/json). Art and audio arrive as **WAD lumps**, read by format rather than by content. See [runtime_architecture.md](../design/runtime_architecture.md).

---

## Content lives in data, and data is not code

- **Every tunable and every content definition is a named entry in a file under `data/`.** Weapons, enemies, items, affixes, drop tables, level layouts. A numeric literal in gameplay logic is a defect ([gameplay_protocol.md](gameplay_protocol.md)).
- **One name, one source.** A value several systems read is defined once. The same tuning value copied into two files means a balance change silently applies to half the game.
- **Content files are hand-editable and diff readably.** A change to enemy health should be a one-line diff a human can review.
- **Structural constants are exempt** where they are genuinely not tunable (a format's field width, an array size tied to a file layout). Say so in a comment when it is not obvious.

> **What not to do:** do not embed content in code "for now", and do not scatter a single concept across several files because it was convenient at the time. The definition of an item lives in one place.

---

## Loaders validate at the boundary

A loader is the only place that turns bytes into game data, and it is where correctness is enforced.

- **Validate once, at the boundary.** The loader checks the file; everything downstream trusts what it was handed rather than re-checking at every use.
- **A malformed file returns a typed error naming what failed** - which file, which field, what was expected. The loader reports failure; whether a missing item definition is fatal is a game decision, not a loader decision ([engine_protocol.md](engine_protocol.md)).
- **Never crash on bad input.** A truncated WAD, a level with a missing `spawn`, an item referencing an unknown affix: each is an expected operational error with a clear message.
- **Load by format, never by specific content.** The WAD reader resolves lumps by name; it does not know that `SHOTA0` is a shotgun. That separation is what lets the asset set be swapped without a code change.

> **Gotcha:** a loader that silently defaults a missing field hides content bugs until a playtest. Default deliberately and log it, or reject it - never both silently.

---

## Resource ownership and lifetime

Loaded content becomes a resource, and every resource has one owner.

- **One owner, named in the API.** Textures, sprite sheets, sound buffers, file handles: it is explicit who releases it and when.
- **Every acquire has a matching release, including on the error path.** An early return that skips cleanup is a leak.
- **Gameplay receives handles, not pointers.** A handle cannot be dereferenced or freed by the holder, and a stale one is detected and reported rather than followed.
- **Teardown is exercised, not assumed.** Load-and-unload is a tested cycle; a resource system that only works because the process exits is not finished.
- **Nothing loads in the hot path.** File I/O happens at level load or behind an already-populated cache, never inside a tick ([engine_protocol.md](engine_protocol.md)).

---

## Save data

Saves are written at level boundaries and carry player stats, inventory, equipment, level progress, currency, and the RNG seed. Mid-level world state is never serialized ([runtime_architecture.md](../design/runtime_architecture.md)).

- **Every save file carries a version field.** No exceptions. A save without a version is unmigratable forever.
- **A schema change ships with its migration.** Adding a field means deciding what an older save gets, and writing the test that proves it.
- **Migrations are forward-only and idempotent.** Running one twice produces the same result.
- **A save is not a trust boundary you can ignore.** A hand-edited save should fail validation cleanly rather than corrupt the run.

---

## Invariants live in code, test-first

JSON enforces nothing. There is no schema engine, no foreign key, no constraint. So every rule about content correctness is a rule you write and test, and **an unenforced invariant is a defect, not a gap**:

- **Single-value rule** (a bound, a range, a required field) is validated in the loader before the value is used, with a test asserting the violating file is rejected with the expected error.
- **Cross-reference rule** (an item referencing an affix, a level referencing an entity type, a drop table referencing an item) is validated after all content loads, when the referenced set is known. A dangling reference is a load failure, not a runtime surprise.
- **Content not authored yet** gets no speculative guard. Record the rule so it is explicit, and enforce it when that content exists (Enforcement Rule 5, no speculative code).

> **What not to do:** do not rely on "the content is authored by us so it will be correct". Content is edited by hand, in bulk, late at night, and a dangling reference that surfaces as a crash three levels in costs far more than the validation would have.

---

## Testing

Content code is ordinary deterministic logic over plain inputs, so it gets **no TDD exception**.

| What                       | The test written first                                                                        |
|----------------------------|-----------------------------------------------------------------------------------------------|
| A content definition schema| Load a fixture, assert the parsed values. Load a malformed fixture, assert the typed error.   |
| A loader                   | Round-trip: read, write, read back, assert equality.                                          |
| Save data                  | Round-trip **and** a migration test from each supported older version.                        |
| A cross-reference rule     | A fixture with a dangling reference fails to load, with the expected message.                 |
| Resource lifetime          | Load and unload in a test; assert nothing leaks and a stale handle is reported.               |

**Fixtures are committed and synthetic.** Tests read small fixtures from the test tree, never from `resources/`. That directory holds copyrighted material, is gitignored, and does not exist in CI. A test that depends on it is a test that cannot run.

---

## When you are doing content work, you own

- Definitions in data with one source per value, and no literals leaking into logic.
- A loader that validates at the boundary and reports typed failure without crashing.
- Resources with a named owner, a matching release, and an exercised teardown.
- A version field on every save, and a migration test for every schema change.
- Invariants enforced in code and proven by a test, never assumed from authoring discipline.
- Fixtures that are synthetic and committed, so the suite runs in CI.

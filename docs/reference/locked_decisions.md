# Locked Decisions

The registry of decisions that are settled and should not be re-litigated casually. This file is the **canon**: when work contradicts a locked decision, the agent flags it and halts until resolved ([session_protocol.md](../protocol/session_protocol.md#enforcement-rules), Rules 1 and 4).

---

## How this file works

- **Lock Type** is `Hard` or `Soft`.
  - **Hard** - a settled invariant. Changing it requires the unlock process and ripples through downstream docs/code.
  - **Soft** - a current default that may still move with less ceremony; still recorded so it is not re-decided by accident.
- **Unlock process** (Enforcement Rule 4): the developer states the reason -> the agent flags the downstream impact -> the change is made, documented, and re-locked in the same session.
- **One decision per row.** Keep the "Locked Value / State" cell self-contained; link to a design doc for the full spec rather than inlining it.
- Group decisions under domain headings (suggested starters below); add or remove sections to fit the project.

---

## Technical Stack

| Item                 | Locked Value / State                                                   | Lock Type |
|----------------------|------------------------------------------------------------------------|-----------|
| Language             | C++23                                                                  | Hard      |
| Base library         | raylib 5.x (window, input, audio, 3D rendering)                        | Hard      |
| Build tooling        | CMake + FetchContent                                                   | Hard      |
| Target platform      | Windows, built with MinGW-w64 (C++23 rules out MSVC)                   | Hard      |
| Dev environment      | WSL2; cross-compile to Windows and run the .exe on the host (ADE-22)   | Hard      |
| Testing              | GoogleTest + GoogleMock                                                | Hard      |
| Data / save format   | JSON via nlohmann/json                                                 | Hard      |
| Physics              | Box3D (erincatto/box3d), MIT, C17. Run single-threaded for determinism | Hard      |
| CI                   | Not yet chosen - see Open Questions                                    | Soft      |

---

## Architecture

Full specification: [runtime_architecture.md](../design/runtime_architecture.md).

| Item              | Locked Value / State                                                                                       | Lock Type |
|-------------------|------------------------------------------------------------------------------------------------------------|-----------|
| Layer boundary    | `game -> engine -> platform`, one way, enforced by directory structure                                     | Hard      |
| Entity model      | Object-oriented hierarchy; context injected via `update(dt, Rng&, EventQueue&)`; entities never present    | Hard      |
| System ordering   | Explicit ordered calls in `World::tick`; ordering never lives inside an entity                             | Hard      |
| Event dispatch    | Immediate, with three mandatory guardrails: static registration, mark-dead/sweep, reentrancy depth guard   | Hard      |
| Simulation rate   | Fixed 60 Hz (`SIM_TICK_HZ`); presentation uncapped and interpolated                                        | Hard      |
| Frame budget      | `FRAME_BUDGET_MS` 6.94; `MAX_CATCHUP_TICKS` 5. A change that blows one halts and is flagged                | Hard      |
| Save model        | Checkpoint at level boundaries; versioned; no mid-level world state                                        | Hard      |
| Level format      | Brush lists in JSON; brushes become static Box3D colliders at load                                         | Hard      |
| Physics stepping  | Box3D stepped once per fixed tick at `SIM_TICK_SECONDS`, single-threaded (worker count 1)                  | Hard      |
| Transform owner   | Physics-authoritative: Box3D owns position and velocity; entities hold an opaque body handle               | Hard      |

---

## Workflow & Tooling

| Item                     | Locked Value / State                                                                                                                                                                                                                           | Lock Type |
|--------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-----------|
| Issue tracker            | Linear, team key `ADE`. Every tracked session maps to a Linear issue (`ADE-<number>`).                                                                                                                                                         | Hard      |
| Version-control workflow | `main` is the protected trunk; all work branches off the long-lived `dev` integration branch; `dev -> main` is a separate, maintainer-only promotion. Model: [core_protocol.md](../protocol/core_protocol.md#branching-model-dev-integration). | Hard      |
| Branch naming            | Off `dev`: `feature/ADE-<number>` for any non-bug work, `bugfix/ADE-<number>` for a fix; `ADE-<number>` is the Linear issue id.                                                                                                                | Hard      |

---

## Product / Platform Fundamentals

| Item         | Locked Value / State                                                                                                                            | Lock Type |
|--------------|-------------------------------------------------------------------------------------------------------------------------------------------------|-----------|
| Product      | `doomer` - a desktop first-person shooter with a Diablo-like loot loop. Linear project `adencraft`                                              | Hard      |
| Actors       | 2D billboard sprites, not 3D models (Doom's own architecture, matching available sprite content)                                                | Hard      |
| Audio        | In scope for MVP - SFX and music, behind the platform seam                                                                                      | Hard      |
| Asset policy | Loader targets the WAD/lump **format**. Local Doom data in `resources/` is gitignored and never shipped; FreeDoom is the distribution asset set | Hard      |
| Non-goals    | Multiplayer / networking; procedural level generation; level editor tooling (deferred, with an early spike)                                     | Soft      |

---

## Open Questions

Unsettled items that are not yet locked. Convert each into a tracked work item before session close (Enforcement Rule 6) - do not leave open-ended prose here long-term.

- **CI provider not chosen** - the framework assumes CI gates block merges ([core_protocol.md](../protocol/core_protocol.md#pr-checklist)), but no provider is selected. Needs a decision before the first PR.
- **The game's real frame budget is unmeasured.** ADE-22 proved the *baseline* is negligible (0.4 ms avg for a trivial scene), not that a populated level holds 6.94 ms. Re-measure once sprites, physics, and a HUD are in a level.
- **`ADE-21` mapping is underscoped** - the brush level format means the map must be derived from geometry rather than read from a grid. Rescope before the issue is picked up.
- **Level authoring ergonomics** - brush lists are hand-authored JSON. Spike after roughly three levels exist to decide whether editor tooling is needed.

# doomer

A desktop first-person shooter with a Diablo-like loot loop, written in C++23.

Fast movement and shooting in the Doom tradition - 3D level geometry, 2D billboard actors - with randomised items, affixes and rarity tiers layered on top.

**Status: early.** The engine foundations are built and tested; the game is not yet playable. See [What works today](#what-works-today).

---

## Stack

| ---------------------------------------------------------------------| |
|--------------------------------------------------------------------|---|
| Language                                                       | C++23 |
| Windowing, input, audio, rendering | [raylib](https://www.raylib.com/) |
| Physics | [Box3D](https://github.com/erincatto/box3d)                  |
| Data and saves | [nlohmann/json](https://github.com/nlohmann/json)     |
| Tests | GoogleTest + GoogleMock                                        |
| Build | CMake + FetchContent                                           |
| Target | Windows, cross-compiled with MinGW-w64                        |

Every dependency is fetched by CMake. There is nothing to install by hand except the toolchain.

---

## Building

### Prerequisites

```bash
# Arch
sudo pacman -S --needed mingw-w64-gcc cmake ninja

# Debian / Ubuntu
sudo apt install mingw-w64 cmake ninja-build
```

You also need a host compiler supporting C++23 for the test build (GCC 14 or newer).

### Three presets

| Preset | What it builds                                              | When |
|---------------------------------------------------------------------|---|---|
| `linux-test` | The test suite, headless                      | Every change |
| `windows-debug` | `doomer.exe` with asserts and the debug view | Day-to-day |
| `windows-release` | `doomer.exe`, shipping configuration         | Releases |

```bash
# Run the tests
cmake --preset linux-test
cmake --build --preset linux-test
ctest --preset linux-test

# Build the game
cmake --preset windows-debug
cmake --build --preset windows-debug
```

The test build **never fetches raylib**. That is deliberate: a test that tries to open a window fails to configure rather than passing locally and breaking in CI.

### Running it from WSL

```bash
cp build/windows-debug/src/doomer.exe /mnt/c/Users/<you>/doomer/
/mnt/c/Windows/System32/cmd.exe /c "cd /d C:\Users\<you>\doomer && doomer.exe"
```

---

## Debug controls

| Key | Does                                                             |
|--------------------------------------------------------------------|---|
| F1 | Toggle the debug view                                             |
| F2 | Cycle wireframe / solid / both                                    |
| F3 | Toggle the Box3D solver overlay - real contact points and normals |

The debug view is on in a debug build and off in a release build.

---

## What works today

Being specific, because "early" covers a lot of ground:

- **Fixed-step frame loop** at 60 Hz, with interpolated presentation and a catch-up cap
- **Platform seam** - window, input, audio, filesystem and clock behind interfaces, all mockable
- **Physics** - Box3D wrapped behind opaque generational handles, stepped once per tick, deterministic
- **Event dispatch** - immediate, with static registration, deferred removal and a reentrancy guard
- **Data model and saves** - versioned JSON, seeded PCG32 so a loaded run replays identically
- **Debug renderer** - physics bodies as wireframes, plus the solver's own contact view

**Not yet:** a player you can move, weapons, enemies, loot, levels, textures, lighting, or a HUD. The window currently shows falling spheres landing on a floor.

Planned work is tracked in [issues](https://github.com/AdenPotato/raydoomer/issues), grouped under epics.

---

## Assets and licensing

The game loads Doom's **WAD format**, not Doom's content.

`resources/` holds commercial id Software data used for local prototyping only. It is gitignored, has never been committed, and must never ship. [FreeDoom](https://freedoom.github.io/) is the distribution asset set - the loader targets the format, so swapping between them needs no code change.

Verifying that swap gates any release.

---

## Documentation

| --------------------------------------------------------------------------------------------------| |
|-------------------------------------------------------------------------------------------------|---|
| [Runtime architecture](docs/design/runtime_architecture.md) | How the game is put together, and why |
| [Locked decisions](docs/reference/locked_decisions.md)     | Settled choices and the open questions |
| [Protocols](docs/protocol/) | Engine, gameplay, content, presentation, testing                      |
| [Glossary](docs/reference/glossary.md)                                          | Shared vocabulary |

## Development process

This repository carries a version-controlled operating framework for working with an AI coding agent - session protocols, role agents, scaffolding skills, and a hard test-first rule. It is documented in **[README_AI.md](README_AI.md)** and [DEVELOPMENT.md](DEVELOPMENT.md).

The short version: all new behaviour is test-first, `main` is protected, work branches off `dev`, and every PR passes three blocking CI gates.

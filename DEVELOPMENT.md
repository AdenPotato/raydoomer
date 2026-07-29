# Development

How to drive this framework with **Claude Code** or **Windsurf (Cascade)** from a locally cloned repo. You only need one of the two; the steps are independent.

This document covers operating the *framework* (the rules, skills, workflows, and shared docs). Your project's own app run/build steps belong in that project's README or its own setup notes.

---

## Prerequisites

- **git**.
- **One** of:
  - **Claude Code** - the `claude` CLI, the desktop app, or an IDE extension (VS Code / JetBrains).
  - **Windsurf** - the Windsurf editor (Cascade is built in). No MCP servers required; the layer uses only the built-in terminal and file editing.

---

## 1. Clone and open

```bash
git clone <this-repo-url>
cd <repo>
```

Open the clone as your editor **workspace**. Each editor discovers its instruction layer at the **repository root**:

- Claude Code reads [`CLAUDE.md`](CLAUDE.md) and the [`.claude/`](.claude) directory.
- Windsurf reads the [`.windsurf/`](.windsurf) directory.

Both resolve the shared [`docs/`](docs) tree through relative links, so the docs must sit at the repo root (they do here). That requirement is the whole reason the layout matters when you share the framework across projects - see [Section 4](#4-sharing-the-framework-across-projects).

---

## 2. Using Claude Code

### How it discovers the framework

| File / dir         | Role                                                                            |
|--------------------|---------------------------------------------------------------------------------|
| `CLAUDE.md` (root) | Auto-loaded into every session as the standing behavior contract.               |
| `.claude/skills/<name>/SKILL.md` | Ordered checklists, invoked explicitly with `/skill-name`.        |
| `.claude/agents/`  | Role subagents (`simulation`, `presentation`, `content`), dispatched as needed. |

### The session loop

1. Run `claude` in the repo (or open the repo in your IDE with the Claude Code extension).
2. `/session-start` - syncs, reads the protocol context, and helps you pick the work.
3. Do the work test-first; delegate per surface to the role agents.
4. Scaffold new units with the skills: `/new-system`, `/new-hud`, `/engine-api-change`, `/data-change`, `/new-work-item`, `/new-design`.
5. `/session-end` - summary, changelog entry, commit, PR.

### Verify the setup

Type `/` in Claude Code and confirm the framework skills (`session-start`, `new-system`, …) appear. Running `/session-start` and getting the protocol checklist back confirms `CLAUDE.md` and `.claude/` are being read.

> **If a skill is missing or `/session-start` returns "Unknown skill":** check the layout first. Claude Code discovers project skills as `.claude/skills/<name>/SKILL.md` folders. A flat `.claude/skills/<name>.md` file is silently ignored - there is no warning, the skill just never registers. Note that moving a skill into a folder adds a directory level, so any relative link inside it needs one more `../` to still resolve.

### Machine-wide option

To use the framework in every repo on your machine without copying it, the same files can live under `~/.claude/` (user-global). The shared `docs/` still needs to be resolvable on disk - see [Section 4](#4-sharing-the-framework-across-projects).

More detail: [.claude/README.md](.claude/README.md).

---

## 3. Using Windsurf (Cascade)

### How it discovers the framework

| File / dir                                          | Role                                                                                |
|-----------------------------------------------------|-------------------------------------------------------------------------------------|
| `.windsurf/rules/core.md`                           | `trigger: always_on` - applied to every message.                                    |
| `.windsurf/rules/{simulation,presentation,content}.md` | `trigger: model_decision` - loaded when Cascade judges the description relevant. |
| `.windsurf/workflows/*.md`                          | Invoked manually with `/session-start`, `/session-end`, `/promote`.                 |
| `.windsurf/skills/<name>/SKILL.md`                  | Auto-invoked on a description match, or called with `@name`.                        |

### The session loop

1. Open the repo folder in Windsurf.
2. `/session-start` (a workflow) - same start checklist.
3. Do the work test-first; the role rules surface as Cascade touches each surface.
4. Scaffold with the skills: `@new-system`, `@new-hud`, `@engine-api-change`, `@data-change`, `@new-work-item`, `@new-design` (or let Cascade auto-invoke them when a request matches).
5. `/session-end` (a workflow) - summary, changelog, commit, PR.

### Verify the setup

In Cascade, type `/` to list the workflows and `@` to list the rules and skills. Seeing `session-start` under `/` and the `new-*` items under `@` confirms `.windsurf/` is being read.

### Tuning

- The role rules ship as `model_decision` because the template's folder layout is still placeholders. Once your source paths are fixed, switch a role rule to `trigger: glob` + `globs: <paths>` so it auto-activates on those files.
- Keep `rules/core.md` lean (rule files cap at ~12,000 characters); detail belongs in `docs/`, which the rule points to.
- Machine-wide option: global workflows/skills live under `~/.codeium/windsurf/` - the shared `docs/` still needs to resolve on disk ([Section 4](#4-sharing-the-framework-across-projects)).

More detail: [.windsurf/README.md](.windsurf/README.md).

---

## 4. Sharing the framework across projects

AI editors read the **workspace filesystem** - there is no native "include these docs from another repo over the network." So when you use this framework in a *different* code project, the instruction layer must sit at that project's root and `docs/` must be reachable where the relative links expect it (`./docs`). Pick one:

| Approach              | What you do                                                          | Trade-off                                                  |
|-----------------------|----------------------------------------------------------------------|------------------------------------------------------------|
| **Copy / vendor**     | Copy the framework into the project root.                            | Simplest; copies drift over time.                          |
| **git subtree**       | `git subtree add/pull` the framework into the project root.          | One upstream, real files in the tree, easy updates.        |
| **Submodule `docs/`** | Keep the thin layer at root; add `docs/` as a submodule at `./docs`. | One source for the heavy docs; relative links unchanged.   |
| **Symlink `docs/`**   | Symlink `docs/` from a local clone of this repo.                     | Fine solo on one machine; fragile on Windows / CI / teams. |

**Recommended:** keep this repo as the upstream source of truth and add **`docs/` as a git submodule at `./docs`** in each consuming project, with the thin instruction layer copied (or subtree'd) to the root. Because `docs/` still lands at the project root, none of the relative links change, and `git submodule update --remote` pulls doc updates everywhere.

What does **not** work: pointing links at GitHub raw URLs (the editor will not auto-load them into context), absolute machine paths (`/Users/you/...` breaks for teammates and CI), or leaving the docs only in this repo and expecting the editor to find them (they are outside the workspace).

---

## 5. Customize for your project

Once the framework is in place, fill in the blanks: the placeholders in [CLAUDE.md](CLAUDE.md) and the template files, your stack in [locked_decisions.md](docs/reference/locked_decisions.md), and your real source layout in [core_protocol.md - Folder Structure](docs/protocol/core_protocol.md#folder-structure). The full checklist is the **Using it in a project** section of the [README.md](README.md).

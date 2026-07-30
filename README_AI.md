# Coding Agent Operating Framework

A reusable, project-agnostic operating framework for working with an AI coding agent in a codebase. It is a set of version-controlled rules, protocols, skills, and role agents that give the agent a consistent, test-first way of working on any project - drop it into a repo, fill in the project-specific blanks, and start running sessions.

It supports **Claude Code** and **Windsurf (Cascade)** out of the box: the canonical content lives in one shared [`docs/`](docs) tree, and each editor gets a thin, native instruction layer that points into it - so the two never fork.

The framework is deliberately stack-agnostic: it encodes *how* to work (session discipline, TDD, branching, an API contract seam, locked decisions) without assuming *what* you build it with.

## Purpose

Working with an AI agent across many sessions tends to drift - conventions get re-litigated, decisions get forgotten, quality bars slip. This repo fixes that by writing the working agreement down where the agent reads it every session:

- A predictable **session lifecycle** (start -> work -> end) with explicit checklists.
- **Test-first development** as a hard rule, per layer.
- A **protected-branch workflow** so shared history is never rewritten.
- A **locked-decisions registry** so settled choices are not re-opened by accident.
- Per-surface **role agents** and **scaffolding skills** so a unit of work is added the same way each time.

## What's inside

| Path                              | What it holds                                                                                                                                              |
|-----------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [docs/](docs)                     | The canonical, editor-neutral framework - protocols, reference, design specs, changelog. The single source both editors read.                              |
| [CLAUDE.md](CLAUDE.md)            | **Claude Code:** auto-loaded every session - behavior rules, response style, git-safety guarantees, TDD rule.                                              |
| [.claude/skills/](.claude/skills) | **Claude Code:** ordered, test-first checklists invoked with `/skill-name`; one `<name>/SKILL.md` folder each.                                             |
| [.claude/agents/](.claude/agents) | **Claude Code:** role subagents - `simulation`, `presentation`, `content` - each owning one surface.                                                       |
| [.windsurf/](.windsurf)           | **Windsurf (Cascade):** the native twin - rules, workflows, and skills - all pointing at the same `docs/`. See [.windsurf/README.md](.windsurf/README.md). |

Full documentation map: [docs/README.md](docs/README.md).

## Editor support

Both editors drive the *same* framework; only the thin instruction layer differs. The Windsurf layer mirrors the Claude one onto Windsurf's native primitives:

| Concept                | Claude Code       | Windsurf (Cascade)                                                 |
|------------------------|-------------------|--------------------------------------------------------------------|
| Always-on behavior     | `CLAUDE.md`       | `.windsurf/rules/core.md` (`always_on`)                            |
| Per-surface role rules | `.claude/agents/` | `.windsurf/rules/{role}.md` (`model_decision`)                     |
| Session ceremonies     | `.claude/skills/<name>/SKILL.md` | `.windsurf/workflows/` (manual `/slash`)            |
| Scaffolding procedures | `.claude/skills/<name>/SKILL.md` | `.windsurf/skills/<name>/SKILL.md` (auto / `@name`) |
| Shared docs            | `docs/`           | `docs/` (same files)                                               |

Each layer has its own README with the editor-specific detail ([.claude/README.md](.claude/README.md), [.windsurf/README.md](.windsurf/README.md)); the reasons behind the Windsurf workflow-vs-skill split live in the latter. Using only one editor? Delete the other layer - `docs/` stands on its own.

## Using it in a project

For step-by-step setup with each editor (and how to share the framework across repos), see [DEVELOPMENT.md](DEVELOPMENT.md).

1. Copy this repo's contents into your project (or use it as a template), keeping the editor layer(s) you use.
2. Fill in the placeholders - `<project-name>` in [CLAUDE.md](CLAUDE.md), the attribution trailer, and the `<...>` slots in the template files ([locked_decisions.md](docs/reference/locked_decisions.md), [glossary.md](docs/reference/glossary.md), [style_guide.md](docs/design/style_guide.md), [integrations.md](docs/reference/integrations.md)).
3. Record your actual source-tree layout in [core_protocol.md - Folder Structure](docs/protocol/core_protocol.md#folder-structure) and your stack choices in [locked_decisions.md](docs/reference/locked_decisions.md).
4. (Windsurf) Once your source paths are fixed, consider switching the role rules from `model_decision` to `glob` so they auto-activate on the matching files - see [.windsurf/README.md](.windsurf/README.md).
5. Start a session (`/session-start`) and go.

## Core rules at a glance

- **Test-first.** A failing test that captures the behavior exists and is shown failing before any implementation.
- **`main` is protected.** Work branches off `dev` as `feature/<issue>-<slug>` or `bugfix/<issue>-<slug>` (the GitHub issue number); `dev -> main` is a separate, maintainer-only promotion.
- **Locked decisions are canon.** Work that contradicts one halts until the decision is unlocked, changed, documented, and re-locked.
- **Never rewrite shared history.** The agent only fast-forward pushes; force-push and history rewrites need explicit approval.

The full working agreement is in [CLAUDE.md](CLAUDE.md) (Claude Code) and [.windsurf/rules/core.md](.windsurf/rules/core.md) (Windsurf).

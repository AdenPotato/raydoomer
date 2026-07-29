# Windsurf layer

The Windsurf (Cascade) instruction layer. It is a thin set of pointers into the shared [`docs/`](../docs) tree - the canonical, editor-neutral framework. Nothing here duplicates `docs/`; these files only tell Cascade *how* to work and route it to the right protocol.

For how this layer relates to the other editor layer(s), see the **Editor support** section of the root [README.md](../README.md).

## What's here

| Path                                      | Windsurf primitive      | Holds                                                                                                |
|-------------------------------------------|-------------------------|------------------------------------------------------------------------------------------------------|
| `rules/core.md`                           | Rule (`always_on`)      | Condensed always-on behavior rules; links out to the protocols.                                      |
| `rules/{frontend,backend,integration}.md` | Rule (`model_decision`) | Per-surface role context, loaded on demand by description.                                           |
| `workflows/*.md`                          | Workflow (`/slash`)     | Deliberate session ceremonies: `/session-start`, `/session-end`, `/promote`.                         |
| `skills/<name>/SKILL.md`                  | Skill (auto / `@name`)  | `new-work-item`, `new-design`, `new-contract`, `new-endpoint`, `new-screen`, `db-change`, `write-story`, `pr-summary`. |

## Requirements

**No MCP servers required.** This layer uses only Cascade's built-in terminal and file editing - nothing here depends on an MCP server or any external tool integration. The one step that *benefits* from extra tooling, the UI smoke check in the `new-screen` skill, degrades to running the app manually when no editor preview/browser tooling is available; the failing-then-green test is the hard requirement either way.

## How Cascade consumes these

- **Rules** (`.windsurf/rules/*.md`) declare activation via a `trigger:` frontmatter field: `always_on`, `model_decision` (only the `description:` is loaded until Cascade judges it relevant), `glob` (+ a `globs:` pattern), or `manual` (`@rule-name`). Workspace rule files cap at ~12,000 characters - thin pointers stay well under.
- **Workflows** (`.windsurf/workflows/*.md`) are invoked `/name` (manual-only - Cascade never auto-runs them), cap at ~12,000 characters each, and may call other workflows.
- **Skills** (`.windsurf/skills/<name>/SKILL.md`) use `name` + `description` frontmatter with **progressive disclosure** (only name + description loaded until invoked), fire **automatically on a description match or `@name`**, and can carry supporting files. The `SKILL.md` format is the standardized, cross-tool one (also used by Claude Code and Cursor).

### Sharing one skill source with the Claude layer

Both layers use the same `<name>/SKILL.md` folder format, so a scaffold skill can be written once and shared (a symlink, or a build step that copies one into the other) rather than maintained twice. The **sets** are not identical: the session ceremonies live in `workflows/` here but in `skills/` on the Claude side, so only the scaffolds (`new-*`, `db-change`, `write-story`, `pr-summary`) overlap.

### Why ceremonies are Workflows and scaffolds are Skills

The session ceremonies are meant to be run **deliberately**, so they are Workflows (manual `/slash`, never auto-fired). The scaffolding procedures are things Cascade can reasonably reach for when a request matches, so they are Skills (auto-invoke or `@name`, progressive disclosure). Workflows reference a scaffold by its skill name (`@new-work-item`), not a slash command, since skills are not `/slash`-invoked.

## Tuning for your project

- **Role rules** use `trigger: model_decision` because the template's folder layout is still placeholders. Once your source paths are fixed, switch a role rule to `trigger: glob` + `globs: <your UI/service/contract paths>` so it activates automatically when Cascade reads or edits those files.
- **Always-on budget:** keep `rules/core.md` lean. Detail belongs in `docs/`, which the rule points to.
- Windsurf **Memories** (auto-persisted context) are out of scope for this framework; leave them to Cascade's defaults.

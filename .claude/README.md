# Claude Code layer

The Claude Code instruction layer. Like the Windsurf layer, it is a thin set of pointers into the shared [`docs/`](../docs) tree - the canonical, editor-neutral framework. These files tell Claude *how* to work and route it to the right protocol; they do not duplicate `docs/`.

The always-on entry point, [`CLAUDE.md`](../CLAUDE.md), lives at the repo root (Claude Code auto-loads it every session), not inside this directory.

For how this layer corresponds to the Windsurf layer (`.windsurf/`), see the **Editor support** section of the root [README.md](../README.md).

## What's here

| Path                                       | Claude Code primitive          | Holds                                                                                                                                                                                               |
|--------------------------------------------|--------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| [`../CLAUDE.md`](../CLAUDE.md)             | Project instructions (auto-loaded) | Behavior rules, response style, git-safety, TDD rule; the always-on layer.                                                                                                                      |
| `agents/{simulation,presentation,content}.md` | Subagent (Agent tool)          | Per-surface role agents; each owns one surface and defers to its protocol.                                                                                                                       |
| `skills/<name>/SKILL.md`                   | Skill (`/skill-name`)          | Ordered, test-first checklists: `session-start`, `session-end`, `promote`, `new-work-item`, `new-design`, `new-system`, `new-hud`, `engine-api-change`, `data-change`, `write-story`, `pr-summary`. |

## How Claude Code consumes these

- **CLAUDE.md** is auto-loaded into every session as the standing behavior contract.
- **Agents** are subagents dispatched via the Agent tool. Each runs as a scoped child with its own context, owns one surface (UI / service + data / the contract seam), and defers to its `*_protocol.md` rather than restating it.
- **Skills** are invoked explicitly with `/skill-name`. Each is an ordered checklist (failing test first, then implement to green), with `name` + `description` frontmatter.

## Notes

- **Each skill must be a `<name>/SKILL.md` folder.** Claude Code discovers project skills by directory. A flat `skills/<name>.md` file is **silently ignored** - no warning, no error, the skill simply never registers and `/skill-name` returns "Unknown skill". If a skill does not appear when you type `/`, check the layout before anything else.
- This matches the `<name>/SKILL.md` format the Windsurf layer already uses, so a single skill file can be shared between the two layers ([.windsurf/README.md](../.windsurf/README.md)). The **sets** still differ: Windsurf splits session ceremonies into `workflows/`, so its `skills/` holds only the scaffolds.
- Unlike Windsurf, Claude Code does not split these into "workflows" vs "skills" - all eleven are `/skill-name` checklists; the ceremony-vs-scaffold distinction only matters on the Windsurf side.

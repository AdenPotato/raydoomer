# docs

Index of the project documentation. Repo overview is the root [README.md](../README.md); how to drive the framework with Claude Code or Windsurf is [DEVELOPMENT.md](../DEVELOPMENT.md). A consuming project keeps its own app run/build setup in its own README.

This page is the **navigation map** (what each doc/folder is). Per-file **update rules** are the [File Inventory](protocol/session_protocol.md#file-inventory) in session_protocol.md - the single source for *when* to touch each doc.

| Path                    | What's here                                   |
|-------------------------|-----------------------------------------------|
| [protocol/](protocol)   | How we work and how we build - the protocols. |
| [reference/](reference) | Canonical reference material.                 |
| [design/](design)       | Product and technical design specs.           |

## protocol/

| File                                                        | Purpose                                                                                       |
|-------------------------------------------------------------|-----------------------------------------------------------------------------------------------|
| [session_protocol.md](protocol/session_protocol.md)         | Session lifecycle, enforcement rules, file inventory.                                         |
| [core_protocol.md](protocol/core_protocol.md)               | Shared dev core: TDD, branching, folder structure, naming, code quality, commits, docstrings. |
| [frontend_protocol.md](protocol/frontend_protocol.md)       | UI / client surface rules.                                                                    |
| [backend_protocol.md](protocol/backend_protocol.md)         | Service + data layer rules.                                                                   |
| [integration_protocol.md](protocol/integration_protocol.md) | The API seam between services and consumers.                                                  |
| [engine_protocol.md](protocol/engine_protocol.md)           | Engine-code rules: the game/engine boundary, resource lifetime, platform seam, frame budget.  |
| [gameplay_protocol.md](protocol/gameplay_protocol.md)       | Gameplay-code rules: tunables in data, feel requests, frame-rate independence.                |
| [game_test_protocol.md](protocol/game_test_protocol.md)     | Test-first for game code: the seams, time control, golden images, the feel exception.         |
| [qa_protocol.md](protocol/qa_protocol.md)                   | Manual QA workflow + environment matrix (template).                                           |

## reference/

| File                                                 | Purpose                                                             |
|------------------------------------------------------|---------------------------------------------------------------------|
| [locked_decisions.md](reference/locked_decisions.md) | The locked-decisions registry (the canon) - template.               |
| [glossary.md](reference/glossary.md)                 | Shared vocabulary - template.                                       |
| [integrations.md](reference/integrations.md)         | External-service setup runbook (env vars per service) - template.   |
| [authoring.md](reference/authoring.md)               | Linear issue + PR summary templates.                                |

## design/

| File                                    | Purpose                                   |
|-----------------------------------------|-------------------------------------------|
| [style_guide.md](design/style_guide.md) | Visual language / style guide - template. |

## changelog

| File                         | Purpose                             |
|------------------------------|-------------------------------------|
| [changelog.md](changelog.md) | Rolling session log (20-entry max). |

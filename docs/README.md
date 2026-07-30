# docs

Index of the project documentation. The project overview is the root [README.md](../README.md); the agent operating framework is [README_AI.md](../README_AI.md), and how to drive it is [DEVELOPMENT.md](../DEVELOPMENT.md).

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
| [presentation_protocol.md](protocol/presentation_protocol.md)                                                 | Render, camera, sprites, HUD, audio output. |
| [content_protocol.md](protocol/content_protocol.md)                                                   | Content definitions, loaders, resources, save data. |
| [engine_api_protocol.md](protocol/engine_api_protocol.md)                                                              | Changing the engine/game boundary. |
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
| [authoring.md](reference/authoring.md)               | GitHub issue + PR summary templates.                                |

## design/

| File                                    | Purpose                                   |
|-----------------------------------------|-------------------------------------------|
| [style_guide.md](design/style_guide.md) | Visual language / style guide - template. |

## changelog

| File                         | Purpose                             |
|------------------------------|-------------------------------------|
| [changelog.md](changelog.md) | Rolling session log (20-entry max). |

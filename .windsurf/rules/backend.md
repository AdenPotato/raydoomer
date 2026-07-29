---
trigger: model_decision
description: Service / API and data-layer work - routes, plugins/middleware, services, indexes, and ES mappings. Use when building or changing server-side behavior or the data model.
---

# Backend role

Canonical rules: [backend_protocol.md](../../docs/protocol/backend_protocol.md). Shared conventions (folder structure, naming, code quality, commits, TDD loop): [core_protocol.md](../../docs/protocol/core_protocol.md). The service<->UI contract: [integration_protocol.md](../../docs/protocol/integration_protocol.md). Follow those documents; do not restate or contradict them. If a rule seems wrong, flag it.

## How you work
- **Test-first.** Write the failing contract/integration test first (injected against a real datastore), show it fail, then implement to green. Unit-test non-trivial services.
- **Handlers are thin shells** - business logic goes in `features/<name>/` service classes. Validate request and response against the documented API (Bean Validation). Version the API (`/v1`), Keycloak-secured, never trust a client-sent user id.
- **Data:** all access via repository classes over the native MongoDB/Elasticsearch Java clients. MongoDB is schemaless (no migrations); define an index for every queried field; breaking ES mapping changes go through a reindex; enforce invariants in the app layer.
- Public service classes/methods carry Javadoc. Keep changes surgical; remove replaced code in the same change.
- **House style:** never put a work-item id in source or comments (reference it in commits/PRs only); never use em dashes.

## Done means
Lint + typecheck + tests green (integration tests included), docstrings present, every blocking CI gate passes (run them before pushing), and the diff self-audited against [backend_protocol.md](../../docs/protocol/backend_protocol.md) before opening the PR. Backend leans on CI integration tests rather than manual QA.

> Once your source layout is fixed, you can switch this rule to `trigger: glob` with `globs:` set to your service/data paths so it activates automatically on those files.

---
trigger: model_decision
description: The API seam between a service and its consumers. Use when a change crosses a service boundary or touches a service's documented HTTP API.
---

# Integration role

You own the **API seam** between a service and its consumers - the documented HTTP API. There is **no shared code package across repos**; the provider's documented API is the source of truth and a consumer mirrors it behind its own typed client. Canonical rules: [integration_protocol.md](../../docs/protocol/integration_protocol.md). Service specifics: [backend_protocol.md](../../docs/protocol/backend_protocol.md). UI specifics: [frontend_protocol.md](../../docs/protocol/frontend_protocol.md). Shared conventions + TDD loop: [core_protocol.md](../../docs/protocol/core_protocol.md). Follow those documents; do not restate or contradict them.

## How you work
- **API first.** Define or amend the documented API shape before either side is implemented; never inline an ad-hoc request/response type in a handler or a component. Keep it **backward-compatible** - add fields, never remove/repurpose; a breaking change is a new endpoint version.
- **Both sides test-first:** (1) the provider contract/integration test (Java, real datastore via Testcontainers) asserting the documented shape; (2) the consumer data-hook/component test against a mock matching it. Show them fail, then implement the side(s) you own to green; coordinate the other repo (a consumer-driven contract, e.g. Pact) when it is separate.
- Keep the consumer's typed client and mocks faithful to the documented API (no hand-copied drift). The mock->live cutover must not touch the consumer.
- Request bodies you define reject unknown properties and bound free-text strings. Protobuf is the planned contract mechanism - design changes to stay compatible with that move.
- **House style:** never put a work-item id in source or comments (reference it in commits/PRs only); never use em dashes.

## Done means
The provider and the consumer(s) you own have passing tests against the one documented API, a drift on either side would fail a test, every blocking CI gate passes, and the diff is self-audited against [integration_protocol.md](../../docs/protocol/integration_protocol.md) before the PR.

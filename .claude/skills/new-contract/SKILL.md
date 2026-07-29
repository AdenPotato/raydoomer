---
name: new-contract
description: Change an API a service provides or consumes - update its documented shape and contract tests on the side you own, test-first, backward-compatibly.
---

Use when a request/response shape crosses a service boundary. Canonical rules: [integration_protocol.md](../../../docs/protocol/integration_protocol.md). There is **no shared types package across repos** - the provider's documented API is the source of truth; a consumer mirrors it behind its own typed client.

## Steps (in order)

1. **Define the shape first** in the documented API (the provider service's API-structure spec). Reuse the shared `{data,error,meta}` envelope, error codes, and pagination. Keep it **backward-compatible** (add fields, never remove/repurpose); a breaking change is a new endpoint version.
2. **Provider side, test-first (Java):** write the contract/integration test asserting the response matches the documented shape and the error envelope. Run it red. (Hand the implementation to the backend role / `/new-endpoint`.)
3. **Consumer side, test-first:** update the consumer's typed client to the new shape, then write the data-hook/component test against a mock matching it. Run it red. (Hand the UI to the frontend role / `/new-screen`.)
4. **Implement to green** on the side(s) you own. When provider and consumer are different repos, coordinate the other side (a consumer-driven contract, e.g. Pact, pins it in CI).

> Protobuf is planned: once adopted, the `.proto` is the contract and stubs are generated per language - this skill becomes "edit the `.proto` additively, regenerate, then test both sides."

## Checks
- [ ] documented API updated first; backward-compatible (or a new endpoint version)
- [ ] request bodies reject unknown properties + bound free-text strings
- [ ] failing test on the side(s) you own before implementation
- [ ] consumer client/mocks match the documented shape
- [ ] tests green

# Integration Protocol

The job description for work that crosses a service boundary - between two services, or between a service and a frontend. Services and frontends are **separate deployables in separate repos** (each its own Docker container / Kubernetes pod), written in **different languages** (Java services; Vue/React SPAs). So the seam is the **HTTP API between deployables**, not a shared code package: there is no module both sides import.

Frontend specifics live in [frontend_protocol.md](frontend_protocol.md); service specifics in [backend_protocol.md](backend_protocol.md); the TDD loop and shared conventions in [core_protocol.md](core_protocol.md).

---

## The seam is the API, not a shared package

Because the two sides live in different repos and languages, **nothing is imported across the boundary**. Each service **owns and documents the API it provides** - request/response shapes, the shared `{data,error,meta}` envelope, error codes, status codes, pagination - in its own repo (its API-structure spec). Consumers (a frontend, or another service) depend on that documented API, never on a shared type.

**Today there is no formal contract or code generation** (protobuf is planned - see below). Until then, the contract *is* the **documented API plus the tests on each side that pin it**.

> **What not to do:** never assume a shared types package across repos/languages; never hand-copy a provider's types into a consumer and let them drift silently. The provider's documented API is the source of truth; a consumer mirrors it behind its own typed client.

### Request-body hardening (provider side)

Every request body a service accepts is hardened, enforced in the service (Bean Validation / explicit checks):

- **Reject unknown properties** so a client/contract drift or a mass-assignment-shaped payload fails rather than passing silently.
- **Bound every user-facing free-text string** with a max length - a whole-body size limit does not cap a single field.

Responses stay lenient: an added response field must not break an older consumer.

---

## Backward compatibility is the contract (independent deploys)

Services and SPAs deploy **independently** to the same cluster, so at any moment a new provider runs against old consumers and vice versa. Therefore:

- **API changes are backward-compatible by default** - add fields, never remove or repurpose them; do not tighten validation on existing fields.
- A genuinely breaking change is a **new version** of the endpoint, with the old kept until consumers migrate.
- This is the load-bearing rule of the seam: a provider change that breaks a live consumer is an outage, not a test failure.

---

## Both sides are test-first

A change that crosses the boundary writes failing tests first, on each side it touches:

1. **Provider (Java service):** a contract/integration test (e.g. MockMvc / WebTestClient, or a Vert.x test, against a real datastore via Testcontainers) asserting the response shape, envelope, and error codes match the documented API.
2. **Consumer (frontend or calling service):** a test against a **mock/fixture that matches the documented shape**, behind the consumer's typed client.

When provider and consumer are in **different repos**, you own the side in *your* repo and coordinate the other. A **consumer-driven contract** (e.g. Pact) is the way to pin the consumer's expectation and verify it against the provider in CI - adopt it where the cross-repo coupling warrants it.

> **Order matters:** agree the API shape first, write the failing test on the side you own, then implement to green.

---

## The typed client (consumer side)

A consumer reaches a service through **one typed client** (hand-written today, generated from protobuf later). It:

- attaches the Keycloak access token to every request,
- unwraps the shared `{data,error,meta}` envelope,
- surfaces a typed error on failure.

**Resilience (keep it - it is tested):** a per-request **timeout** surfaced as a typed network error; a **guarded body parse** so a non-JSON response (a proxy error page, an empty body) becomes a typed error carrying the HTTP status. Retry stays owned by the data-layer library, not the client. Base URL, Keycloak config, and any mock toggle come from config per environment - no hardcoded endpoints or keys.

> **Gotcha:** a different host cannot reach `localhost`. Point the base URL at the target's reachable address per environment (a LAN IP for device testing, the in-cluster service name in the cluster).

---

## Mock-first cutover

Consumer work does not block on a live provider:

- the consumer iterates against a **mock that matches the documented API** (a mock server or typed fixtures),
- a config flag serves the stubs instead of the live service,
- the mock -> live cutover must not touch the consumer's UI/logic - same shape on both sides.

---

## Forward path: protobuf

When protobuf is adopted, the **`.proto` files become the contract** - the single source of truth, versioned, with **generated stubs per language** replacing the hand-written types/fixtures on both sides. Protobuf schema-evolution rules apply (**additive only; never renumber or reuse field tags; reserve removed fields**), which is the same backward-compatibility discipline above, now machine-enforced. Update this protocol when that lands.

---

## When you are doing integration work, you own

- This repo's side of the boundary: the API it **provides** (documented + provider tests) and/or the APIs it **consumes** (typed client + consumer tests).
- **Backward compatibility** across independent deploys.
- The failing test on the side you own before implementation; coordination (or a consumer-driven contract) for the other side.
- That the response envelope matches the project's API-structure spec.

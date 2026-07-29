# Backend Protocol

The job description for service and data work: the **service / API** and the **data layer**.

Cross-cutting rules (folder structure, naming, code quality, env vars, commits, TDD loop, branching, docstrings) live in the shared core [core_protocol.md](core_protocol.md). The service<->client API seam lives in [integration_protocol.md](integration_protocol.md).

> **Stack:** services are **Java** (Spring Boot or Vert.x) over **MongoDB + Elasticsearch** (native Java clients), secured by **Keycloak**. The rules below are written for that stack.

---

## Routes and handlers

- The service exposes **thin HTTP handlers** (controllers / routes); cross-cutting concerns (auth, datastore access) are **wired once** (Spring config / filters, or a Vert.x router) and available everywhere.
- **No business logic in the handler** - extract to a service class under `features/<name>/`. The handler validates, calls the service, and shapes the response.
- Handlers validate the request **and** the response against the **documented API** (Bean Validation / explicit checks). The response shape, error codes, the `{data,error,meta}` envelope, rate limiting, and request logging are defined once in the service's API-structure spec - follow it exactly; do not redefine per handler.
- **Version the API** (e.g. `/v1/`). Decide auth posture deliberately (default to auth-required; document any public endpoints explicitly).

> **What not to do:** do not put a query, a business-rule branch, or response assembly inside a route handler. If a route file grows past validate -> call service -> return, the logic belongs in `features/<name>/`.

### Auth guard

- Auth is the **Keycloak** resource-server integration: it validates the OIDC JWT (issuer, audience, signature via the JWKS) and exposes the authenticated identity. **Never trust a client-sent user id** - derive identity from the verified token.
- Elevated/admin-only endpoints sit behind an explicit role guard (Keycloak realm/client roles).

## Data access (MongoDB + Elasticsearch)

- All datastore access goes through **repository classes wrapping the native MongoDB / Elasticsearch Java clients**. No driver calls or raw connections scattered across controllers or service logic.
- **MongoDB is schemaless - there are no relational migrations.** The document shape is owned by the application; enforce it in the repository/service layer (and, where it matters, with a **MongoDB JSON Schema validator** on the collection). Reshaping or backfilling existing documents is an explicit, **idempotent** data task, not an auto-generated migration.
- **Indexes are explicit.** MongoDB does not index your query fields for you - define an index for **every field used in a filter, a sort, or a lookup**, and a **compound index** matching a multi-field query. Use a **unique index** to enforce uniqueness. A genuinely-unindexed hot path is a bug; a rarely-queried field left unindexed is fine, but say so in a comment.
- **Elasticsearch is a derived search index**, fed from the source of truth in MongoDB and reindexable from it. The **index mapping** (fields, types, analyzers) is explicit and version-controlled. An additive mapping change applies in place; a **breaking mapping change requires a planned reindex** (new index + alias swap), never an in-place edit.
- Keep index definitions and ES mappings in version control and apply them **idempotently** at startup or via a setup step.

> **What not to do:** never open a Mongo/ES connection outside a repository; never ship an unindexed query on a hot path; never make a breaking ES mapping change in place; never treat Elasticsearch as a system of record.

### Enforce invariants in the application layer

MongoDB has no `CHECK` constraints or foreign keys, so invariants live in code, test-first - **never leave one silently unenforced**:

- **Single-document rule** (a bound, a range, a not-self reference) -> validated in the repository/service before the write (optionally backed by a Mongo JSON Schema validator on the collection), with a test asserting the violating write is rejected with the expected error envelope.
- **Cross-document / cross-collection rule** (a count cap, a referential constraint, a uniqueness rule) -> a **unique index** for uniqueness; everything else an **app-layer guard in the service, test-first** (the rejected write returns the expected error envelope).
- **Write path not built yet** -> do **not** add a speculative guard; record the classification so it is explicit, and enforce it when the feature builds that path.

---

## Testing

Backend leans on CI integration tests (a real datastore, every PR) rather than manual QA, so the bar is concrete:

- **Integration tests required for every endpoint**, against a **real datastore** (Testcontainers MongoDB + Elasticsearch, not mocks). At minimum each endpoint covers the happy path.
- **Contract tests** assert responses match the **documented API** (prevent drift) - the provider side of the integration seam ([integration_protocol.md](integration_protocol.md)).
- **Unit tests** (JUnit) for service classes with non-trivial logic.
- Test files mirror the source structure (`src/test/java/...`).
- **Javadoc:** public service classes, methods, and interfaces carry doc comments.

**Contract test pattern (provider).** Stand up the app and assert the response matches the documented API - shape, status, and the `{data,error,meta}` envelope. In Spring Boot use **MockMvc** / **WebTestClient**; in Vert.x use its test client. Data-touching tests run against **Testcontainers** (MongoDB + Elasticsearch), not mocks.

```java
@SpringBootTest
@AutoConfigureMockMvc
class HealthEndpointTest {
  @Autowired MockMvc mvc;

  @Test
  void returnsBodyMatchingTheDocumentedApi() throws Exception {
    mvc.perform(get("/v1/health"))
       .andExpect(status().isOk())
       .andExpect(jsonPath("$.data.status").value("ok"));
  }
}
```

For endpoints behind the Keycloak guard, drive authenticated / unauthenticated cases with a test JWT (e.g. the `spring-security-test` `jwt()` post-processor, or a Testcontainers Keycloak). Identity comes from the verified token, never a client-sent id.

---

## Folder shape

The canonical service tree (entry point, thin HTTP handlers under `api/`, `features/` service classes, `repository/`) is in the shared core: [core_protocol.md - Folder Structure](core_protocol.md#folder-structure). The rule that matters here: handlers are thin shells, business logic is a service under `features/<name>/`, and request/response types are owned by this service (mirrored by consumers behind their own client), never inlined ad-hoc in handlers.

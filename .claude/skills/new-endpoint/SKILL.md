---
name: new-endpoint
description: Add a Java service endpoint (Spring Boot or Vert.x) with its documented API and a failing contract/integration test first.
---

Use to add a service endpoint. Canonical rules: [backend_protocol.md](../../../docs/protocol/backend_protocol.md). If the request/response shape is new, do `/new-contract` first.

## Steps (in order)

1. **Contract:** ensure the request + response are in the service's documented API (run `/new-contract` if not).
2. **Failing test first (Java):** write the contract/integration test against a real datastore (Testcontainers MongoDB + Elasticsearch) - happy path plus the auth case (Keycloak). Run it red.
3. **Thin handler** under the versioned path (`/v1/...`), Keycloak-secured: validate the request (Bean Validation / explicit checks), call a **service class** in `features/<name>/`, shape the `{data,error,meta}` response. Never trust a client-sent user id - derive identity from the verified token.
4. **Implement the service** to green. Add Javadoc to public methods.
5. If the data model / indexes / ES mappings changed, run `/db-change`.

## Checks
- [ ] failing contract/integration test before implementation
- [ ] handler thin; logic in a `features/<name>/` service; request + response validated
- [ ] versioned path, Keycloak-secured, identity from the verified token
- [ ] Javadoc present; tests green

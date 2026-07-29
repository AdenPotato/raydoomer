---
name: db-change
description: Use when changing the data model, MongoDB indexes, or Elasticsearch mappings. Adds the spec-driven integration test in the same change.
---

Change the data model - MongoDB documents/indexes or Elasticsearch mappings. Canonical rules: [backend_protocol.md](../../../docs/protocol/backend_protocol.md). This is the documented TDD exception (you cannot query a collection/index until it exists): write the spec-driven integration test in the **same** change and drive to green.

## Steps (in order)

1. **Update the model in the repository layer** - the document shape (and any MongoDB JSON Schema validator) and/or the Elasticsearch index mapping. MongoDB is schemaless: there is no relational migration.
2. **Define indexes / mappings explicitly** - an index for every field filtered, sorted, or looked up (compound where a query is multi-field; unique where uniqueness is required); ES mapping fields/types/analyzers. Apply them **idempotently** at startup or via a setup step - never rely on auto-indexing.
3. **Breaking ES mapping change?** Plan a **reindex** (new index + alias swap), reindexable from MongoDB; never edit a mapping in place.
4. **Spec-driven integration test** in the same change: assert the model's invariants and the index/mapping behavior against a **real datastore** (Testcontainers MongoDB + Elasticsearch). Reseed with synthetic data if needed - never copy real production data.

## Checks
- [ ] document shape / validator and/or ES mapping updated in the repository layer
- [ ] indexes defined for every queried field (unique where required); applied idempotently
- [ ] breaking ES mapping changes go through a reindex, not an in-place edit
- [ ] spec-driven integration test against real Mongo/ES (Testcontainers), green

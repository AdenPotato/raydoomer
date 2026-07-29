---
name: data-change
description: Use when changing a content definition or the save schema. Adds round-trip and migration tests first, per content_protocol.md.
---

Change a content schema (weapons, enemies, items, affixes, drop tables, levels) or the save format. Canonical rules: [content_protocol.md](../../../docs/protocol/content_protocol.md).

> **No TDD exception applies here.** Unlike a database schema, a JSON schema can be exercised the moment a fixture exists, so the test genuinely comes first. Write it red.

## Steps (in order)

1. **Failing test first:** a **round-trip** test (read, write back, read again, assert equality); a **malformed** test (a fixture missing a required field returns the typed error naming what failed, rather than crashing or silently defaulting); and for save data, a **migration test** from each supported older version. Run them red.
2. **Update the schema and the loader.** Validate once, at the boundary. Report typed errors; never crash on bad input.
3. **Bump the version field** for save data. A save without a version is unmigratable forever.
4. **Add a cross-reference check** if the new shape references other content. Validate after all content loads, when the referenced set is known. A dangling reference is a load failure, not a runtime surprise.
5. **Update the content files** to the new shape and remove any entries the change orphans.
6. **Implement to green.**

## Fixtures

Fixtures are **synthetic and committed** to the test tree. Never read from `resources/` - it is gitignored, holds copyrighted material, and does not exist in CI.

## Checks
- [ ] failing round-trip and malformed-input tests before implementation
- [ ] migration test from each supported older version (save data)
- [ ] version field present and bumped where applicable
- [ ] typed error naming the file and field; no crash, no silent default
- [ ] cross-reference validation for any new reference
- [ ] orphaned content entries removed in the same change
- [ ] fixtures synthetic and committed; no test reads `resources/`
- [ ] build + tests green

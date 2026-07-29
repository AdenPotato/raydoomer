---
name: data-change
description: Change a content definition or save schema, with round-trip and migration tests written first, per content_protocol.md.
---

Use to change a content schema (weapons, enemies, items, affixes, drop tables, levels) or the save format. Canonical rules: [content_protocol.md](../../../docs/protocol/content_protocol.md).

> **No TDD exception applies here.** Unlike a database schema, a JSON schema can be exercised the moment a fixture exists, so the test genuinely comes first. Write it red.

## Steps (in order)

1. **Failing test first:**
   - A **round-trip** test: read the fixture, write it back, read again, assert equality.
   - A **malformed** test: a fixture missing a required field returns the typed error naming what failed, rather than crashing or silently defaulting.
   - For a **save schema change**, a **migration test** from each supported older version.
   Run them red.
2. **Update the schema and the loader.** Validate once, at the boundary. Report typed errors; never crash on bad input.
3. **Bump the version field** if this is save data. A save without a version is unmigratable forever, and a schema change without its migration strands every existing save.
4. **Add the cross-reference check** if the new shape references other content (an item referencing an affix, a level referencing an entity type). Validate after all content loads, when the referenced set is known. A dangling reference is a load failure, not a runtime surprise.
5. **Update the content files** themselves to the new shape, and remove any entries the change orphans (Enforcement Rule 10 - removal takes the content with it).
6. **Implement to green.**

## Fixtures

Fixtures are **synthetic and committed** to the test tree. Never read from `resources/` - it is gitignored, holds copyrighted material, and does not exist in CI. A test that depends on it is a test that cannot run.

## Checks
- [ ] failing round-trip and malformed-input tests before implementation
- [ ] migration test from each supported older version (save data)
- [ ] version field present and bumped where applicable
- [ ] typed error naming the file and field; no crash, no silent default
- [ ] cross-reference validation for any new reference
- [ ] orphaned content entries removed in the same change
- [ ] fixtures synthetic and committed; no test reads `resources/`
- [ ] build + tests green

# Changelog

Rolling log of session outcomes. Newest entry at the top. Max 20 entries - drop the oldest when a 21st is added. Insert new entries via surgical edit (str_replace), never rewrite the file in full.

<!-- New entries go here, directly below this line and above the format section. -->

---

## Changelog format

Each entry, newest at the top, follows this exact shape:

```
YYYY-MM-DD HH:MM TZ
<Person>

## <Title>

[<category>] <Two sentences describing what happened.>

Tickets: <links or ids to the work items / PRs>
```

Rules:

- **Order is date line -> person -> `## <Title>` -> body -> `Tickets:`.** The date line is the author's local time + timezone abbreviation. Never an em dash anywhere.
- **The body opens with the category tag in brackets:** `[feature]` (new capability) · `[enhancement]` (improve existing) · `[bug]` (fix) · `[internal]` (tooling / docs / refactor / process), then a two-sentence summary.
- **Body is two sentences** describing what happened - the deeper detail lives in the PR/commits. No typed bullet list.
- `<Person>` is whoever ran the session (the git author); `<Title>` is a short human summary.
- Always include the `Tickets:` footer with the work item (and PR) references.
- No session numbers - date + person keep entries branch-merge-safe.

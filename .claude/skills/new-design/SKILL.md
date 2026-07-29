---
name: new-design
description: Start a new design document - runs the mandatory one-by-one questionnaire before generating any file in docs/design/*.
---

This skill enforces the Design Session questionnaire requirement from session_protocol.md. No design document may be produced until the full questionnaire is completed.

## Rules

- Questions are asked **one at a time** - never as a list.
- After each question, state how many questions remain (e.g., "3 questions remaining").
- The developer must answer each question before the next is asked.
- If an answer reveals a contradiction or unresolved branch, flag it immediately before continuing.
- Only after **all questions are answered and confirmed** does Claude generate the design file.
- The generated file must reflect every decision made during the questionnaire - no omissions.

## Step 1 - Identify the design area

Ask: **What design area or topic is this document for?**

Then read any existing related design files in `docs/design/` to avoid duplicating decisions already locked in `locked_decisions.md`.

## Step 2 - Build the questionnaire

Based on the design area, identify all relevant design decisions and edge cases. Structure them as yes/no or multiple-choice questions. Cover:
- Entity relationships and cardinality
- State transitions and lifecycle
- Permissions (who can do what, at which stage)
- Edge cases (what happens when X is empty, null, or conflicting)
- In-scope now vs. deferred
- Any interaction with locked decisions in `locked_decisions.md`

## Step 3 - Run the questionnaire

Ask questions one at a time. After each answer, acknowledge it and ask the next. State the count after each question.

> **Question 1:** [question text]
> *(N questions remaining)*

## Step 4 - Confirm all decisions

After all questions are answered, present a summary of every decision made. Ask the developer to confirm the list is complete and accurate before generating the file.

## Step 5 - Generate the design file

Create the file at `docs/design/<filename>.md` (lowercase, snake_case, clearly reflecting the domain). The file must:
- Reflect every decision made in the questionnaire
- Include locked-decision tables where appropriate
- Reference related files and locked decisions in `locked_decisions.md`

## Step 6 - Post-generation updates (Tracked sessions only)

1. Update `docs/reference/locked_decisions.md` - add any new locked decisions in the appropriate section.
2. Update `docs/reference/glossary.md` - add any new terms introduced.
3. Update the `session_protocol.md` File Inventory - add the new file with its purpose and update rule.

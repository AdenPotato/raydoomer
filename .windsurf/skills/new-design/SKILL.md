---
name: new-design
description: Use when creating a new design document under docs/design/. Runs the mandatory one-by-one questionnaire before any design file is generated.
---

This skill enforces the Design Session questionnaire requirement from [session_protocol.md](../../../docs/protocol/session_protocol.md). No design document may be produced until the full questionnaire is completed.

## Rules

- Questions are asked **one at a time** - never as a list.
- After each question, state how many remain (e.g., "3 questions remaining").
- The developer answers each question before the next is asked.
- If an answer reveals a contradiction or unresolved branch, flag it immediately.
- Only after **all questions are answered and confirmed** is the design file generated, and it must reflect every decision made.

## Step 1 - Identify the design area

Ask: **What design area or topic is this document for?** Then read any existing related design files in `docs/design/` to avoid duplicating decisions already locked in `locked_decisions.md`.

## Step 2 - Build the questionnaire

Identify all relevant design decisions and edge cases as yes/no or multiple-choice questions. Cover: entity relationships and cardinality; state transitions and lifecycle; permissions; edge cases (empty, null, conflicting); in-scope now vs. deferred; any interaction with locked decisions.

## Step 3 - Run the questionnaire

Ask one at a time. After each answer, acknowledge it, state the remaining count, and ask the next.

## Step 4 - Confirm all decisions

Present a summary of every decision made. Ask the developer to confirm it is complete and accurate before generating the file.

## Step 5 - Generate the design file

Create `docs/design/<filename>.md` (lowercase, snake_case). It must reflect every decision, include locked-decision tables where appropriate, and reference related files and locked decisions.

## Step 6 - Post-generation updates (Tracked sessions only)

Update `docs/reference/locked_decisions.md` (new locked decisions), `docs/reference/glossary.md` (new terms), and the `session_protocol.md` File Inventory (the new file + its update rule).

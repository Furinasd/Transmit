# Transfer Design Contract

> Status: Active, intentionally incomplete. This file is the engineering boundary, not a mirror of Notion.

## Authority

Only rules explicitly promoted into this document are authorized for implementation. Notion may contain exploration, alternatives, and rejected ideas; those remain non-binding until promoted here.

When a behavior is absent or marked unresolved, do not infer it from a brainstorm. Ask for a design decision or implement only the smallest reversible seam needed to defer that decision.

## Core Verb

The player can transfer one **Transfer State** from one valid carrier to another valid receiver.

The concrete meaning and initial set of Transfer States have not yet been promoted.

## Vocabulary

- **Transfer State:** The single transferable gameplay state currently held by a carrier.
- **Carrier:** A Player, Enemy, or Environment actor that currently owns a Transfer State.
- **Receiver:** A Player, Enemy, or Environment actor that is eligible to receive a Transfer State.
- **Transfer attempt:** One explicit request to move a Transfer State from a carrier to a receiver.

## Invariants

1. A carrier owns at most one Transfer State.
2. A transfer attempt never fails silently; success or rejection must produce an inspectable result and player-readable feedback.
3. A valid receiver is readable before transfer input is committed.
4. Transfer State uses one consistent visual language across Player, Enemy, and Environment.

## Interaction Matrix

No directional interaction has been promoted yet. Add a source-to-receiver row here only when that interaction is an accepted implementation requirement.

| Source | Receiver | Rule | First Level | Status |
| --- | --- | --- | --- | --- |
| — | — | No interaction promoted | — | Unresolved |

## Current Level Usage

No level-specific Transfer usage has been promoted yet. The five-level paper progression and the three playable levels remain success criteria in `GOAL.md`, not evidence that their rules or content already exist.

## Required Promotion Details

A new rule is implementation-ready only when this contract states:

- the Transfer State involved;
- valid source and receiver categories;
- preconditions and rejection behavior;
- player-readable preview and result feedback;
- where the rule first appears in the level progression.

## Change Rule

Design exploration happens outside this file. Once a rule is accepted, update this contract first, then reconcile `ARCHITECTURE.md`, the relevant ADR, implementation, tests, and `STATE.md` in that order.

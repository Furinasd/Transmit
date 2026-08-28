# ADR-002: Transfer Interaction Model

- Status: Proposed
- Date: 2026-08-28
- Owners: Project design and engineering

## Context

Combat and puzzle interactions must share one underlying Transfer system. Valid receivers must be readable before input, and a transfer attempt must never fail silently. Pair-specific Player/Enemy/Environment logic would make these guarantees inconsistent and difficult to extend.

## Proposed Decision

Use one query-then-commit interaction model for every promoted source-to-receiver pairing:

1. Target selection resolves actors through `ITransferable`.
2. Preview queries the source and receiver components and returns eligibility plus a rejection reason.
3. Committed input calls `TryTransferTo(Target)` on the source component.
4. Commit validation runs again against current world state.
5. Success clears the source and sets the receiver as one atomic operation.
6. Success or rejection returns a structured result and emits the corresponding event.

Target selection may differ by platform or presentation, but it must feed this same interaction path. Actor-specific reactions subscribe to transfer/state events rather than modifying the transaction.

## Rationale

- Combat and puzzle use the same validation and commit semantics.
- Preview and commit share rule evaluation without assuming the preview remains valid.
- Structured rejection prevents silent failure and gives presentation a stable feedback input.
- New actor categories do not require pair-specific casting in the core system.

## Consequences

- Validation results need stable reason identifiers suitable for UI/VFX mapping.
- Transfer commit must define rollback-safe behavior if either participant changes during the request.
- Target-selection presentation remains separate from transfer authority.
- Each promoted interaction still needs explicit design-contract rules; the generic pipeline does not authorize every possible pairing.

## Rejected Alternatives

- **Direct Blueprint mutation after a trace:** allows preview, validation, and state mutation to diverge.
- **Pair-specific interaction functions:** duplicates the core verb and couples combat/puzzle actors.
- **Preview-only validation:** can commit stale or invalid target state.

## Acceptance Gate

Before changing this ADR to `Accepted`:

- promote at least one directional interaction into `DESIGN_CONTRACT.md`;
- define its valid/invalid preview language and rejection behavior;
- confirm the target-selection requirements for the first playable platform;
- prove that a rejected commit leaves both participants unchanged.

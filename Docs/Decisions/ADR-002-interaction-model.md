# ADR-002: Capture / Transfer Interaction Model

- Status: Proposed
- Date: 2026-08-28
- Owners: Project design and engineering

## Context

Combat and puzzle interactions must share the same `Read → Capture → Carry → Transfer → Convert` grammar. Third-person aim selects actors but cannot rewrite Motion direction. Valid Sources/Targets must be readable before input, and neither Capture nor Transfer may fail silently or duplicate/lose state.

## Proposed Decision

Use one query-then-commit interaction model with two explicit player verbs.

### Capture

1. Target selection resolves a Source through `IMotionTransferable`.
2. Preview calls `CanCaptureMotion` and returns eligibility plus a rejection reason.
3. Capture input revalidates current world state.
4. Success atomically clears Source and assigns the same `FMotionState` to Player.

### Transfer

1. Target selection resolves a Target through `IMotionTransferable`.
2. Preview calls `CanReceiveMotion` with the Player's current state.
3. Transfer input revalidates compatibility.
4. Success atomically clears Player and assigns or consumes the state at Target.

Both verbs return a structured success/rejection result and emit state facts after commit. A rejected operation preserves the current owner.

Target ranking is deterministic: reticle angle, occlusion, distance, then compatibility. Soft-cone assistance and short target stickiness may vary by platform, but they feed the same transaction path and never provide output direction.

Environment conversion is a separate `IMotionConverter` path. It deterministically maps an input signature/entry geometry to an output signature and does not become another player verb.

## Rationale

- Combat and puzzle use the same ownership, validation, and commit semantics.
- Preview and commit share rule evaluation without assuming the preview remains valid.
- Structured rejection prevents silent failure and gives presentation a stable feedback input.
- Stable target ranking makes third-person selection predictable under overlap and motion.
- New actor categories do not require pair-specific casting in the core system.
- Converters can change direction/type without granting the Player arbitrary redirection.

## Consequences

- Validation results need stable reason identifiers for type, direction, magnitude, occlusion, timing, and invalidation.
- Capture and Transfer commits must be rollback-safe if either participant changes during the request.
- Target-selection presentation remains separate from transfer authority.
- Room Reset must clear transient selection and restore all critical ownership state.
- Each promoted interaction still needs explicit Design Contract rules; the generic pipeline does not authorize Player → Charger Transfer.

## Rejected Alternatives

- **Direct Blueprint mutation after a trace:** allows preview, validation, and state mutation to diverge.
- **Pair-specific interaction functions:** duplicates the core verb and couples combat/puzzle actors.
- **Preview-only validation:** can commit stale or invalid target state.
- **Aim vector becomes output direction:** contradicts direction preservation and turns Transfer into ordinary shooting.
- **Converter handled as a special level script:** prevents the same transformation rule from composing across levels.

## Acceptance Gate

Before changing this ADR to `Accepted`:

- EXP-001 must complete Source → Player → Receiver with preview, Capture, Carry, Transfer, and Reset;
- a first-time player must complete the loop in 60–90 seconds without verbal explanation and identify the current owner;
- invalid/occluded Target tests must preserve state and produce distinguishable feedback;
- overlapping candidates must select consistently under small camera motion;
- only after EXP-001 passes should the Redirect/Relay path validate deterministic conversion.

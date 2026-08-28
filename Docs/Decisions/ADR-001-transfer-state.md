# ADR-001: Motion State Ownership and Representation

- Status: Proposed
- Date: 2026-08-28
- Owners: Project design and engineering

## Context

The Design Contract now fixes the transferable resource as one gameplay-authored Motion State. P0 uses Linear motion with direction and magnitude; Player, Enemy, and Environment must share one ownership rule. The repository still has no Motion Transfer code, so the first implementation must avoid parallel Blueprint mutation paths.

## Proposed Decision

Represent the runtime value as `FMotionState`:

- `Type` — Linear in P0;
- `DirectionOrAxis`;
- `Magnitude`;
- optional Period/Phase fields reserved for future motion types;
- SourceId/DebugTag for traceability.

Each participating actor owns one `UMotionTransferComponent`. That component owns zero or one runtime `CurrentMotion` and is the only authority allowed to mutate it.

Actors participate through `IMotionTransferable`, which exposes the component and compatibility queries. Capture and Transfer atomically move the value between components; they do not copy or recreate it.

Data Assets may describe reusable presentation/tuning by Motion type, but they never own direction, magnitude, carrier ownership, target selection, or room progress for a runtime instance.

## Rationale

- The same ownership rule applies to Source, Player, Enemy, Carrier, and Receiver.
- A single writer makes the one-state invariant testable.
- A value struct represents the actual direction/magnitude without creating one asset per runtime state.
- Actor-specific behavior remains decoupled from core state mutation.
- Future Angular/Oscillation fields can extend the value without introducing a second resource grammar.

## Consequences

- Participating actors must provide a Motion component and implement the interface.
- Blueprint receives read/query and request APIs, but no writable `CurrentMotion` property.
- Capture, Transfer, consumption, and Reset must all use the same component mutation boundary.
- Save/load or replication, if later added, must serialize component state rather than Data Assets.
- P0 magnitude can change from continuous to tiered without changing ownership or interface boundaries.

## Rejected Alternatives

- **State stored directly on each actor class:** duplicates rules and encourages pair-specific implementations.
- **Global subsystem owns all carrier state:** centralizes unrelated actor lifetime and adds lookup/cleanup complexity before it is needed.
- **Mutable state stored in a Data Asset:** shared assets cannot safely represent per-instance runtime ownership.
- **Motion represented only as live physics velocity:** cannot reliably preserve identity, preview compatibility, or deterministic Reset.

## Acceptance Gate

Before changing this ADR to `Accepted`, EXP-001 must demonstrate:

- one Source, Player, and Receiver using the same component/interface boundary;
- at least 20 Capture/Transfer/Reset cycles without duplication or lost ownership;
- a rejected transaction leaves the current owner unchanged;
- room Reset restores the authoritative Source/Receiver/Player state;
- Blueprint presentation reacts to events without directly writing runtime state.

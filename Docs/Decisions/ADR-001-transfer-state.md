# ADR-001: Transfer State Ownership

- Status: Proposed
- Date: 2026-08-28
- Owners: Project design and engineering

## Context

Player, Enemy, and Environment actors must participate in one Transfer system while preserving the invariant that a carrier owns at most one Transfer State. The repository does not yet contain Transfer code, so ownership must be decided before implementation creates multiple mutation paths.

## Proposed Decision

Each participating actor owns exactly one `UTransferComponent`. That component owns zero or one runtime `CurrentState` and is the only authority allowed to mutate it.

Actors participate through `ITransferable`, which exposes their Transfer component. Authored state definitions live in `DA_TransferDefinition`; Data Assets are immutable definitions and never own runtime carrier state.

The component validates and commits transfers. Actor classes and Blueprint presentation react to component events but do not write the state directly.

## Rationale

- The same ownership rule applies to Player, Enemy, and Environment.
- A single writer makes the one-state invariant testable.
- Actor-specific behavior remains decoupled from core state mutation.
- Definitions can be reused and tuned without storing runtime ownership in shared assets.

## Consequences

- Participating actors must provide a Transfer component and implement the interface.
- Blueprint needs read/query and request APIs, but no writable `CurrentState` property.
- Transfer-specific persistence or replication must serialize component state rather than Data Assets.
- Component lifecycle and save/load behavior must be decided if those capabilities enter scope.

## Rejected Alternatives

- **State stored directly on each actor class:** duplicates rules and encourages pair-specific implementations.
- **Global subsystem owns all carrier state:** centralizes unrelated actor lifetime and adds lookup/cleanup complexity before it is needed.
- **Mutable state stored in a Data Asset:** shared assets cannot safely represent per-instance runtime ownership.

## Acceptance Gate

Before changing this ADR to `Accepted`:

- confirm that every initial participant can host the component;
- confirm whether zero-state and replacement behavior match the design contract;
- define the minimal result/state types needed by the first promoted interaction;
- agree that Blueprint cannot directly write runtime state.

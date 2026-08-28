# Transmit Architecture

> Status: Current Blueprint skeleton plus proposed M0 Motion Transfer target. The repository contains no Motion Transfer implementation; proposed sections do not claim otherwise.

## Scope

This document records technical ownership, interfaces, data flow, and the Blueprint/C++ boundary. Whether a rule is fun, balanced, or promoted for a level belongs in `DESIGN_CONTRACT.md`.

Read project context in this order:

1. `GOAL.md`
2. `DESIGN_CONTRACT.md`
3. `ARCHITECTURE.md`
4. Relevant records under `Decisions/`
5. Current implementation and tests
6. `STATE.md` for temporary handoff context only

## Current Repository Architecture

The GitHub repository is named `Transmit`. The Unreal project file and internal game name remain `passely`; renaming those identifiers is outside this architecture bootstrap because serialized assets and future native-module paths may depend on them.

This is currently a Blueprint-only project. There is no custom C++ module under `Source/` and no project-local plugin content under `Plugins/`.

| Boundary | Current responsibility |
| --- | --- |
| `passely.uproject` | Unreal project entrypoint and Engine-plugin declarations |
| `Config/DefaultEngine.ini` | Startup map, default GameMode, renderer, target-platform, asset-manager, and project settings |
| `Content/ThirdPerson/` | Current map plus Character, PlayerController, and GameMode Blueprints |
| `Content/Input/` | Enhanced Input actions, mapping contexts, and touch interface assets |
| `Content/Characters/Mannequins/` | Manny/Quinn meshes, rigs, materials, textures, and animation library |
| `Content/LevelPrototyping/` | Template geometry, materials, and sample interactables used for level assembly |
| `Content/__ExternalActors__/` and `Content/__ExternalObjects__/` | One-file-per-actor/object data belonging to the current map; these are source assets, not generated cache |
| `Docs/` | Product contract, architecture boundary, decisions, and handoff state |

### Current Runtime Path

```text
passely.uproject
        ↓
Config/DefaultEngine.ini
        ├── startup/default map: /Game/ThirdPerson/Lvl_ThirdPerson
        └── default GameMode: BP_ThirdPersonGameMode
                    ↓
BP_ThirdPersonPlayerController + BP_ThirdPersonCharacter
                    ↓
Enhanced Input mappings/actions
                    ↓
Mannequin animation and level presentation assets
```

The configuration-to-asset path above is present. Blueprint graph behavior was not exhaustively inspected, and no Transfer path exists yet.

### Current Dependencies and Persistence

- The project enables `ModelingToolsEditorMode`, `GameplayStateTree`, `ModelContextProtocol`, and `AllToolsets`; the latest local Editor log resolved all four from the UE 5.8 Engine installation rather than from repository-local plugins.
- Gameplay persistence, save/load, networking, replication, and external-service integrations are not currently represented in project code or configuration.
- Unreal binary assets are Git LFS content. `DerivedDataCache/`, `Intermediate/`, `Saved/`, IDE state, local automation reports, and `Content/Developers/` are local-only boundaries.

## Target Runtime Model

```text
FMotionState (runtime value)
        ├── Type: Linear [P0]
        ├── DirectionOrAxis
        ├── Magnitude
        ├── OptionalPeriod / Phase [future]
        └── SourceId / DebugTag

Player / Enemy / Environment Actor
        │
        ├── implements IMotionTransferable
        │
        └── owns one UMotionTransferComponent
                    ├── CurrentMotion (zero or one)
                    ├── TryCaptureFrom(Source)
                    ├── TryTransferTo(Target)
                    ├── Clear / Reset
                    └── state/result events

Environment Converter
        └── implements IMotionConverter
                    ├── CanConvert(Input)
                    ├── PreviewOutputSignature(Input)
                    └── ConvertMotion(Input)
```

`FMotionState` is an instance value, not an asset identity. It carries the actual direction and magnitude being routed through the room.

The actor owns the component. The component is the only runtime writer of `CurrentMotion`; Player, Enemy, and Environment presentation can observe it but cannot mutate it directly. One successful Capture or Transfer atomically moves the value between components.

This technical ownership proposal is recorded in `Decisions/ADR-001-transfer-state.md` and remains proposed until EXP-001 proves the smallest runtime path.

## Proposed Interfaces

`IMotionTransferable` marks an actor as participating in Motion ownership. Player, Moving Source, Transfer Crate, Receiver, and Charger use the same interface rather than pair-specific casts.

The initial C++ surface should remain small:

- `GetMotionTransferComponent()` — returns the actor's Motion component.
- `CanCaptureMotion(Request)` — reports whether the actor is a valid Source and why not.
- `CanReceiveMotion(State, Request)` — reports type/direction/magnitude compatibility and why not.
- `TryCaptureFrom(Source)` — atomically clears Source and assigns Player.
- `TryTransferTo(Target)` — atomically clears Player and assigns/consumes at Target.
- `ClearMotionState()` and `ResetMotionState()` — support authoritative room recovery.

`IMotionConverter` is separate because Redirect Rails and Cranks transform a moving carrier or Motion signature; they are not additional Player verbs or arbitrary receivers.

Exact Unreal signatures, result structs, replication policy, and lifecycle hooks remain implementation decisions.

## Target Selection Boundary

Third-person aim chooses candidates; it never supplies output direction.

The targeting layer produces a stable candidate plus a preview result using:

1. reticle angle;
2. occlusion;
3. distance;
4. compatibility with the Player's current carry state.

Soft-cone assistance and short target stickiness belong here. The transaction layer revalidates the selected actor at commit time and remains authoritative.

## Proposed Control and Data Flow

### Capture

```text
Capture input
        ↓
Resolve IMotionTransferable Source
        ↓
Preview + commit-time CanCaptureMotion
        ↓
Atomically clear Source.CurrentMotion
and set Player.CurrentMotion
        ↓
Emit captured/state-changed result
        ↓
Source stop/stagger + Player carry presentation
```

### Transfer

```text
Transfer input
        ↓
Resolve IMotionTransferable Target
        ↓
Preview + commit-time CanReceiveMotion
        ↓
Atomically clear Player.CurrentMotion
and set or consume Target state
        ↓
Emit transferred/state-changed result
        ↓
Target movement/function + causal presentation
```

Validation occurs during preview and again at commit because world state may change between them. A rejected transaction preserves the current owner and returns a structured reason such as type, direction, magnitude, occlusion, timing, or target invalidation.

## Conversion Boundary

A converter is a deterministic mapping from an input Motion signature and entry geometry to an output Motion signature.

- Redirect Rail: `Linear(input direction) → Linear(rail output direction)`.
- Crank P1: `Linear(input) → Angular(axis, clockwise/counter-clockwise, magnitude)`.
- `PreviewOutputSignature` must use the same rule as committed conversion.
- Critical conversion should use constrained or authored motion when full Chaos simulation would make identical inputs diverge.

Converters do not authorize the Player to rewrite direction and do not become level-specific scripted keys.

## Proposed Event Boundary

The core system emits state facts; consumers decide presentation and actor reactions.

- **Preview changed:** selected target, eligibility, output signature, or rejection reason changed.
- **Motion captured:** Source, Player, and moved `FMotionState` are reported after commit.
- **Motion transferred:** Player, Target, and moved `FMotionState` are reported after commit.
- **Motion converted:** input and output signatures plus converter are reported.
- **Motion rejected:** attempted verb and rejection reason are reported without changing ownership.
- **Motion state changed:** a component gained, lost, consumed, or restored state.

Presentation listens to these events. It must not become a second state writer.

## Data Asset Boundary

Runtime direction, magnitude, ownership, target selection, and room progress live in structs/components, not Data Assets.

If M0 needs authored presentation/tuning, a Motion presentation Data Asset may store reusable references such as type-specific trail/VFX/audio, indicator rules, and tuning limits. P0 should not create one asset per runtime state instance, and it should not freeze a broad schema before EXP-001 demonstrates a concrete need.

## Room Reset Boundary

The room owns an authoritative start snapshot for every critical Source, Carrier, Receiver, Converter, Charger, and Player carry state. Death, fail, or explicit Reset restores that snapshot and clears transient selection/events. This prevents state loss, out-of-bounds carriers, or eliminated Sources from creating a soft lock.

## Blueprint and C++ Boundary

### C++ owns

- `FMotionState` representation and invariants
- `UMotionTransferComponent`, `IMotionTransferable`, and `IMotionConverter`
- preview queries, stable rejection identifiers, and atomic Capture/Transfer commit
- deterministic conversion functions and ownership-safe room Reset contracts
- stable events needed by gameplay and presentation
- focused automation tests for ownership, rejection, conversion, and Reset

### Blueprint owns

- input binding, reticle/soft-cone presentation, and selected-target feedback
- materials, VFX, audio, animation, and UI feedback
- actor-specific reactions built on core events
- Data Asset authoring and tuning
- level assembly and scripted teaching beats

Blueprint may request Capture/Transfer and react to results, but it must not directly mutate `CurrentMotion`.

## Dependency Direction

Core Motion Transfer code must not depend on a specific Player, Enemy, Environment actor class, level, material, or UI widget. Those layers depend on Motion interfaces, result data, and events.

## Current Implementation Boundary

As of 2026-08-28, no C++ source or Motion Transfer implementation was found in the repository. A local UE 5.8.1 Editor log records successful engine initialization and a map check with 0 errors and 0 warnings. A `CompileAllBlueprints` commandlet run completed Blueprint compilation with 0 errors, 0 warnings, and 0 load failures; the process still exited 1 because the workstation's Installed DDC/Zen cache had no writable node and used an in-memory fallback. PIE gameplay, build, packaging, and cross-platform compatibility remain unverified.

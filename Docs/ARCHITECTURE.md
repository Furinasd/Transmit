# Transmit Architecture

> Status: Current Blueprint skeleton plus proposed M0 Transfer target. The repository contains no Transfer implementation; proposed sections do not claim otherwise.

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

## Proposed Ownership Model

```text
Player / Enemy / Environment Actor
        │
        ├── implements ITransferable
        │
        └── owns one UTransferComponent
                    │
                    ├── CurrentState (zero or one)
                    ├── CanSend()
                    ├── CanReceive(Definition)
                    ├── TryTransferTo(Target)
                    └── state/result events

DA_TransferDefinition
        ├── stable identity
        ├── presentation references
        ├── rule parameters
        └── tuning parameters
```

The actor owns the component. The component is the only runtime writer of Transfer State. A Data Asset describes immutable authored data; it does not own per-actor runtime state.

This ownership proposal is recorded in `Decisions/ADR-001-transfer-state.md` and is not final until that ADR is accepted.

## Proposed Interfaces

`ITransferable` marks an actor as participating in the Transfer system and exposes its `UTransferComponent`. Player, Enemy, and Environment actors use the same interface rather than pair-specific casts.

The initial C++ surface should remain small:

- `GetTransferComponent()` — returns the actor's Transfer component.
- `CanSend()` — returns eligibility and an explicit rejection reason.
- `CanReceive(Definition)` — returns eligibility and an explicit rejection reason.
- `TryTransferTo(Target)` — validates and commits one transfer as a single operation.

Exact Unreal signatures, result types, replication policy, and lifecycle hooks remain implementation decisions.

## Proposed Control and Data Flow

```text
Input / target selection
        ↓
Resolve ITransferable source and candidate receiver
        ↓
Query CanSend + CanReceive for preview
        ↓
Render valid / invalid receiver feedback
        ↓
TryTransferTo on committed input
        ↓
Validate again, then atomically clear source and set receiver
        ↓
Emit success or rejection result
        ↓
Blueprint presentation and actor-specific reactions
```

Validation occurs both during preview and at commit because world state may change between them. A failed commit must preserve the source state and return an explicit reason.

## Proposed Event Boundary

The core system emits state facts; consumers decide presentation and reactions.

- **Preview changed:** target eligibility or rejection reason changed.
- **Transfer committed:** source, receiver, and Transfer definition are reported after state mutation succeeds.
- **Transfer rejected:** the attempt and rejection reason are reported without mutating either actor.
- **State changed:** a component gained, lost, or replaced its current state.

Presentation listens to these events. Presentation code must not become a second writer of Transfer State.

## Data Asset Boundary

`DA_TransferDefinition` stores authored, reusable data such as identity, visual/audio references, rule parameters, and tuning values. It must not store mutable carrier ownership, transient target selection, or level-specific runtime progress.

The exact parameter schema cannot be frozen until the first concrete Transfer State is promoted into `DESIGN_CONTRACT.md`.

## Blueprint and C++ Boundary

### C++ owns

- Transfer State representation and invariants
- `UTransferComponent` and `ITransferable`
- validation and atomic transfer commit
- structured success/rejection results
- stable events needed by gameplay and presentation
- focused automation tests for invariants

### Blueprint owns

- input binding and target-selection presentation
- materials, VFX, audio, animation, and UI feedback
- actor-specific reactions built on core events
- Data Asset authoring and tuning
- level assembly and scripted teaching beats

Blueprint may request a transfer and react to results, but it must not directly mutate `CurrentState`.

## Dependency Direction

Core Transfer code must not depend on a specific Player, Enemy, Environment actor class, level, material, or UI widget. Those layers depend on the Transfer interface, result data, and events.

## Current Implementation Boundary

As of 2026-08-28, no C++ source or Transfer-specific implementation was found in the repository. A local UE 5.8.1 Editor log records successful engine initialization and a map check with 0 errors and 0 warnings. A `CompileAllBlueprints` commandlet run completed Blueprint compilation with 0 errors, 0 warnings, and 0 load failures; the process still exited 1 because the workstation's Installed DDC/Zen cache had no writable node and used an in-memory fallback. PIE gameplay, build, packaging, and cross-platform compatibility remain unverified.

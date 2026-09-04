# Transmit Architecture

> Status: Final v0.4 design lock promoted to repository docs on 2026-09-05 (docs-only). The v0.3 ordinary-Linear resolver core, Charger FSM, Charger swept movement, and Actor dispatch are implemented and automated-verified on `feat/gameplay-core-v03` (HEAD `1fb96ea`). Directional Carrier and Boss High Motion direction policy are v0.4-promoted runtime deltas that are not implemented yet; `L_Transmit` is future content. Sections still marked "Proposed" are target design.

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

The project now has a custom C++ runtime module under `Source/passely/` (Motion types, ownership component, Actor interface, interactor, room reset, endpoint/indicator actors, and focused automation tests) plus a Blueprint layer under `Content/Transmit/` (input assets, Character/Controller/GameMode/Source/Receiver Blueprints, and `L_TestChamber`). There is no project-local plugin content under `Plugins/`.

Map architecture under Final v0.4: `L_TestChamber` is the regression / micro-validation map; `L_Transmit` is the single production map containing continuous Zone 1 Learn → Zone 2 Route → Zone 3 Weaponize. `L_Transmit` does not exist in the repository and is future content; L1 / L2 / L3 remain design progression IDs, not independent `.umap` files.

| Boundary | Current responsibility |
| --- | --- |
| `passely.uproject` | Unreal project entrypoint and Engine-plugin declarations |
| `Config/DefaultEngine.ini` | Startup map, default GameMode, renderer, target-platform, asset-manager, and project settings |
| `Source/passely/` | EXP-001 Motion core C++: `FMotionState`, `UMotionTransferComponent`, `IMotionTransferable`, interactor, room reset, endpoint/direction indicators, automation tests |
| `Content/ThirdPerson/` | Current map plus Character, PlayerController, and GameMode Blueprints |
| `Content/Transmit/` | EXP-001 Blueprint layer: Transmit input assets, Character/Controller/GameMode/Source/Receiver Blueprints, `L_TestChamber` |
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

The configuration-to-asset path above is present. A Transfer path now exists under `/Game/Transmit` (`L_TestChamber`); Blueprint graph behavior still requires human playtest acceptance.

### Current Dependencies and Persistence

- The project enables `ModelingToolsEditorMode`, `GameplayStateTree`, `ModelContextProtocol`, and `AllToolsets`; the latest local Editor log resolved all four from the UE 5.8 Engine installation rather than from repository-local plugins.
- Gameplay persistence, save/load, networking, replication, and external-service integrations are not currently represented in project code or configuration.
- Unreal binary assets are Git LFS content. `DerivedDataCache/`, `Intermediate/`, `Saved/`, IDE state, local automation reports, and `Content/Developers/` are local-only boundaries.

## Target Runtime Model

```text
FMotionState (runtime value)
        ├── Type: Linear [P0]
        ├── DirectionOrAxis
        ├── DirectionPolicy: Ordinary Linear = CameraCanonical,
        │                     Boss High Motion = PreserveSource (Dash world direction)
        │                     [v0.4 promoted; not yet implemented]
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

This technical ownership model is recorded in `Decisions/ADR-001-transfer-state.md`; EXP-001 has now proven the smallest runtime path in PIE.

## Proposed Interfaces

`IMotionTransferable` marks an actor as participating in Motion ownership. Player, Moving Source, Transfer Crate, Receiver, and Charger use the same interface rather than pair-specific casts.

Native callers use `IMotionTransferable::Call*` helpers for Actor-level interface dispatch. They call a class-level UFunction when a Blueprint owns the event and otherwise use the native interface vtable, so native-only C++ `_Implementation` overrides remain reachable without bypassing Blueprint overrides.

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

Third-person aim chooses candidates; it never supplies a free output direction.

The targeting layer (`UMotionInteractorComponent`) produces a stable candidate plus a preview result using:

1. reticle angle;
2. occlusion;
3. distance;
4. compatibility with the Player's current carry state and the resolved canonical direction.

Soft-cone assistance and short target stickiness belong here. The transaction layer revalidates the selected actor at commit time and remains authoritative.

## Direction Policy Boundary (Final v0.4)

Target selection and direction resolution are physically decoupled. Direction policy is a property of the carried Motion, not of the camera or the Target:

- **Ordinary Linear — CameraCanonical**: `UMotionCanonicalDirectionResolver` maps (carried Linear direction + gameplay camera pose) → one of six canonical directions and a world-space `ProjectedWorldDirection`, with pitch and sector hysteresis. This is the implemented, frozen v0.3 resolver.
- **Boss High Motion — PreserveSource**: direction stays locked to the committed Charger Dash world direction and bypasses the camera resolver. This policy is promoted by v0.4 and not implemented; current code carries no explicit direction-policy marker, so captured Dash Motion still enters CameraCanonical on Transfer. The policy must not be inferred from magnitude or `SourceId`.
- **Preview = Commit is policy-independent**: whichever policy applies, the interactor computes the world direction once and carries it inside `FMotionTransferContext.DirectionResolution`; Preview and Commit consume the same result.
- **`RequiredCanonicalDirection`**: receivers may declare one of the six canonical directions; a mismatch is `IncompatibleDirection` and never consumes Player Motion. This is a compatibility/regression capability, not the Zone 2 core mechanic.
- **Directional Carrier (promoted, not implemented)**: the ordinary-Linear Target role that accepts any of the six resolved directions and moves itself — the Actor, not a child presentation mesh — in world space. It must use deterministic swept collision, stop on blocking collision, remain a valid Source for re-capture, and be restored by the existing Room Reset snapshot.

The seam for this change is the existing `FMotionTransferContext`/state path: introduce one source-authored direction-policy value, resolve the world direction once per policy, and keep ownership mutation (`TryMoveBetween`) untouched.

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
Determine direction policy from the carried Motion
        ├── Ordinary Linear → CameraCanonical: carried direction + gameplay camera
        │         → one of Forward / Back / Left / Right / Up / Down
        └── Boss High Motion → PreserveSource: committed Dash world direction
                              [v0.4 promoted; not yet implemented]
        ↓
Compute one world direction; carry it in FMotionTransferContext.DirectionResolution
        ↓
Preview + commit-time CanReceiveMotion(resolved state, same direction)
        ↓
Atomically clear Player.CurrentMotion and set/consume resolved state at Target
        ↓
Emit transferred/state-changed result
        ↓
Target movement/function (Carrier moves in world space) + causal presentation
```

Validation occurs during preview and again at commit because world state may change between them. A rejected transaction preserves the current owner and returns a structured reason such as type, direction, magnitude, occlusion, timing, or target invalidation.

## Conversion Boundary

A converter is a deterministic mapping from an input Motion signature and entry geometry to an output Motion signature.

- Redirect Rail: `Linear(input direction) → Linear(rail output direction)`.
- Crank P1: `Linear(input) → Angular(axis, clockwise/counter-clockwise, magnitude)`.
- `PreviewOutputSignature` must use the same rule as committed conversion.
- Critical conversion should use constrained or authored motion when full Chaos simulation would make identical inputs diverge.

Converters do not authorize the Player to rewrite direction freely. Under Final v0.4, ordinary Linear is rerouted by CameraCanonical at Transfer; Boss High Motion keeps its captured Dash world direction (PreserveSource) and does not pass through a converter for direction. Redirect Rail remains a possible deterministic converter and is not the Zone 2 mechanism (ADR-003).

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

As of 2026-09-05, HEAD `1fb96ea` (`feat/gameplay-core-v03`) is the committed checkpoint. Runtime state by category:

**Implemented / verified**

- v0.3 ordinary-Linear core: `UMotionCanonicalDirectionResolver` (six canonical directions, pitch + sector hysteresis, deterministic), `FMotionDirectionResolution` carried inside `FMotionTransferContext`, `EMotionCanonicalDirection` Receiver requirements, and `UMotionTransferComponent::GrantMotionState`.
- Charger: `ATransmitChargerActor` + `UMotionChargerStateMachine` (Telegraph → Dash → Recovery with a capture window), collision-enabled capsule root, presentation-only Body collision, swept dash movement, blocking-hit Recovery, and `EMotionTransferRejection::TimingRejected`.
- Actor dispatch: `IMotionTransferable::Call*` preserves Blueprint event dispatch while making native-only C++ implementations reachable.
- Historical evidence: macOS `passelyEditor` build succeeded and 10/10 `Transmit.MotionTransfer` tests passed; EXP-001 PIE loop and 20/20 Reset were verified on the pre-v0.3 baseline. A fresh source-aligned rerun on HEAD `1fb96ea` is not yet recorded.

**Pre-existing committed work (checkpoint `1fb96ea`, not v0.4 deltas)**

- `TransmitHUD`, `MotionDirectionIndicatorComponent` presentation rework, `TransmitMotionEndpointActor` presentation changes, canonical resolver camera-pitch Up/Down update with tests, and `BP_TransmitGameMode` asset update.

**v0.4 promoted, not implemented**

- Directional Carrier actor: no world-space actor movement, swept blocking stop, re-capture path, or carrier test exists. The existing `TransmitMotionEndpointActor` only loops a child `Body` presentation and cannot substitute.
- Direction-policy seam: no policy field exists in `FMotionState`/context, so captured Charger Dash Motion is still camera-rerouted on Transfer instead of preserving the committed Dash world direction.
- Open consistency defect against frozen Preview = Commit: the default Actor-path Preview discards `FMotionTransferContext.DirectionResolution` (it calls `CanReceiveState(State)` without the resolution) while the component-level Commit rechecks with the resolution. A `RequiredCanonicalDirection` Receiver can therefore preview eligible and reject at commit. Existing tests call the component directly and do not cover the Actor path.
- Ram Block content role for Zone 3.

**Future content**

- `L_Transmit` single map, Zone 1 Learn → Zone 2 Route → Zone 3 Weaponize blockout, Bridge Slab / Ram Rail / Ram / Breakable Gate / Boss encounter content, presentation pass, and first-player playtest.

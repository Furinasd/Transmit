# Transmit Architecture

> Status: updated during L_Transmit candidate production on 2026-09-06. CameraCanonical, PreserveSource, Actor-path Preview=Commit and Directional Carrier are implemented. The formal Learn → Route → Weaponize map and level flow exist on `Jason/L_Transmit_v01`; current validation and remaining human gates are recorded in `STATE.md` and `Handoff/A_LevelFlow.md`. Sections marked "Proposed" remain target design.

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

Soft-cone assistance and short target stickiness belong here. The gameplay camera POV matches the reticle; non-player test actors retain their eye-view fallback. Candidate range and original LOS stay based on the player eyes, with camera visibility also required, so a SpringArm cannot extend reach or see around a player-blocking wall. Acquisition defaults to a 28-degree half cone, with up to 10 degrees of physical-mesh size assistance; the current target has an additional 12-degree release margin. Ranking still uses angle/distance plus the existing sticky score. Physical mesh centers avoid selecting a moving Source by an unrelated Actor pivot. The transaction layer revalidates the selected actor at commit time and remains authoritative.

Presentation reads that same preview: `TransmitHUD` draws a small reticle and corner brackets around the selected physical body. `MotionDirectionIndicatorComponent` positions a compact runtime arrow just outside the dominant physical mesh's output face (mesh-local intersection supports rotation and non-uniform scale), aligned to `ProjectedWorldDirection`; it never chooses direction. A dimmed self-occlusion material keeps the far-side cue visible while occluded/ineligible targets still suppress it. Existing Blueprint component layouts and serialized fields are retained; the old overhead-cone mesh/material are replaced only at runtime.

`TransmitMotionEndpointActor` also places its owned-motion arrow on the body's output face and follows the animated body each tick. This remains presentation only; the motion state, preview loop, and Reset ownership are unchanged.

## Direction Policy Boundary (Final v0.4)

Target selection and direction resolution are physically decoupled. Direction policy is a property of the carried Motion, not of the camera or the Target:

- **Ordinary Linear — CameraCanonical**: `UMotionCanonicalDirectionResolver` quantizes gameplay camera yaw against fixed world X/Y axes and camera pitch against Up/Down thresholds, producing one of six world-axis `ProjectedWorldDirection` values with the existing hysteresis model. Carried direction is validated as state data, not used for selection; Capture/Carry retain it. The source-relative v0.3 interpretation is corrected by the 2026-09-06 human-PIE finding.
- **Boss High Motion — PreserveSource**: direction stays locked to the committed Charger Dash world direction and bypasses the camera resolver. The Charger grants `PreserveSource` in `FMotionState.DirectionPolicy` when its Dash commits; the interactor bypasses CameraCanonical for that carried state. The policy must not be inferred from magnitude or `SourceId`.
- **Preview = Commit is policy-independent**: whichever policy applies, the interactor computes the world direction once and carries it inside `FMotionTransferContext.DirectionResolution`; Preview and Commit consume the same result.
- **`RequiredCanonicalDirection`**: receivers may declare one of the six canonical directions; a mismatch is `IncompatibleDirection` and never consumes Player Motion. This is a compatibility/regression capability, not the Zone 2 core mechanic.
- **Directional Carrier (implemented)**: the ordinary-Linear Target role that accepts any of the six resolved directions and moves itself — the Actor, not a child presentation mesh — in world space. It must use deterministic swept collision, stop on blocking collision, remain a valid Source for re-capture, and be restored by the existing Room Reset snapshot.

The direction-policy seam lives in `FMotionState` and the existing `FMotionTransferContext` path. Resolution occurs once per policy; atomic ownership mutation stays inside `TryMoveBetween`.

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
        ├── Ordinary Linear → CameraCanonical: gameplay camera yaw / pitch
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

The formal content line is `Jason/L_Transmit_v01` (2026-09-06). Older implementation inventories at `1fb96ea` and `d9b8c4a` are superseded; historical planning and validation remain in Git and `Docs/dev/`.

- `UMotionTransferComponent` owns Motion state, atomic transfer, rejection preservation and Reset snapshots. `IMotionTransferable::Call*` keeps native and Blueprint Actor paths consistent.
- `UMotionInteractorComponent` owns camera-based target acquisition and the preview shared by commit. Ordinary motion resolves against six world axes; Charger High Motion keeps its committed Dash axis.
- `ATransmitDirectionalCarrierActor` moves its collision root in world space, stops on swept blocking collision, permits re-capture and restores through the existing room Reset.
- `ATransmitBridgeSlab`, `ATransmitRam` and `ATransmitArenaCharger` are concrete L_Transmit content roles. The delivered relay locks its existing Motion in place and arms the Ram. Each consumed High Motion drives one Ram stroke; the first impact fractures the gate, the second opens it.
- `ATransmitLevelDirector` observes those real actors and chooses objective/transition state. It activates the encounter only after arming and reaching the arena, ends the threat after impact two, and completes after the player crosses the exit marker. It does not own or transfer Motion.

### Level retry and presentation boundary

`R` retains the existing whole-room Reset transaction. `Backspace` calls the director's local retry:

- Learn restarts the whole room.
- Route restores the player's empty carry state and the original Route Source/Carrier states and transforms, preserving the completed bridge.
- Weaponize preserves the delivered relay, armed Ram and committed gate impacts, and restores the player to arena entry. The Charger restarts only while fewer than two impacts are committed.
- If the route resource is stored outside the Route retry group, local retry falls back to whole-room Reset, avoiding a second copy of that resource. Completion retry also starts a fresh full run.

`OnFlowChanged` and `OnLocalRetry` fire after their observable state changes. Ram `OnArmed` fires after carrier permission locking; `OnImpact` fires after hit count and gate mutation. The HUD reads the director and actual interaction preview; it explains objectives and rejection without granting interaction eligibility. Dedicated presentation may observe these events and references but must not supply its own gameplay state.

`Scripts/Editor/author_ltransmit_flow.py` patches the scoped formal map idempotently. The older full graybox builder is a historical bootstrap, not a safe way to update an authored candidate. Gameplay/resource checks, Editor authoring checks, and human readability acceptance are distinct evidence layers; current results belong in `STATE.md` rather than this architecture record.

### Added practice and reuse passage (2026-09-06)

`Scripts/Editor/extend_ltransmit_pacing.py` adds two introductory bridges and a post-dock passage containing two bridges that share one recoverable ordinary Motion. All use existing endpoint/slab capabilities. The original Learn, Route and Boss actors remain; the authoring script protects their transforms and only opens two baseline walls and relocates PlayerStart.

The director observes concrete `Transmit.Pacing.LearnA/LearnB/RouteA/RouteB` tags for tutorial text. Actors tagged `Transmit.Pacing.Transition` form the narrow pre-arena retry group: their initial transforms and existing Motion snapshots are restored while the main dock and Ram stay complete. An externally held practice resource escalates to the existing full Reset. Entering the arena restores the original arena-retry behavior. The director never writes Motion state directly. Full-run timing resets on R, includes local retries and freezes for the completion heading.

### Vertical layout and compact work-order guidance (2026-09-07)

`author_ltransmit_vertical.py` patches only the existing formal map. It creates a
3m arrival overlook and 4.8m return loop, raises the existing two-bridge reuse
passage and its single source by 6m, and connects the banks with ordinary swept
CharacterMovement ramps. The moving actors retain their existing world-space
movement and reset snapshots. Original L2 and Boss transforms remain unchanged.
All baseline actors are retained; the authoring evidence records exact changes.

`ATransmitHUD` presents a compact work order, actual carry status, and contextual
E/Q affordances from the existing eligibility preview. The director still chooses
objectives from real participant state. Detailed guidance displays for 12 seconds
after a change and remains available while Tab is held. A new run start resets
that presentation timer, including R while the objective is unchanged. No input
mapping, reflected authoring property or gameplay permission is introduced.
Scene plaques use the existing material palette and simple host-hardware geometry;
world naming does not change Motion state, magnitude or compatibility.

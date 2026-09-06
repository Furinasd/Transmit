# A — Level & Flow integration

## Active production authority

2026-09-06 task: https://www.notion.so/3d36dbf617ac812a9bd5ec6b1a6ad11a
User authorizes content layout, gameplay metrics, encounter, guidance and recovery iteration;
old geometry/30-degree route/per-phase stop restrictions are superseded for this task.
Core single ownership, atomic rejection preservation, ordinary CameraCanonical and High
Motion PreserveSource remain unchanged. Ely judges the complete experience.

## Baseline and ownership

- A: `/Users/ely/Documents/Unreal Projects/passely`, `Jason/L_Transmit_v01`.
- Starting SHA: `bf1d0aa19c9eb377802e664cd89d0a44c82d20ef`, clean.
- B: `/Users/ely/.codex/worktrees/0155/passely`, `Jason/visual-presentation`, same checkpoint.
- A alone writes formal map, shared gameplay classes, startup/package config.
- B owns dedicated Presentation code/assets, preview map and assembly script.
- Shared window: start 2026-09-06 11:32 UTC, deadline 2026-09-07 07:32 UTC;
  final integration reserve starts 03:32 UTC. Earlier jointly recorded deadline wins.
- Editor: A currently owns formal-project launch/write access; B coordinates a handoff
  before using the shared GUI. Independent commandlet builds do not transfer GUI ownership.

## First playable increment

1. Keep first bridge causal interaction; add opening/step-specific objectives.
2. Replace obsolete diagonal route with +X send, chase, re-capture, +Y reroute and dock.
3. Stage-specific failure recovery, explicit full restart R, distinct first/second impact goals.
4. Receive B ready commits early, assemble on formal actors, run continuous main-map traversal.
5. Early Mac packaging attempt; final source/Editor/package evidence kept distinct.

## Stable integration identities

Actor labels retained: `Learn_Source`, `Learn_BridgeSlab`, `Route_Source`, `Route_Carrier`,
`Route_DockMarker`, `Weaponize_Ram`, `Weaponize_Charger`, `Weaponize_Gate`,
`Weaponize_EntryMarker`, `Weaponize_ExitMarker`, `Flow_Director`, `Flow_Reset`.
Motion ParticipantIds: `Learn_Source`, `Learn.Bridge`, `Route_Source`, `Route.Carrier`,
`Weaponize.Ram`, `Weaponize.Charger`. Bind by object references/IDs, never stale coordinates.

Current real events: Motion `OnMotionTransaction` / `OnMotionStateChanged` /
`OnMotionConsumed`; Charger `StateMachine` transitions; Ram `bArmed`, `Hits`.
Full reset: `Flow_Reset.RequestRoomReset`, `OnPreRoomReset` and `OnPostRoomReset`.
A will add a narrow director flow-step observation and retry event, and explicit Ram
armed/impact events for B; no generic event framework. B may initially observe current
real fields while those additions compile.

## Historical status — first increment

In production. Baseline docs STATE/ARCHITECTURE contain historical missing-capability
claims; existing source and map supersede those inventory claims. No current-run
build/PIE/package success asserted yet. Formal map delta saved: one changed binary, `Content/Transmit/Maps/L_Transmit.umap`.
First new Editor build passed. First runtime harness exposed old same-frame camera
setup after the camera-selection update; harness now waits for the gameplay POV tick.
Live flow validation in progress.


## Added interfaces (first increment)

- `ATransmitChargerActor::GetDashDirection()` gives authored normalized world dash axis.
- `ATransmitRam::OnArmed`, `OnImpact(int32 ImpactNumber)` broadcast real state changes.
- `ATransmitLevelDirector::GetFlowStep()`, `IsComplete()`, `OnFlowChanged`, `OnLocalRetry`.
- `RequestLocalRetry()` restores current stage original resources and safe player stance;
  completed bridge, delivered carrier and committed gate impact remain. Full R is unchanged.
- New marker labels: `Flow_RouteEntry`, `Flow_CatchMarker`; dock now near (5350,950,85),
  relay catch near (5350,0,85), route reroute is canonical +Y, no diagonal semantics.
- A owns a minimal readable HUD objective strip and a Backspace local retry binding.
  B should avoid replacing shared HUD; dedicated world/audio/FX remain independently owned.


## Increment A1 — ready for B integration

- Latest Mac Editor module build succeeded, including dash getters, event ordering,
  Backspace retry binding and out-of-stage route-resource ownership guard.
- Formal map changed alone among binary assets; existing Blueprint assets unchanged.
- Continuous actual CharacterMovement/Interactor run reached Director completion:
  `Saved/LTransmitEvidence/run-1788695227.json` (52.665 game seconds; not human duration).
- Failure-injected actual traversal passed Route fall / arena hit after first impact /
  continuation to completion / completed full Reset / repeated Reset:
  `Saved/LTransmitEvidence/run-1788695391.json` (66.292 game seconds).
- Those runs precede the final event-order/input-binding/ownership-guard additions and
  end-platform geometry; those additions are built, with matching runtime recheck due
  after B integration. No claim that a build is authoring-surface acceptance.
- Clear actual renders: `Saved/LTransmitEvidence/flow-first.png` and
  `flow-arena-first.png` (the latter captured completion).
- A Editor closed at 11:51 UTC; GUI lease passed to B. No concurrent A GUI/cook.
- Ready interfaces: Dash `GetDashDirection`, `GetDashSpeed`; Ram `OnArmed`, `OnImpact`;
  Director `GetFlowStep`, `IsComplete`, `OnFlowChanged`, `OnLocalRetry`.
- Events now fire after their public state and gate changes; full Reset clears completion
  before flow notification. Exotic backtracking that stashes the route resource outside
  the retry group uses full Reset, preserving single ownership instead of duplicating it.

## A2 integration preparation

- B1 `16b9e8d` integrated as `bb7d1ac`; 13 assets hydrated, original WAV sources present. B2 still pending.
- Additional Flow corrections: gate-open retry keeps the Charger stopped; local retry notification follows objective recomputation; full Reset clears stale retry feedback.
- HUD rejection explanations and larger wrapped objective/hint typography implemented; latest Editor build passed (`/private/tmp/transmit-flow-hud-build.log`, 7.81 seconds).
- New `validate_ltransmit_reset_edges.py` schedules real-Interactor Ram-stroke full Reset and external route-owner fallback checks; runtime execution pending GUI return.
- Mac Development build/file Cook/IoStore Stage succeeded (`/private/tmp/transmit-flow-package-file.log`, 164.72 seconds). Full staged app copied to `Saved/LTransmitCandidate/A1-Verified/Transmit.app`, signature verified, independently launched L_Transmit using Metal SM6. Native R and E observed. A1 package predates B2 and newest HUD. Closed after smoke.
- Reusable `Scripts/package_ltransmit_mac.sh` verifies/copies full staged bundle; do not ship the incomplete earlier UAT archive. Packaging config selects formal map only and file-based cooked output.
- B retains GUI lease for current preview validation, then returns it for A formal-map integration.

## A3 — integrated candidate content

- B2 `dfedd6b` integrated as `9b2ec1c`; B evidence update as `903c724`. Formal map uses the actual Presentation Rig, five materials and eight original cues. B remains isolated and frozen.
- A authored material hierarchy, eight explicit cable corner references, a Ram operating mark with a gate-facing approach, and the open exit frame. Corrected Python Rotator positional arguments that had pitched guide strips and the arena retry marker instead of setting yaw.
- BP_TransmitCharacter retains its existing 400 cm boom and 90 degree FOV; only SocketOffset changes to (0,70,50), exposing the Source and bridge together. This is the second A-owned binary alongside L_Transmit.umap.
- Director holds an ObserveImpact objective through the real Ram stroke. Existing flow enum values are preserved; the new value is appended. Core transfer/ownership semantics are unchanged.
- Final Editor module build passed: `/private/tmp/transmit-impact-flow-build.log` (12.17 s). Transmit automation: 25 passed, zero warning/failure/not-run, `Saved/LTransmitEvidence/CandidateTests/index.json`.
- Saved shoulder-camera clean traversal: `run-1788699847.json`, 66.342 game seconds, actual CharacterMovement and MotionInteractor, no teleport/resource injection. Runtime frames: `Saved/LTransmitEvidence/candidate-1788699780/`, opening through completion, including both impacts at +0.45/+1.3 seconds. Script time does not measure human playtime.
- Recovery `run-1788699643.json` passed Route fall, actual Charger collision after hit one, progress retention, open-gate retry with Charger inactive, completion and repeated R. Failure teleports are explicitly labeled.
- Reset-during-stroke `run-1788699252.json` and external Route-owner fallback `run-1788699339.json` passed using real interaction transactions; exact initial resource ownership restored.
- Native Backspace `run-1788699903.json` passed at Route: bridge retained, source restored, carrier/player empty, retry camera pitch zero.
- Fresh Editor process reloaded 123 formal-map actors, eight cable references and the serialized camera. Five Blueprints compiled BS_UP_TO_DATE; Map Check 0 errors/0 warnings. Fresh PIE instance confirmed SocketOffset (0,70,50). Evidence `/private/tmp/transmit-final-reload.log`.
- Full Details/inline-struct edit-save-reopen authoring smoke remains a human gate. Rig selection/Details generation was observed, but native coordinate control did not reliably reach nested fields. API save/reopen and builds are not a substitute.
- Final Mac packaging/launch evidence will follow against the A3 content commit. No Win64 validation or human experience acceptance is claimed.

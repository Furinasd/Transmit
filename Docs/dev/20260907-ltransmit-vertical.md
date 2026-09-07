# Vertical maintenance route and compact guidance — 2026-09-07

The user authorizes substantial layout changes and modern minimal teaching,
referencing [STANDARD 04｜任务、世界与演出](https://app.notion.com/p/a1d6dbf617ac83f091088183c88004ba).

## Scoped result

- Arrival starts on a 3m inspection platform with the first bridge and source in
  view, followed by an 800cm descent. The initial player position is (-10000,0,400).
- The existing return loop rises 4.8m before descending to the original crossing.
- The two service bridges, single shared source, banks and stops are raised 6m.
  A 43m approach ramp and 13m departure ramp connect the deck to existing gameplay.
  The local checkpoint remains before the climb; it cannot skip either bridge.
- Route markings and guards follow the new walking surfaces. Core L2 chase,
  docking, Ram, Charger and gate transforms are protected.
- Existing twelve tutorial boards become concise mounted maintenance plaques.
  Chip packages, capacitors, screws, solder pads and heatsink fins establish the
  giant-host setting using existing engine primitives and project materials.
- HUD separates work order, contextual action and loaded state. Detailed guidance
  appears on state changes, fades after twelve seconds and is recallable with Tab.
  R reopens the lesson even when restarting at the same objective. Completion is
  the restored connection/current work order, not the end of the entire world.

Only binary asset: `Content/Transmit/Maps/L_Transmit.umap`. All 408 baseline actors
remain. No Blueprint, input, material asset, global lighting or project setting is
changed. English facility copy follows the existing demo language; a Chinese UI,
full voiced satire, inspector reveal and ordinary trial impact are not delivered
by this iteration.

## UE feature choice

Local engine Build.version is UE 5.8.2, CL 56702186. Existing settings already use
Lumen GI/reflections and Virtual Shadow Maps. The authored hardware silhouettes,
layered ramps and moving bridges use that existing dynamic presentation. This
iteration does not claim a new 5.8-exclusive rendering capability or a Nanite
performance improvement. References: [Epic Lumen](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-global-illumination-and-reflections-in-unreal-engine),
[Epic Virtual Shadow Maps](https://dev.epicgames.com/documentation/en-us/unreal-engine/virtual-shadow-maps-in-unreal-engine).

## Validation

Evidence and final results are recorded below after execution. Gameplay traversal
uses CharacterMovement and MotionInteractor. Failure recovery explicitly labels
its fall/collision teleports; clean traversal never teleports or injects Motion.
Screenshots must come from possessed PIE, not the unrelated Editor camera.

## Human inspection

Play from L_Transmit/PlayerStart. Check whether arrival communicates a circuit
board and maintenance identity, whether the inspection loop rewards looking back,
whether the upper service route remains interesting, and whether E/Q/Tab guidance
is readable at the preferred resolution. First-play 5–7 minute timing and feel
remain human acceptance, not a claim derived from scripted traversal.

### Saved layout evidence

- Retained all 408 baseline actors; saved map contains 571 actors. Author inventory:
  `Saved/LTransmitEvidence/Vertical/authoring.json`. Baseline numeric transforms:
  `baseline.json`. Reauthoring requires this original inventory; the script refuses
  to invent a fresh baseline from an already modified map.
- Saved/reloaded Map Check: 0 errors, 0 warnings.
- Clean possessed floating PIE traversal: **173.957 game seconds**, **175.327 wall
  seconds**, completed through the exit. Movement samples cover capsule-center Z
  from 92.1 to 772cm. The latter includes standing on the bridge end stop, not an
  authored 7.72m deck. This is scripted timing, not first-play duration or a fair
  performance comparison with the previous editor session.
- The first capture harness attempted a non-exposed PlayerController Python method.
  It was stopped and replaced with SystemLibrary console dispatch to the explicit
  player controller. The complete rerun succeeded. Actual screenshots include the
  possessed character and HUD; earlier Editor-camera screenshots are not reused.
- Combined recovery: **205.841 game seconds**, successful. It covers a Route fall,
  a service-deck fall immediately after sending the first bridge, actual Charger
  collision after impact one, local retry after gate opening, completion, and
  repeated full resets. Initial sources remain exactly five unique owners. Both
  service bridges restore to Z=625 and the source to Z=720; repeated R returns the
  player to the raised arrival surface (capsule center Z=392.15).
- Final Mac Editor build passed in 11.84s. All 25 discovered Transmit tests passed
  through the Editor automation API (0 failed/skipped, no test errors/warnings),
  `Saved/LTransmitEvidence/Vertical/automation.json`. The startup console test
  queue waited for interactive FPS; it is not the source of this passed result.
- Final HUD observed at a 1280x720 game viewport: readable enlarged objective and
  wrapped lesson, empty/loaded state and contextual action. Screenshots confirm the
  lesson fades and reappears after a full reset at the unchanged first objective.
  Final-build E capture and Q bridge transfer also passed; Tab hold remains a
  human input check. Evidence: `Vertical/FinalHUD/`.
- First package attempt completed cooking work but correctly failed on a logged
  MCP port-8000 bind error caused by the concurrent validation Editor. The error
  was not suppressed; the validation process was closed before retrying. No plugin
  or project configuration was changed.
- Final Mac Build/Cook/Stage passed in **53.02 seconds**, exit code 0. The complete
  staged app was copied by the existing packaging workflow and passed deep/strict
  codesign verification. Candidate: `Saved/LTransmitCandidate/20260907-vertical-Mac/Transmit.app`.
  Source is the current uncommitted working tree; provenance and playtest notes
  are stored beside the app.
- Independently launched the packaged Mac Development app with Metal SM6. Observed
  the new hardware silhouettes and final HUD in its real window. A native R key
  restored the raised starting camera/player state and the initial lesson. Full
  standalone traversal remains unverified; full traversal evidence is possessed PIE.

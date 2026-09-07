# L_Transmit wayfinding checkpoint — 2026-09-07

Scope: combine `Jason/presentation-juice` (`5d6b4d9`) with the saved pacing expansion (`003d497`), then improve decisions and traversal readability. This is a playable checkpoint, not human acceptance of pacing or presentation.

## Implemented

- Kept the expanded map during local branch integration. Replayed the presentation branch's map-local geometry/material pass and void backdrop through Unreal Editor against the current geometry. Integrated its capture/transfer/impact FOV feedback and cancellation cleanup.
- Added neutral ground chevrons through arrival, both return turns, the south service departure, the resource-carry passage and the arena approach. Markings stop at puzzle banks; they do not draw a walkable route through gaps or the blocked original shortcut.
- Marked four safe bridge operating positions and one reverse-facing reclaim position. The west-bank mark specifically avoids sending the service slab while standing on it.
- Added 11 low outer guards on long transit edges and turn ends. Puzzle mouths, source access, the relay catch, and Boss movement space remain open.
- Revised eight instruction signs. The departure sign distinguishes the power cable from the player route; the reuse landing explicitly asks the player to look back and carry the same resource onward.
- Corrected Learn tutorial stage selection at the second source and added the return-passage instruction. Loaded/unloaded reuse hints now identify the appropriate operating bank, movement wait and far-bank reclaim.
- Follow-up from observed runtime frames: move seven signs off the camera/walking line and add non-colliding dark backplates to the 12 pacing signs. No new gameplay capability or reflected property.

## Protected scope

Only binary asset written: `Content/Transmit/Maps/L_Transmit.umap`.

All original and pacing gameplay actors, sources, bridges, PlayerStart, checkpoint/reset configuration, relay and Boss transforms are retained. Seven text-only signs are repositioned. No Beat is removed or folded. Existing Blueprint/material assets, input assets, project settings, rendering CVars and lighting/exposure actors are unchanged. The already-authored presentation branch fog is replayed locally, not replaced by new global settings.

`optimize_ltransmit_wayfinding.py` compares numeric transform values before save, rather than Unreal Python string representations containing temporary object addresses. The initial string comparison correctly prevented saving but reported address-only differences; the numeric comparison retains the protection. Evidence is under `Saved/LTransmitEvidence/Wayfinding/`.

## Validation and timing

- Mac Editor build passed (11.38 seconds); final contextual-text adjustment rebuilt successfully in 7.67 seconds.
- First saved-map reload: Map Check 0 errors / 0 warnings.
- First clean traversal after route/guard changes: **148.754 game seconds**, 148.966 wall seconds, success through the completion marker. Actual CharacterMovement and MotionInteractor; no actor teleport or Motion injection.
- Previous expanded baseline: 148.805 game seconds. Delta: **−0.051 seconds**, effectively unchanged. Original pre-expansion baseline remains 66.342 seconds.
- Combined recovery passed in 178.393 game seconds: original Route fall, shared-resource passage fall after sending its bridge, actual Charger collision after the first impact, preserved progress, completion and repeated full resets with five unique initial resources.
- Final integrated feedback traversal passed: `run-1788713680.json`, 165.923 game seconds including observation stops, one local-retry probe and three full-reset probes. FOV observed from 88.158 to 92.209 degrees around a 90-degree baseline; all four cancellations restored FOV within 0.01 degrees and cleared pulse/sound pools. Evidence: `Saved/JuiceEvidence/runtime.json`.
- Existing Transmit automation: **25 passed, 0 failed, 0 skipped, 0 errors/warnings**, `Saved/LTransmitEvidence/Wayfinding/automation.json`.
- After the final text-only placement/alignment adjustment, saved/unloaded/reloaded `L_Transmit`: Map Check **0 errors / 0 warnings**, all 161 baseline actors retained, all gameplay transforms numerically unchanged, twelve centered non-colliding sign backplates verified. Seven player-camera views observed (`FinalSigns/`); these explicitly use viewpoint teleports and make no traversal claim. No gameplay collision, resource or C++ changes followed the complete traversal/recovery checks.
- The final map contains 408 actors: the 161 pacing baseline actors, 130 replayed presentation actors, and 117 wayfinding pieces (including 11 guards and 12 sign backplates). No baseline actor is deleted.
- No new standalone package is produced for this local Editor checkpoint; the previous package predates these changes.

This checkpoint is saved as a local merge commit on `Jason/L_Transmit_v01`, retaining both branch histories. No remote push or PR change is part of this task.

## Remaining human gates

First-play 5–7 minute duration is still unverified. Ground guidance does not by itself establish that budget. Human play should specifically check whether the long service passage sustains attention, whether the reverse-facing reclaim cue communicates resource reuse, whether signs are readable at the player's preferred resolution, and whether FOV impulses are comfortable. Full standalone traversal and earlier nested Details/Blueprint authoring smoke remain separate gates. This task makes no Windows compatibility claim.

## Inspection

- `ATransmitLevelDirector::GetPacingTutorial`: observation-driven contextual text; no ownership mutation.
- `ATransmitPresentationRig::UpdateCameraFeedback` / `ClearCameraFeedback`: bounded additive FOV that removes only its own offset.
- Editor: `L_Transmit`, folders `Wayfinding`, `Pacing_Gameplay`, `Presentation/GeometryFinish`; compare the west-bank operation, the return turns and the arena arrival in PIE.

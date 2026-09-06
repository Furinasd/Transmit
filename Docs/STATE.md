# Agent Handoff

> Operational snapshot, not design authority. Gameplay rules live in `DESIGN_CONTRACT.md`; architecture lives in `ARCHITECTURE.md`. Historical evidence remains in `Docs/dev/` and Git.

Last inspected: 2026-09-06, L_Transmit candidate production in progress.

## Current truth

- Active integration line: `Jason/L_Transmit_v01`, baseline `bf1d0aa`; A1 flow/layout commit `58aa225`, B1 material/audio integration `bb7d1ac`.
- Formal map and default Editor/game startup: `/Game/Transmit/Maps/L_Transmit`.
- Learn → Route → Weaponize now has real bridge traversal, +X send/chase/re-capture, +Y relay docking, armed Ram, two captured Charger impacts and an exit/completion state.
- Objective HUD and Backspace local retry are implemented. R remains full restart. Core Motion ownership, atomic rejection, CameraCanonical and High Motion PreserveSource remain intact.
- Dedicated B presentation is being integrated; B1 palette/audio assets are present. B2 runtime rig/assembly is pending. Current map is not yet the final visual candidate.

## Evidence already obtained

- Mac Editor build succeeded for A1 and later flow fixes/HUD revision.
- A1 `Transmit` automation: 25/25 passed, `Saved/LTransmitEvidence/A1Tests/index.json`.
- Actual CharacterMovement/Interactor clean traversal reached completion (52.665 game seconds): `Saved/LTransmitEvidence/run-1788695227.json`.
- Failure-injected traversal passed Route fall, Charger collision after first impact, retained progress, completion and repeated full Reset: `Saved/LTransmitEvidence/run-1788695391.json`.
- Those runtime runs precede final flow fixes and end-frame geometry. Integrated runtime recheck is required; script time is not a human playtime measurement.
- Early Mac Development Build/Cook/Stage passed. A complete 1.0 GB staged `.app` copy independently launched the formal map using Metal SM6; native R/E input was observed. This early package predates B2 and is not the final delivery.

## Current validation work

1. Integrate B2 into the formal map through actor references, retain A map ownership.
2. Run clean traversal and failure recovery on the integrated map, including post-open-gate Backspace and full Reset during a Ram stroke.
3. Check the backtracking case where the Bridge owns the Route resource: local retry must choose full Reset and leave exactly one owner per original resource.
4. Reopen assets, Blueprint compile, Map Check, and inspect the changed reflection/Details authoring surface.
5. Capture continuous runtime images and rebuild/launch the final Mac candidate; inspect its actual startup and playable flow.

## Packaging

`bash Scripts/package_ltransmit_mac.sh [new-output-directory]` runs the current project through Mac Development BuildCookRun with file-based cook output and IoStore, then copies and verifies the complete staged app. `TRANSMIT_ENGINE_DIR` may override the local UE 5.8 directory.

The first local Zen cooked-output attempt could not read its Mac oplog (HTTP 404, then fallback stalled). Explicit `-SkipZenStore` with `bUseZenStore=False` completed; ordinary DDC still uses the existing engine setup. This is a chosen packaging backend, not an engine Zen root-cause fix.

UAT's early `-archive` output omitted the bundle's `Contents/UE` data despite reporting success. The verified standalone source is `Saved/StagedBuilds/Mac/passely.app`. The script checks executable and Paks before copying that bundle; do not deliver the incomplete early archive.

## Human gates and limitations

- Ely still judges complete player understanding, pacing, readability, camera, feel, audio balance and visual acceptance. The 5–7 minute experience remains a hypothesis.
- Editor authoring surface validation is pending; build/tests do not certify Details generation or save/reopen behavior.
- Only local macOS delivery is being validated. No current Win64 package or cross-platform compatibility claim.
- Integration details, ownership and ready batches: `Handoff/A_LevelFlow.md` and B's separate-worktree `Handoff/B_Visual.md`.

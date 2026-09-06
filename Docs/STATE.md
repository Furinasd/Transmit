# Agent Handoff

> Operational snapshot, not design authority. Gameplay rules live in `DESIGN_CONTRACT.md`; architecture lives in `ARCHITECTURE.md`. Historical evidence remains in `Docs/dev/` and Git.

Last inspected: 2026-09-07, Boss rail correction and Chinese guidance. Human feel/readability and packaged performance acceptance remain open.

## Current Boss correction checkpoint

- Latest approved behavior: player-directed Boss commitment, guaranteed gate-front recovery, actual Zone 2 carrier as a constant-speed rail counterweapon. A physical circular hit damages Boss/gate; a miss consumes energy without progress. Holding captured energy pauses Boss at home and does not duplicate the resource.
- Chinese objectives, contextual teaching, Notion 04 narrative, Boss phase/readout, dock camera glance and map-local progression sun are integrated. Existing four-state FSM and bounded presentation pools are reused; no new gameplay framework or VFX simulation system.
- Only binary changed in this pass: `Content/Transmit/Maps/L_Transmit.umap` (577 actors). Original connection-floor overlaps now meet at edges; obsolete arena blockers/instructions are hidden through Editor. No project rendering settings changed.
- Current candidate: `Saved/LTransmitCandidate/20260907-boss-rail-Mac/Transmit.app`. Mac Development Build/Cook/IoStore Stage passed in 50.98 s; complete bundle signature verification passed. Old candidates were not overwritten. `PLAYTEST.md` documents controls and the remaining human checks.
- Independent candidate process initialized `L_Transmit` and loaded packaged Roboto/DroidSansFallback fonts (`Boss/standalone-bootstrap.json`). Mac was locked, so native standalone visual/input inspection and foreground profiling remain open; PIE screenshots do not certify those gates.
- Final Mac Editor build passed (`/tmp/transmit-boss-final-build.log`, 8.44 s). Final automation: **26 passed, 0 failed**, `Saved/LTransmitEvidence/Boss/automation-final.json`, including Boss re-aim/return/single-resource regression.
- Final production gameplay run: `Saved/LTransmitEvidence/Boss/full-run.json`, 187.830 game seconds, actual movement/E/Q from spawn to exit without teleport/resource injection. `full-reset.json` confirms original carrier transform/permissions, empty resources, Boss home and gate collision restored. The final HUD contrast-only change was observed separately in `Boss/final-boss-hud00000.png`.
- `Boss/retry.json`: actual first hit followed by local retry during the next stroke preserved dock/damage and cancelled pending impact; full reset during another stroke restored carrier endpoint/resources/gate. Explicit dock/position setup is separate from the clean full-run evidence.
- TestChamber fixture: `Boss/chamber.json` confirms aimed dash, miss return, same-carrier ping-pong, actual E/Q, off-axis miss, two real hits and contact return. Fixtures were unsaved and did not change the chamber asset.
- Chinese and floor visuals: `Boss/01-chinese00001.png`, `02-overlap-repair00001.png`; arena impact: `07-fracture00001.png`. These are observed runtime images, not aesthetic acceptance.
- Ram Details opened/expanded, RailSpeed changed through Editor API to 181 and observed in Slate Details, restored to 180, saved/reloaded. Evidence `Boss/details.png`, `details-181.json`. Other pre-existing nested struct/Blueprint human authoring gates below remain separate.
- Zone 2 combination proposals remain paper-only in `Docs/dev/20260907-boss-rail-polish.md`. Performance budget must be measured in a foreground packaged build; throttled PIE frame samples are not a GPU benchmark.

## Previous vertical layout checkpoint (historical candidate)

- User-authorized major layout iteration on `Jason/L_Transmit_v01`, currently an
  uncommitted working-tree change. Source of world context: STANDARD Notion 04.
- A 3m arrival platform, 4.8m inspection loop and 6m service/reuse deck connect by
  traversable ramps. All 408 baseline actors remain; map total is 571. Core L2,
  Ram, Charger and gate transforms are unchanged. Twelve large tutorial boards
  become maintenance plaques; giant host-hardware silhouettes establish scale.
- Compact work-order HUD, actual loaded status, target-dependent E/Q prompts,
  12-second contextual lessons and held-Tab help. Same-stage R restores the lesson.
  World text retains the current demo's English language.
- Only changed binary: `Content/Transmit/Maps/L_Transmit.umap`. No Blueprint,
  material asset, project/plugin configuration or global lighting edits.
- Final Mac Editor build passed; 25/25 Transmit automation passed without test
  warnings/errors. Saved-map Map Check: 0 errors / 0 warnings. Clean possessed PIE
  completed in 173.957 game seconds; combined recovery completed in 205.841 seconds,
  covering Route/service falls, hit-one collision, open-gate retry and repeated R.
  Five initial resources retain one owner each. Final HUD fade/restart and first
  capture/transfer were independently observed after the last code build.
- Current Mac candidate: `Saved/LTransmitCandidate/20260907-vertical-Mac/Transmit.app`.
  Final Build/Cook/Stage passed in 53.02s; bundle signature verification passed.
  The standalone app independently rendered the new map/HUD with Metal SM6;
  native R restored the raised starting view and lesson. Full standalone traversal
  remains a human check; full automated traversal evidence is PIE.
- Evidence and remaining gates: `dev/20260907-ltransmit-vertical.md` and
  `Saved/LTransmitEvidence/Vertical/`. Human gates: first-play understanding/timing,
  elevated-route feel, preferred-resolution text, held-Tab help and aesthetics.
  Full Notion voice/narrative staging and ordinary trial-impact behavior are not
  part of this delivered layout/UI scope. No Windows compatibility claim.

## Previous wayfinding checkpoint

- Integration line: `Jason/L_Transmit_v01`, combining saved pacing `003d497` with `Jason/presentation-juice` at `5d6b4d9`. The expanded map is retained; the other branch's geometry/backdrop edits are replayed through Editor.
- Added continuous neutral route marks, four safe bridge operating positions, a reverse-facing reclaim position and 11 low transit guards. Revised eight signs, moved seven text-only signs away from the walking/camera line and added 12 non-colliding sign backplates. Fixed practice/return/reuse HUD stage instructions.
- All gameplay actor transforms, existing Beats, L2, Boss, resources and retry rules remain. Only binary change is `Content/Transmit/Maps/L_Transmit.umap`; Blueprint/material assets and project settings are unchanged.
- Mac Editor build, 25/25 automation, clean traversal (148.754s), combined recovery (178.393s), final feedback traversal/reset cancellation (165.923s) and final map/sign readback passed. Map Check: 0 errors / 0 warnings. Previous comparable clean baseline: 148.805s. Details: `dev/20260907-ltransmit-wayfinding.md`.
- Current entry is the saved `L_Transmit` in Mac Editor. The `20260906-pacing-Mac` standalone package predates this checkpoint and must not be used to assess these changes.
- Human acceptance still includes 5–7 minute first-play pacing, independent route/resource-reuse understanding, preferred-resolution text readability and camera comfort. See the checkpoint report for evidence and remaining gates.

## Previous pacing expansion

- Saved as `003d497` on `Jason/L_Transmit_v01`, based on `67ed5cb`.
- Four added bridge applications, three ordinary sources, a shared-resource reuse passage and twelve instruction signs. All 123 original actors remain, including the complete L2 and two-hit Boss chain. No visual polish or core-rule changes.
- Only binary change: `Content/Transmit/Maps/L_Transmit.umap`. New PlayerStart is `(-10000,0,100)`.
- Mac Editor build, saved-map Map Check (0 errors/warnings), 25/25 automation, real full traversal and combined failure/repeated-reset validation passed.
- Scripted clean completion is **148.805 game seconds**, versus baseline 66.342s (+82.463s, 2.243x). This does **not** establish the 5–7 minute human target. The completion heading now shows full-run time including local retries.
- Details, evidence, timing budget and open acceptance: `dev/20260906-ltransmit-pacing.md`.
- New Mac package: `Saved/LTransmitCandidate/20260906-pacing-Mac/Transmit.app`. Build/Cook/Stage passed in 61.87s; signature verification passed. Standalone Metal SM6 startup rendered the new Learn practice area and native R restored the starting view/resources. Full standalone playthrough remains a human check. Do not use the old `ea081ff` package to judge this expansion.
- Human gate: first-play duration, reuse comprehension, attention through the longer passages and text readability. The added-resource external-owner fallback has code review but no separate runtime induction in this suite.

## Historical integrated candidate below

The remaining snapshot describes the earlier `ea081ff` package and its provenance, not the expanded playable. Its successful checks are historical; the checks above correspond to this working tree.

## Current truth

- Integration line: `Jason/L_Transmit_v01`. Candidate content: `ea081ff`; starting baseline `bf1d0aa`.
- B1 `16b9e8d` / B2 `dfedd6b` are integrated as `bb7d1ac` / `9b2ec1c`; B evidence update is `903c724`.
- Formal Editor/game startup: `/Game/Transmit/Maps/L_Transmit`.
- Complete Learn → Route → Weaponize: bridge traversal, +X send/chase/re-capture, +Y docking, armed Ram, two captured Charger impacts, open exit and completion.
- Objective HUD, impact observation, Backspace local retry and R full restart are active. Core single ownership, atomic rejection preservation, CameraCanonical and High Motion PreserveSource remain unchanged.
- Formal map now consumes B's ceramic/graphite/motion palette, original mechanical audio and runtime Presentation Rig. A adds authored cable corners, guide/material hierarchy, a gate-facing Ram approach and shoulder camera.

## Verified evidence

- Mac Editor/Game build and final Build/Cook/IoStore Stage passed. Final package completed in 56.42 seconds from clean `ea081ff`; signature verification passed.
- Final Transmit automation: 25 passed, zero warnings/failures/not-run, `Saved/LTransmitEvidence/CandidateTests/index.json`.
- Clean saved-camera traversal: `run-1788699847.json`, actual CharacterMovement and MotionInteractor, no teleports or resource injection, completion in 66.342 game seconds. This is not human playtime.
- Runtime images: `Saved/LTransmitEvidence/candidate-1788699780/`, opening to completion, with first/second impact and delayed reaction frames.
- Failure recovery: `run-1788699643.json` covers Route fall, actual Charger collision after hit one, preserved progress, safe retry after gate opens, completion and repeated R.
- Full R during Ram stroke (`run-1788699252.json`) and external Route-resource owner fallback (`run-1788699339.json`) restore exactly the initial unique resources.
- Native Backspace at Route (`run-1788699903.json`) retained the completed bridge and restored the local source/carrier/player correctly.
- Fresh Editor process reloaded 123 actors, eight cable anchors and saved camera (0,70,50). Five Blueprints compiled BS_UP_TO_DATE; Map Check 0 errors/0 warnings. Fresh PIE instance confirmed camera serialization.
- Final standalone Mac app independently rendered L_Transmit with Metal SM6 and integrated presentation. Native R → E capture → Q bridge transfer/movement → Backspace restoration was observed through the actual window. Full standalone traversal is still a human check; the continuous completion evidence above is PIE.

## Candidate entry

`Saved/LTransmitCandidate/ea081ff-Mac/Transmit.app`

Double-click the local Mac app. WASD move, mouse aim, Space jump, E capture, Q transfer, Backspace retry the current area, R restart. The app is a local Development candidate, not a notarized public distribution. No Win64 package was produced.

The candidate directory contains `package.log`, `source-head.txt`, `source-status.txt` (clean at packaging) and `PLAYTEST.md`. Final documentation-only commits do not change the packaged gameplay/assets.

## Changed binary assets

A: `Content/Transmit/Maps/L_Transmit.umap` and `Content/Transmit/Blueprints/BP_TransmitCharacter.uasset` (existing camera boom SocketOffset only).

B: five materials (`M_Ceramic`, `M_Graphite`, `M_Impact`, `M_Inlay`, `M_Motion`), eight SoundWaves (`Capture`, `Complete`, `Dock`, `Intercept`, `RamImpact1`, `RamImpact2`, `Telegraph`, `Transfer`) under `Content/Transmit/Presentation/`, plus `Maps/L_PresentationPreview.umap`. Total: 16 binary assets compared with the task baseline. B's preview map is not the playable entry.

## Packaging

`bash Scripts/package_ltransmit_mac.sh [new-output-directory]` runs the current project through Mac Development BuildCookRun with file-based cook output and IoStore, then copies and verifies the complete staged app. `TRANSMIT_ENGINE_DIR` may override the local UE 5.8 directory.

The first local Zen cooked-output attempt could not read its Mac oplog (HTTP 404, then fallback stalled). Explicit `-SkipZenStore` with `bUseZenStore=False` completed; ordinary DDC still uses the existing engine setup. This is a chosen packaging backend, not an engine Zen root-cause fix.

UAT's early `-archive` output omitted the bundle's `Contents/UE` data despite reporting success. The verified standalone source is `Saved/StagedBuilds/Mac/passely.app`. The script checks executable and Paks before copying that bundle; do not deliver the incomplete early archive.

## Human gates and limitations

- Ely still judges complete player understanding, pacing, readability, camera, feel, audio balance and visual acceptance. The 5–7 minute experience remains a hypothesis.
- Rig selection and Details generation were observed. Full nested-struct/inline authoring edit → save → reopen remains unverified because native coordinate control did not reliably address those fields. Check Director/Ram exposed fields, Rig Cues and inherited Blueprint defaults in Editor; API reload/build/Map Check do not close this gate.
- Only local macOS delivery is being validated. No current Win64 package or cross-platform compatibility claim.
- Integration details, evidence and ownership: `Handoff/A_LevelFlow.md` and `Handoff/B_Visual.md`. No additional implementation is scheduled before Ely reviews the candidate.

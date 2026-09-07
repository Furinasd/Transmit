# Agent Handoff

> Operational snapshot, not design authority. Gameplay rules live in `DESIGN_CONTRACT.md`; architecture lives in `ARCHITECTURE.md`. Historical evidence remains in `Docs/dev/` and Git.

Last inspected: 2026-09-07, weak guidance / target-locked counterstroke refinement. Human first-play feel and foreground packaged profiling remain open.

## Delivery checkpoint — 2026-09-07

- Packaging / PR preparation continues on `Jason/L_Transmit_v01`, targeting `main`.
- Windows packaging entry: `Scripts/package_ltransmit_windows.ps1`; instructions: `Docs/PACKAGING.md`. No Windows host or registered repository Actions runner is available in this session; Win64 build/cook/runtime remain unverified.
- MCP / AllToolsets are restricted to Editor targets for packaging; gameplay and binary assets are unchanged by this delivery pass.
- User-reported ordinary Motion carried into Arena blocks Boss capture; unresolved. SUV / folding-screen identity and Boss staging are unfinished design work, recorded in `Docs/submission/KNOWN_GAPS.md`. Notion synchronization is pending explicit destination approval. These are not fixes or accepted visual changes.

## Current experience refinement checkpoint

- Latest user-approved rules supersede the older fixed-axis counterstroke: Q locks the vector from the rail carrier to Boss's current position; flight stays straight. Preview, body and impact effects share this output direction. Arena capture alone is limited to a 1000 cm horizontal radius from the player (approximately two authored floor tiles), with the same aimed E / visibility checks.
- Default HUD now shows interaction/resource state and brief chapter notices. Solutions are held-Tab only. Three session checkpoints restore Learn, Route and actual Arena entry; remote docking retains the service-deck retry. Arena retry preserves dock and committed gate damage; R resets everything.
- The same C-01 performs an ordinary commissioning contact before the inspector appears, then becomes the folded-screen counterweapon. Notion 04's worker/brand/interface fiction is expressed through sparse Chinese subtitles, seven static sign designs and eight mounted instances; no dialogue framework or new puzzle system. Zone 2 combination proposals remain paper-only.
- Gate and clear aperture widened to 10 m, gate height 9 m. Two cover pillars sit behind the counterweapon rail. Nominal dash cycle shortened to 5.65 s; return still guarantees the gate-front anchor. Existing map sun now has more distinct cool bus / low warm arena / restored warm exit stages; existing bounded effects and interruptible camera are reused.
- Mac Editor build passed, `/tmp/transmit-experience-final-build.log` (6.81 s). Final Transmit automation: **27 passed / 0 failed**, `Saved/LTransmitEvidence/Experience/automation.json`. Native checks cover target locking after launch and arena radius.
- Production full run completed in **166.754 game seconds**, actual movement/E/Q from spawn to exit without teleport or resource injection. Both successful Q shots started off-axis (rail offset >=320 cm); preview direction was asserted against Boss. `Experience/full-run.json` and `full-reset.json` confirm completion and full reset. This is an automated practiced route, not a first-play timing claim.
- Unsaved TestChamber fixture (`Experience/chamber.json`) exercised real capture/transfer, post-launch evasion/miss, two circular impacts, rail reversal and contact return. Saved `L_TestChamber` was not changed.
- Checkpoint/cover fixture passed in **38.006 s**, `Experience/checkpoints.json`: falls in all three zones, service retry before entry, ordinary trial with zero High damage, real first hit preserved after falling, pillar interception (Boss minimum X=6786.14; sheltered player not moved), and full restart. Explicit setup positions/resources in this fixture are separate from the clean full run.
- Binary scope: `Content/Transmit/Maps/L_Transmit.umap` (**617 actors**) and **14 new assets** under `Content/Transmit/Presentation/Narrative` (`T_`/`M_` WorkOrder, Launch, Standard, Quiet, Access, Carrier, Exit). No TestChamber, Blueprint, input, project/plugin or renderer configuration changes.
- Floor repair retains all original collision bounds and substitutes 28 disjoint render pieces for 15 clipped floor meshes, retaining four untouched original display meshes. Static scan of 133 large horizontal faces found zero coplanar intersections (`Experience/floor-overlaps.json`). This does not certify every potential visual artifact; actual route screenshots are in the same evidence folder.
- Final sign front-face, mounting and duplicate-message corrections were checked separately (`Experience/story-*.png`). Four redundant render-only copies were then removed in favour of their geometrically identical original faces; final Map Check: **0 errors / 0 warnings**, 617 actors. No gameplay transforms or collision bounds changed after the clean run.
- Current candidate: `Saved/LTransmitCandidate/20260907-quiet-counter-Mac/Transmit.app`. Mac Development Build/Cook/IoStore Stage passed in **59.29 s**, complete bundle signature verification passed. Old candidates remain. `PLAYTEST.md` describes controls, changed rules and human gates.
- New independent candidate loaded `/Game/Transmit/Maps/L_Transmit`, initialized packaged Chinese fallback fonts and created its Metal SM6 native game window. Evidence: `Experience/standalone-bootstrap.json`; log `/tmp/transmit-experience-standalone.log`. Native visual/control traversal and foreground performance were not certified from this bootstrap; the observed gameplay images are from PIE. The new candidate is left running for user inspection.

## Previous Boss correction checkpoint (historical candidate)

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

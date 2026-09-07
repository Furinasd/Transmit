# Experience refinement — 2026-09-07

Source: user playtest feedback and [STANDARD 04](https://app.notion.com/p/a1d6dbf617ac83f091088183c88004ba). Latest player-targeted Boss and target-locked counterstroke supersede the document's older fixed-axis combat wording. No L4/L5 implementation.

## Observe

User confirmed visuals and Boss return, but reported excessive solution guidance, incorrect counter direction, floor flicker and missing zone checkpoints. Current HEAD before edits: `fd8aa4c`; clean working tree.

## Reproduce

Source path reproduced a concrete mismatch: Ram used `FixedAxis`, while the interactor/effects displayed captured PreserveSource direction. Thus an off-centre carrier could not aim toward Boss. Geometry inspection found 23 floor-plane intersection pairs plus four decorative cap crosses. The repaired floor subset is recorded in `Saved/LTransmitEvidence/Experience/map-authoring.json`. These are static geometric findings, not a claim that every flickering pixel was visually reproduced.

## Localize

Ram owns target conversion, not the global captured state. Director owns zone entry/retry and story timing. HUD owns optional guidance. The floor issue is duplicated render faces, not a renderer-CVar problem. Existing collision union is intentionally retained.

## Form Hypothesis

A Q-time Boss aim vector shared by preview and the physical stroke removes the directional mismatch; locking it rather than homing preserves meaningful misses. Disjoint visible floor rectangles remove equal-depth surfaces while preserving traversal. Entering the room, rather than arming its device remotely, is the correct Zone 3 checkpoint boundary.

## Test Hypothesis

- Native regression checks cover off-centre aim, locked flight after target movement, atomic consumption and the two-tile capture boundary.
- The temporary native World must initialize actors before Unreal will dispatch Actor UFUNCTION delegates (`AActor::ProcessEvent` checks `AreActorsInitialized`). The first fixture omitted this; assertions were retained and initialization corrected.
- Unsaved TestChamber fixtures use real E/Q, test a post-launch evade, two successful circular impacts, rail reversal and player-contact return. They never modify the saved chamber asset.
- Production full-run, checkpoint/cover fixtures and scene screenshots are tracked separately in `Saved/LTransmitEvidence/Experience/`.

## Fix Smallest Cause

- Same C-01 actor, renamed in fiction as 苹果折叠屏. Its renderer is a slim screen silhouette; no replacement projectile, skeletal mesh, or homing system. Ram caches the Boss once, computes aim at Q, and locks it through flight. Preview shows receiver output without mutating the incoming PreserveSource state.
- Arena-only Charger capture checks 1000 cm horizontally from `Context.Requester`, matching approximately two 500 cm tiles. Ordinary puzzle reach, selection/occlusion and E/Q verbs are unchanged.
- Nominal Boss cycle: idle .65, warning .95, dash 1.65, recovery 2.4 seconds, dash speed 1250. Return: .16-second hold + .55-second interpolation. Held dash ownership still prevents duplication.
- Three session checkpoints: Learn start, Route entry, actual Arena entry. Zone 2's existing service-deck retry preserves docking. Backspace/falls preserve earlier-zone success and committed gate damage. Exceptional externally stored resource conflicts retain the pre-existing full-reset safety fallback rather than duplicate a resource. No save-game/disk system.
- Solutions are Tab-only; brief zone notices and state feedback remain. Default HUD does not auto-repeat the instruction sequence.
- The incoming ordinary route powers a physical commissioning cycle after arena entry; it taps the gate without counting High damage. Boss then appears. This is an automatic commissioning stroke, not an added input verb. Three sparse narrator lines and short institution/Boss fragments follow real progression. Gatekeeping sign retires after the second hit, exposing the original rule.
- Seven typeset texture/material pairs plus eight in-world sign instances. No imported real-person voice, video, faction mechanics or dialogue framework.
- Larger 10 m × 9 m gate, matching aperture and exit wall clearance. Two pillars sit behind the rail (x=6480), outside the rail-to-home fan (x>=7020). Floor collisions retain original bounds; 28 non-colliding render pieces replace 15 clipped visible floor meshes; four unchanged original display meshes are retained. Static surface scan: 133 planes, zero coplanar overlaps in the tested large-horizontal-face set.
- Light stages: initial daylight → cooler, dimmer bus → low warm interface → brighter restored connection. Changes touch only the tagged map sun; no project renderer setting changes. Existing bounded effect pools, audio and one temporary camera are reused. Camera glance occurs at actual rail arrival or final impact and yields to input.

## Re-run Original Failure

Evidence remains layered: native tests/build establish checked rules; real PIE establishes the exercised physical behavior; screen captures establish observed text/composition only. Human first-play discovery, dodge/capture feel, satire comprehension and foreground packaged profiling remain separate acceptance gates. Final outcomes and candidate path are recorded in `Docs/STATE.md` after validation, not inferred from this design note.

Final content inspection corrected all eight sign normals, reused the three existing stands, mounted the exit message on its frame and the original notice inside the corridor, and hid duplicate English brand plaques. `Experience/story-*.png` are explicit flying-camera/player-hidden visual fixtures, not first-person traversal evidence. A final Map Check flagged four coincident hidden-collision/render copies; the authoring script now retains the four unchanged original visible floors and removes only their newly created redundant copies through EditorActorSubsystem. Final map: 617 actors, 0 errors / 0 warnings (`Experience/map-check.json`). Collision and visible surface geometry/materials are equivalent to the tested full-run version.

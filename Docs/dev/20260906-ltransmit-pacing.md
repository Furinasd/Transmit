# L_Transmit pacing expansion — 2026-09-06

Baseline: clean `67ed5cb` on `Jason/L_Transmit_v01`. This is an additive gameplay-content pass, not a visual production phase. Design target: a 5–7 minute complete first experience; five-player blind-test median remains the human acceptance criterion.

## Content and preservation

All 123 baseline actors remain. The original Learn bridge, complete L2 send → chase → catch → re-capture → +Y docking chain, armed Ram, Charger cycle, two captured High Motion impacts, gate and exit remain in place. No core Motion, targeting, canonical direction, Boss signature, input, material asset, audio asset or project configuration is changed.

The only binary write is `Content/Transmit/Maps/L_Transmit.umap`:

- L1 starts at a new arrival platform. An eastbound bridge teaches source stopping and target movement; a second northbound bridge applies camera direction/Preview before returning to the original Learn beat.
- After the original L2 relay docks, a south service passage leads to two more crossings. A single new source moves the first bridge. The player crosses, reclaims that same Motion from the far bank, carries it north and redirects it into the second bridge. The second crossing deliberately has no source.
- The approach to L3 explains the threat/resource relationship, Telegraph versus committed Dash, locked direction, Ram delivery and two-hit progress. The Boss fight itself is retained, not counted as a newly implemented combat encounter.
- Four bridge actors, three independent ordinary sources, traversal floors/stops/separator and 12 instruction signs make up 38 added actors. The three modified baseline actors are PlayerStart and two walls opened for the added connections. Existing L2/Boss gameplay transforms and configuration are protected by the incremental script.

`extend_ltransmit_pacing.py` is idempotent and only saves the formal map. It never invokes the historical destructive graybox builder. The original actor inventory and changed/additional labels are retained under `Saved/LTransmitEvidence/Pacing/`.

## Runtime responsibility

Existing `ATransmitBridgeSlab`, `ATransmitMotionEndpointActor` and their Motion components implement all transfers. The director observes authored tags to select tutorials; it does not grant or rewrite Motion.

During the added post-dock transition, Backspace/fall restores only the new source and two bridges and returns the player to the service-route entrance. The original dock and armed Ram remain completed. If a practice resource is held outside that restore group, retry falls back to the full authoritative Reset to prevent duplication. Once the original arena is entered, its existing local retry and committed-impact preservation remain active.

The completion heading displays whole-run game time. R starts its timer again; local retries remain included. This is distinct from the old arena-only log. The traversal harness also records wall time independently of game time.

No reflection exposure, new Blueprint properties or instancing semantics are added. Existing authoring-surface human gates from the baseline are not retroactively closed by this pass.

## Timing and acceptance

Historical baseline evidence: 66.342 game seconds for a scripted, fully informed traversal. It is not a human baseline.

The first-play pacing budget is L1 100–130 seconds, L2 and its reuse transition 120–150 seconds, and L3 80–110 seconds (300–390 seconds total). These are design hypotheses, not measured player results. Experienced/scripted players know all solutions and can complete faster.

The expanded clean scripted run completed in **148.805 game seconds** (152.987 wall seconds), versus 66.342 seconds at baseline: **+82.463 seconds / 2.243×**. Its milestones were original Learn exit at 57.861s, original dock at 72.807s, arena entry waypoint at 120.903s and completion at 148.791s. This is a fully informed traversal, not a 5–7 minute player result.

**The requested human-duration target is still unverified.** No forced waits, movement-speed reductions, extra required Boss hits or enemy-count inflation are used to manufacture duration. A player who already knows the route can finish below the target band. Longer passages, hint comprehension and pacing need first-play evidence before further tuning.

## Validation record

- Mac Editor build passed (19.39s), with the final whole-run timer and tutorial changes.
- Saved-map Map Check: zero errors / zero warnings.
- `Saved/LTransmitEvidence/run-1788705174.json`: full clean traversal passed, real CharacterMovement and MotionInteractor, no teleports or resource injection; all added crossings and original L2/Boss/exit completed.
- `Saved/LTransmitEvidence/run-1788705440.json`: combined recovery passed. It exercises original Route fall, transition fall after sending, real Charger collision after impact one, retry after opening the gate and four full resets. All four new bridges return to their starts and five independent resources have exactly one owner each. Its 199.981 game seconds include deliberate failures and resets and are not a clean pacing measurement.
- `Saved/LTransmitEvidence/Pacing/automation.json`: 25 passed, zero failures/skips/warnings.
- `Saved/LTransmitEvidence/Pacing/suite.json`: both full traversal and combined recovery completed successfully.
- New local package: `Saved/LTransmitCandidate/20260906-pacing-Mac/Transmit.app`. Build/Cook/Stage succeeded in 61.87s and signature verification passed. Standalone Metal SM6 startup visibly showed the added Learn practice and native R restarted it. Full-chain runtime evidence is PIE, not a full manual standalone playthrough.
- The external-owner fallback for the new practice resource was reviewed in code; that additional branch was not separately induced in this runtime suite.

An initial run (`run-1788704767.json`) found the first service-bridge operating position was on the moving slab; swept collision stopped it against the player. The near bank was extended west and the instruction/path moved the player to the bank before transfer. Core collision behavior was preserved. The failure is retained alongside the corrected successful run.

## Human inspection

Play the saved formal map from the new PlayerStart, using E/Q, Backspace and R normally. Read the final timer, record failures and where a hint was needed. Check whether the longer passages sustain attention, whether reclaiming the first service bridge is understood, and whether the pre-Boss text is readable in movement. The added content remains plain gameplay geometry; art acceptance is outside this pass. Use the new `20260906-pacing-Mac/Transmit.app` package; `ea081ff` content predates this expansion. The new app is left open at the introduction. Press R immediately before a timed first play.

# L_Transmit v0.1 candidate review

## 1. Inherited game

Transmit already had a transactional Motion grammar: Capture moves one owned state into Player, Preview resolves eligibility/direction, and Transfer stores or consumes that same state. Ordinary Linear motion follows CameraCanonical resolution. Charger Dash carries a preserved committed world direction and High magnitude. Carrier movement, collision stop, recapture and snapshot Reset existed before this experiment. The missing product was an authored continuous progression using those capabilities.

## 2. Repository contradictions

The baseline is `4db656bbacc15bbabfedebffc9485e22be26853f`, the requested development HEAD. DESIGN_CONTRACT/ARCHITECTURE/STATE status prose lagged at `1fb96ea`: actor-path Preview repair (`2a17171`), Charger PreserveSource (`a5166de`), directional Carrier (`d9b8c4a`) and Test Chamber staging (`4db656b`) were already present. README's missing-carrier-fixture claim was also stale. Code, assets, fresh core tests and live Test Chamber observations established the baseline. These historical status paragraphs were not treated as permission to reimplement existing systems.

## 3. Spatial thesis

One interrupted transmission line changes meaning from walkable deck to remote payload to final impact. Geometry creates the verbs: a gap needs a moving bridge; a low culvert admits Motion's carrier while rejecting the Player; a tangent catch face enables a new outbound direction; arena baffles expose a fixed dash lane that supplies the only qualifying final resource. The experience stays in one map with physical transitions.

## 4. Zones

- **Learn:** The source is offset from the bridge sightline. Transferring ordinary motion advances a 10m slab across a 9m break to a physical stop. The resulting traversal change is visible from the approach.
- **Route:** The carrier goes through a 150cm-high throat while the standing Player follows an outer gallery. At the catch station the Player recaptures from its west side and reroutes 30 degrees along a diagonal rail. Physical carrier delivery to the dock latches the Ram armed; holding Motion and walking ahead cannot arm it.
- **Weaponize:** Baffles channel the Player across the Charger lane. The fixed +X Dash is capturable only during commitment, remains +X when carried around cover, and powers the aligned Ram. Two distinct deliveries produce visible impacts; the second lifts the gate and exposes the exit. Falling or a dash collision resets the run.

## 5. Implementation

Four level-scoped native actors provide the missing content glue: a swept box-root bridge slab; a dock-armed Ram with signature rejection, impact timing and gate restoration; a Charger subclass that returns to its authored bay between cycles; and a director for encounter activation, fall/hit recovery and completion. The original Motion transaction, direction resolver, Carrier and Charger implementations are unchanged. Authoring is a scoped Editor Python script using existing basic shapes and six graybox materials. No new gameplay Blueprint family was necessary.

## 6. Inherited runtime findings

The Test Chamber Charger parked against blocking geometry and subsequent cycles had no useful dash travel/capture window. A level-specific return-to-bay behavior resolves that reproducible encounter blocker. Target selection aims at actor roots, while animated source bodies can occlude targets; placement avoids this. The camera-relative resolver does not promise arbitrary 90-degree steering; vertical incoming motion also dominates pitch selection. The route uses a validated horizontal 30-degree turn without changing the frozen resolver. The inherited Carrier capsule/body setup was unsuitable for a broad walkable deck, hence the level-specific box-root slab.

## 7. Iterations after PIE

The detailed design log records observations and rejected alternatives. The initial bridge overlapped the source plinth and its deck exceeded step height; source offset and deck thickness were corrected. A perpendicular catch wall stopped the outgoing state; the tangent face now stops incoming +X while permitting outgoing +30 degrees. A floor gap caused a real fall/reset; a west approach then hit a curb, so the final approach loops south of the diagonal rail. Baffles were extended to remove a west-side bypass. A synchronous fall-reset latch bug in the new director was removed. Ram armed/completed feedback was added. Foreground validation then found that a scripted post-recapture waypoint intersected `Route_SouthSill_3`; the actual transfer worked from outside, so only the harness stance was corrected. Final automation initially exposed an invalid test setup that tried to Grant into a non-provider Player; the fixture now uses explicit initial state while retaining Player permissions.

## 8. Deliberate cuts

No new Motion type, second enemy, health, weapon combat, Behavior Tree/EQS, framework, PCG, BeatMarker expansion, narrative, environment art pass, checkpoint framework, audio production or packaging. Global Reset is sufficient for a first candidate and remains a pacing cost to evaluate.

## 9. Known limitations

The 5–7 minute first-play duration remains unverified. The final foreground perfect-information scripted run took 52.375 game seconds (an earlier background run took 63.366); it proves traversal and interaction, not discovery time or comprehension. The fixed-lane Charger returns directly to its bay rather than navigating. Off-axis/vertical mistakes may require R. Readability, camera comfort, the second impact's value and whole-run reset frustration need human play. Validation is macOS Editor only, with no packaged or other-platform claim.

## 10. Exact changed scope

Relative to the development baseline:

- `Content/Transmit/Maps/L_Transmit.umap`
- `Content/Transmit/ExperimentV01/M_Amber.uasset`
- `Content/Transmit/ExperimentV01/M_Chalk.uasset`
- `Content/Transmit/ExperimentV01/M_Cyan.uasset`
- `Content/Transmit/ExperimentV01/M_Red.uasset`
- `Content/Transmit/ExperimentV01/M_Slate.uasset`
- `Content/Transmit/ExperimentV01/M_White.uasset`
- `Source/passely/Public/Transmit/TransmitLevelActors.h`
- `Source/passely/Private/Transmit/TransmitLevelActors.cpp`
- `Source/passely/Private/Tests/TransmitPlayableTests.cpp`
- `Scripts/Editor/build_ltransmit.py`
- `Scripts/Editor/validate_ltransmit_pie.py`
- `Scripts/Editor/validate_ltransmit_recovery.py`
- `Docs/dev/20260905-ltransmit-v01.md`
- `Docs/dev/20260906-ltransmit-v01-report.md`
- `Docs/dev/20260906-ltransmit-v01-evidence.json`

Seven Unreal binary assets total. Existing gameplay Blueprints, other maps, Config and project/plugin settings are outside the changed scope. Work remains on `exp/astra-ltransmit-v01`; no merge into the development branch.

## 11. Validation

Final macOS Editor build succeeded in 8.75 seconds. All 17 Transmit automation tests passed, including the new Ram rejection/ownership test. Five inherited gameplay Blueprints compiled BS_UP_TO_DATE without saving. Fresh L_Transmit load Map Check reported 0 errors and 0 warnings. The final foreground continuous PIE run reached the exit in 52.375 game seconds with two Ram impacts. The clean-run harness uses actual CharacterMovement and MotionInteractor calls, without teleporting, injecting ownership or direct component transfers. Recovery probes separately teleport the live pawn only to induce three falls and two dash collisions. Those probes are not represented as a player walkthrough. All final recovery checks passed: completed-gate reset, three consecutive falls, two consecutive dash collisions, restored source/carrier/bridge ownership and transforms, Ram disarm and gate collision restoration. A standing capsule hit `Route_LowRoof_0`; the carrier sphere cleared the same throat. Final gate state before reset was Hits=2, collision=false, Z=1000.

Machine-readable source excerpts and complete run/recovery events: [validation evidence](20260906-ltransmit-v01-evidence.json). The detailed [design and iteration log](20260905-ltransmit-v01.md) retains failed attempts. `git diff --check` passed. Editor remains on L_Transmit with PIE stopped. The final run displayed an Editor system-memory-pressure notification; this is not a performance certification.

## 12. Ely's judgment

Open `/Game/Transmit/Maps/L_Transmit`, Play from PlayerStart, use the existing movement/Capture/Transfer controls and R to restart. First play without reading the automation waypoints. Judge whether the first transfer clearly changes traversal, the low passage makes the separate Motion path understandable, the catch face invites recapture/reroute, the dock visibly arms a later goal, and the Charger becomes a wanted resource rather than an unexplained obstacle. Record blind completion time and moments of hesitation. Decide whether the two impacts earn their repetition and whether full-run Reset is acceptable. The candidate's functional evidence does not substitute for those decisions. Controls: WASD/mouse, E Capture, Q Transfer, R full restart.

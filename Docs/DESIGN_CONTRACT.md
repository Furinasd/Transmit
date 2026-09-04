# Transmit Design Contract

> Status: Active under the **Final v0.4 design lock** (promoted 2026-09-05). This is the engineering boundary, not a mirror of Notion. Implemented behavior and rules that are promoted but not yet implemented are stated explicitly below; a promotion does not make repository runtime truth until its matching delta is implemented and verified.

## Authority

Only rules explicitly promoted into this document are authorized for implementation. Notion remains the source for design intent, hypotheses, evidence, alternatives, and rejected ideas. An experiment, illustrative parameter, or paper-level possibility is not an implementation requirement unless promoted here.

When this contract lists an open decision, preserve the seam and avoid choosing a permanent behavior without evidence.

## Final v0.4 Capability Status (2026-09-05)

| Capability | Repository status |
| --- | --- |
| Ordinary Linear — CameraCanonical six-direction reroute | Implemented and automated-verified (v0.3 core). Contract now scopes it as the ordinary policy. |
| Ownership / atomic transaction / rejection preservation / Reset semantics / targeting / canonical resolver implementation | Frozen; no redesign authorized by v0.4. |
| Directional Carrier | **Promoted, not implemented.** No carrier actor, Blueprint, map wiring, world-space movement/collision behavior, re-capture path, or test exists. |
| Boss High Motion direction policy | **Promoted, not implemented.** Charger Dash Capture preserves direction through Capture, but Transfer still enters the ordinary camera resolver; no source-authored direction policy exists. |
| `RequiredCanonicalDirection` | Implemented capability, demoted to compatibility/regression only. No longer the core Zone 2 mechanic. Actor-path Preview/Commit consistency still has an open defect. |
| Final playable shape | **Promoted, future content.** `L_Transmit` does not exist; `L_TestChamber` remains the regression environment. |

## Core Fantasy

The player cannot directly move arbitrary objects. The player manipulates **Motion State** that already exists in the world:

`Read → Capture → Carry → Transfer → Convert → Function`

Combat produces or interrupts motion. Puzzles route, transform, and consume the same motion. The project must not split these into separate rule systems.

## Motion State

Motion State is the only transferable resource.

| Field | Contract |
| --- | --- |
| Type | P0 implements `Linear` only. `Angular` is a P1 paper/extension target. `Oscillation` is interface-level future scope only. |
| Direction / Axis | Always travels with the state. Ordinary Linear is rerouted at Transfer by **CameraCanonical** into one of six canonical world directions (Forward / Back / Left / Right / Up / Down). **Boss High Motion is the explicit exception**: its direction stays locked to the captured Charger Dash world direction and bypasses camera reroute. Aim selects targets but never supplies a free direction. |
| Magnitude | Always travels with the state. Continuous velocity versus gameplay tiers remains open. |
| Period / Phase | Out of P0 scope; reserved only for future periodic motion. |
| Source / Debug identity | Must be traceable for diagnostics and ownership verification; it is not a second gameplay resource. |

Motion State is gameplay-authored motion, not a promise of strict mass, friction, or energy conservation.

## Core Verbs

| Verb | Contract | Minimum readable result |
| --- | --- | --- |
| Read | Identify type, direction, and magnitude before committing input. | World-space motion trail/arrow/rhythm plus target response; no tutorial text required. |
| Capture | Remove Motion State from a valid moving Source and assign that same state to the Player. | Source stops or staggers; motion visibly rewinds/transfers to the Player. |
| Carry | Player temporarily owns exactly one Motion State. | A persistent 3D indicator communicates type, direction, and magnitude. |
| Transfer | Move the carried state to a valid Target. Ordinary Linear direction is deterministically rerouted by the gameplay camera (CameraCanonical); Boss High Motion preserves the captured Dash world direction (PreserveSource) and bypasses camera reroute. Preview and Commit always use the same resolved world direction. | One continuous causal trail plus immediate Target response; the projected direction is visible before commit. |
| Convert | An explicit environment rule deterministically changes direction or type. | Input and output signatures are simultaneously readable from the converter geometry. |

## Direction Semantics (Final v0.4, promoted 2026-09-05)

### Ordinary Linear — CameraCanonical (unchanged v0.3 policy)

- **Capture preserves Source Motion**: direction and magnitude are taken from the Source unchanged.
- **Transfer reroutes by gameplay camera**: the carried Linear direction is interpreted against the gameplay camera and quantized to exactly one of Forward / Back / Left / Right / Up / Down; the Target receives the resolved world direction.
- **One resolver, one result**: the same Canonical Direction Resolver drives Preview and Commit. Preview and Transfer may never compute direction or compatibility with different rules.
- **Deterministic and hysteresis-guarded**: horizontal four directions come from camera yaw; Up / Down use an explicit pitch threshold; boundaries keep hysteresis so Forward ↔ Up and adjacent horizontal sectors do not flicker. Identical camera pose + identical carried direction must produce an identical result.
- **Scope**: CameraCanonical applies to Ordinary Linear Motion. It is the default Transfer policy, not a rule that overrides promoted Motion-specific exceptions.

### Directional Carrier (promoted, not implemented)

- A Directional Carrier is the primary ordinary-Linear Target for Zone 1 Learn and Zone 2 Route.
- It accepts ordinary Linear Motion at any of the six canonical directions resolved by CameraCanonical and moves the **Carrier Actor in world space** — not only a child presentation mesh.
- Movement uses deterministic swept collision; a blocking collision stops the Carrier by a stable, repeatable rule.
- A moving Carrier is a valid Motion Source: it can be **captured again**; Capture stops the Carrier and returns the same Motion State to the Player.
- Room Reset restores the Carrier's transform and Motion State through the existing authoritative snapshot semantics.

### `RequiredCanonicalDirection` — compatibility / regression only

- Receivers may still declare a `RequiredCanonicalDirection`; a mismatch is reported as `IncompatibleDirection` (DirectionMismatch) and never consumes Player Motion.
- This remains a system capability for compatibility and regression, **not the core Zone 2 mechanic**. The existing Forward / Up Receiver pair is regression evidence, not the primary proof of the production route puzzle.
- Functional Receivers may consume matching Motion Signatures to trigger functions; new production content must not depend on `RequiredCanonicalDirection` to carry the Route zone.

### Boss High Motion — PreserveSource exception (promoted, not implemented)

- Boss High Motion is captured from the Charger Dash only inside the committed Dash window.
- Direction policy is **PreserveSource**: the captured Motion keeps the committed Dash world direction (the Dash vector).
- It **bypasses CameraCanonical**: camera pose cannot rewrite the preserved direction; the Player chooses Capture timing and Transfer Target, not output direction.
- **Preview == Commit** is policy-independent: Preview and Commit show and apply the same preserved world direction.
- A Ram Block is the promoted Zone 3 Target: it receives Boss High Motion along its fixed axis and converts it into Gate impact.

### Scope of the v0.3 decision record

ADR-003 (`Docs/Decisions/ADR-003-camera-driven-linear-reroute.md`) governs Ordinary Linear Motion. The Boss High Motion exception is recorded as the Final v0.4 amendment to that ADR and does not reopen the v0.3 decision.

## Frozen Semantics (unchanged by v0.4)

- **Ownership**: one Motion State belongs to exactly one owner; Capture / Transfer never duplicate it.
- **Atomic transaction**: a successful transaction clears the previous owner and assigns the next owner as one operation.
- **Rejection preservation**: a rejected request never silently consumes or loses the state and returns a structured, distinguishable reason.
- **Reset semantics**: the room restores the authoritative start snapshot for critical actors and Player carry state; Reset clears transient selection / Preview.
- **Targeting**: stable selection ranking, soft-cone assistance, stickiness, and commit-time revalidation are unchanged.
- **Canonical resolver implementation**: resolver thresholds, hysteresis model, and the six-direction set are frozen.

These semantics are not part of the v0.4 implementation delta. A defect fix that restores them (for example, Preview passing the same direction data that Commit validates) is a repair, not a redesign.

## Player Contract

- Player kit: movement, jump, third-person shoulder-aim selection, Capture, and Transfer; no normal attack.
- Capture and Transfer are explicit actions over one shared target-selection language.
- Soft-cone assistance and target stickiness may improve usability but must not change rules or hide which target won selection.
- Target priority is stable: reticle angle, occlusion, distance, then Motion State compatibility.
- The Player cannot create motion, copy motion, hold more than one state, or redirect motion through free aim; ordinary Transfer reroute is driven by the gameplay camera through the canonical resolver. Boss High Motion is the promoted exception: its preserved Dash world direction is not camera-authored.
- Death, room Reset, and level transition clear Player carrying state and restore the room's authoritative starting snapshot.

## Invariants

1. **Single ownership:** one Motion State belongs to exactly one Source, Player, or Target at a time; Capture and Transfer never duplicate it.
2. **Atomic movement:** a successful transaction clears the previous owner and assigns the next owner as one operation.
3. **Direction preservation:** direction remains unchanged through Capture and Carry; a P0 direction change happens only under an explicit promoted policy — CameraCanonical reroute for Ordinary Linear, or the preserved Charger Dash world direction for Boss High Motion.
4. **Explicit compatibility:** valid and invalid targets are readable before committed input.
5. **Observable failure:** type, direction, magnitude, occlusion, or timing rejection produces a distinct inspectable result; failure never silently consumes the state.
6. **Deterministic conversion:** the same input signature entering the same converter direction produces the same output signature.
7. **Recoverable resources:** critical Sources respawn, loop, or restore on room Reset; eliminating an enemy cannot create an unrecoverable soft lock.
8. **Stable selection:** overlapping targets use deterministic ranking and brief lock stability rather than flickering between candidates.
9. **Gameplay physics:** critical paths constrain bounce, roll, friction, and Chaos variance when uncontrolled physics would break prediction.
10. **Consistent language:** Player, Enemy, and Environment use one visual grammar for state ownership and motion signatures.

## P0 Interaction Matrix

| Interaction | Promoted rule | Zone / status |
| --- | --- | --- |
| Moving Source / Prop → Player | Capture removes its Linear state; the Source stops and Player carries the same direction and magnitude. | Zone 1 Learn — implemented core |
| Player → Directional Carrier | Accepts ordinary Linear Motion at any of the six CameraCanonical directions; moves the Carrier Actor in world space with deterministic swept collision, stops on blocking collision, stays re-capturable, and is restored by Reset. | Zone 1–2 Route — promoted, not implemented |
| Player → Functional Receiver | `RequiredCanonicalDirection` compatibility/regression only: mismatch is `IncompatibleDirection` and never consumes Player Motion. Not the Zone 2 core mechanic. | Regression fixture — implemented |
| Moving Carrier → Redirect Rail | Superseded for P0: Redirect Rail may return as a deterministic environment converter; it is not required by the final shape. | Paper candidate |
| Charging Charger → Player | Capture is legal only inside the committed Dash window; success stops/staggers the Charger and yields high-magnitude Boss High Motion locked to the Dash world direction. | Zone 3 Weaponize — FSM implemented; direction lock promoted, not implemented |
| Boss High Motion → Ram Block | Ram Block receives the preserved Dash world direction and converts it into Gate impact along the fixed attack axis. | Zone 3 Weaponize — promoted, content not implemented |
| Player → Charger | **Open:** direct enemy reception is not authorized; the promoted final chain does not require it. | Open |
| Carrier / Charger → Hazard or Wall | Collision, displacement, stagger, or elimination is a motion consequence, not a direct damage ability. | Zone 3 |

## Paper Extension Contract

- L4 may introduce a deterministic Crank that converts Linear input into Clockwise Angular output.
- An Angular Receiver may validate axis, clockwise/counter-clockwise direction, and magnitude.
- L5 introduces no new verbs or mechanisms; it recombines Source, Carrier, Redirect Rail, Crank, Receiver, and Charger into at least two legal routes.
- P1 paper rules do not authorize P0 implementation before the v0.4 runtime closure and single-map zone validation pass.

## Combat Contract

- Enemy attacks are dynamic motion Sources before they are damage events.
- P0 includes one Charger with `Telegraph → Dash → Recovery` only.
- Capturing a dash changes the Charger from high-speed motion to stop/stagger; it does not directly deal damage.
- A captured Dash yields high-magnitude Boss High Motion locked to the committed Dash world direction; Transfer must preserve that direction (Preview = Commit) rather than camera-reroute it.
- Combat outcomes come from collision, walls, hazards, displacement, and resource routing.
- The loop must retain decisions without health bars, ordinary attacks, or enemy-count difficulty.

## Puzzle Contract

Puzzles ask:

1. What Motion State currently exists?
2. What signature does the Receiver require?
3. Which deterministic direction/type conversion is missing?

They must not collapse into color-slot matching, one-off scripted keys, or aim-based direction rewriting.

## Final Playable Shape (v0.4, promoted 2026-09-05)

The only final playable map is **`L_Transmit`** — one continuous map, not three independent production levels. L1 / L2 / L3 remain design progression IDs inside that map:

| Zone | Progression ID | Learning objective | Required proof |
| --- | --- | --- | --- |
| Zone 1 — Learn | L1 | Motion can be taken from A and given to B and directly change traversal. | Source → Capture → Bridge Slab / traversal change. |
| Zone 2 — Route | L2 | Motion can cross space ahead of the Player, be tracked, re-captured, and re-routed. | Send → Chase → Directional Carrier re-capture → Re-route → Arm Ram. |
| Zone 3 — Weaponize | L3 | The Boss Dash is both a threat and the unique strong enough final resource. | Capture direction-locked Boss High Motion → armed Ram → two-hit Gate Break. |

- `L_Transmit` is future content; it does not exist in the repository yet.
- `L_TestChamber` remains the regression / micro-validation environment only and must not become the submission experience.
- L4 / L5 remain paper extension targets (see Paper Extension Contract).

## Validation Gates (Final v0.4)

The old Gate A / Gate B sequence is superseded by a narrow v0.4 closure. The runtime gate must pass in `L_TestChamber` before `L_Transmit` production:

1. Ordinary Linear regression: six-direction CameraCanonical, hysteresis, mismatch rejection, Preview = Commit.
2. Directional Carrier matrix: Preview direction equals actual Carrier movement; swept blocking stop; moving-Carrier re-capture; Reset restores Carrier / Player / Source.
3. Boss High Motion matrix: captured Dash direction is preserved under multiple camera rotations; Preview = Commit; direction-locked Transfer reaches the Ram Target; Charger returns to its attack cycle; Reset restores FSM, transform, Motion, and Ram state.
4. Repeated Reset cycles show no stale ownership, FSM, Preview, or outcome state.
5. Fresh source-aligned evidence: macOS Editor module build, all `Transmit.MotionTransfer` automation, Blueprint compile/validation, Map Check, and repeated PIE — with human PIE acceptance recorded separately.

After the runtime gate, produce the single continuous `L_Transmit` (Learn → Route → Weaponize). A clean run must support Reset / Fail / Restart and complete in 5–7 minutes. Five-player blind-test targets remain: comprehension 4/5, voluntary reuse 3/5, route/reroute prediction 4/5, threat reframing 3/5, no repeated selection/state-loss failure, and median completion in 5–7 minutes.

## Open Decisions

These are intentionally not promoted as fixed implementation behavior:

- continuous Source velocity versus normalized weak/medium/strong magnitude;
- instant Capture versus a 0.12–0.25 second short attach phase;
- invalid Transfer being blocked, returning state, or adding a short recovery window;
- whether Zone 3 allows Player → Charger Transfer or environment Targets only (the promoted Ram chain does not require it);
- the minimum Angular representation beyond axis, direction, and magnitude.
- Resolved (v0.3, 2026-09-01): camera-driven six-direction reroute at Transfer for Ordinary Linear Motion; see ADR-003.
- Resolved (Final v0.4, 2026-09-05): Directional Carrier is the ordinary-Linear Target role with world-space movement, deterministic swept collision, blocking stop, re-capture, and Reset restoration.
- Resolved (Final v0.4, 2026-09-05): Boss High Motion uses PreserveSource — the committed Charger Dash world direction — bypasses CameraCanonical, and keeps Preview = Commit.
- Resolved (Final v0.4, 2026-09-05): `RequiredCanonicalDirection` is a compatibility/regression capability only and is not the Zone 2 core mechanic.
- Resolved (Final v0.4, 2026-09-05): the final playable is one continuous `L_Transmit` map; L1 / L2 / L3 are progression zones, not three production maps.
- Resolved (Final v0.4, 2026-09-05): ownership, atomic transaction, rejection preservation, Reset semantics, targeting, and the canonical resolver implementation remain frozen.

## Change Rule

Design exploration and evidence remain in Notion. Once a rule is accepted, update this contract first, then reconcile `ARCHITECTURE.md`, the relevant ADR, implementation, tests, and `STATE.md`.

## Promotion Sources

- [Transmit — 米哈游](https://app.notion.com/p/3c96dbf617ac801d86d8f74173199c20)
- [02 — Gameplay Lab](https://app.notion.com/p/3c96dbf617ac8141b2fddc14786d9bd2)
- [07 — Production Workflow & Repo Architecture](https://app.notion.com/p/3ca6dbf617ac81dcb66be87ef08b4ce0)
- [08 — Final Production Plan | v0.4 Delta → L_Transmit → Presentation](https://app.notion.com/p/3ce6dbf617ac81d5b1aff7871934ffb7)
- [Goal 09 — Final Design Lock → L_Transmit Production → Presentation](https://app.notion.com/p/3cf6dbf617ac811e9ed1e798f1931545)

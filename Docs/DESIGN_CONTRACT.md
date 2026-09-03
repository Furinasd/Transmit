# Transmit Design Contract

> Status: Active for the current `Concept → Proof of Fun` phase. This is the engineering boundary, not a mirror of Notion.

## Authority

Only rules explicitly promoted into this document are authorized for implementation. Notion remains the source for design intent, hypotheses, evidence, alternatives, and rejected ideas. An experiment, illustrative parameter, or paper-level possibility is not an implementation requirement unless promoted here.

When this contract lists an open decision, preserve the seam and avoid choosing a permanent behavior without evidence.

## Core Fantasy

The player cannot directly move arbitrary objects. The player manipulates **Motion State** that already exists in the world:

`Read → Capture → Carry → Transfer → Convert → Function`

Combat produces or interrupts motion. Puzzles route, transform, and consume the same motion. The project must not split these into separate rule systems.

## Motion State

Motion State is the only transferable resource.

| Field | Contract |
| --- | --- |
| Type | P0 implements `Linear` only. `Angular` is a P1 paper/extension target. `Oscillation` is interface-level future scope only. |
| Direction / Axis | Always travels with the state. At Transfer, the gameplay camera deterministically reroutes Linear direction into one canonical direction (Forward / Back / Left / Right / Up / Down); aim selects targets but never supplies a free direction. |
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
| Transfer | Move the carried state to a valid Target. Linear direction is deterministically rerouted by the gameplay camera into one canonical direction, and the Target receives that resolved direction. | One continuous causal trail plus immediate Target response; the projected canonical direction is visible before commit. |

## Direction Semantics (v0.3, promoted 2026-09-01)

- **Capture preserves Source Motion**: direction and magnitude are taken from the Source unchanged.
- **Transfer reroutes by gameplay camera**: the carried Linear direction is interpreted against the gameplay camera and quantized to exactly one of Forward / Back / Left / Right / Up / Down; the Target receives the resolved world direction.
- **One resolver, one result**: the same Canonical Direction Resolver drives Preview and Commit. Preview and Transfer may never compute direction or compatibility with different rules.
- **Deterministic and hysteresis-guarded**: horizontal four directions come from camera yaw; Up / Down use an explicit pitch threshold; boundaries keep hysteresis so Forward ↔ Up and adjacent horizontal sectors do not flicker. Identical camera pose + identical carried direction must produce an identical result.
- **Receivers declare canonical direction**: a Receiver's Required Direction is one of the six canonical directions; a mismatch is reported as `IncompatibleDirection` (DirectionMismatch) and never consumes Player Motion.
- **Supersedes pre-v0.3 rule**: "aim never supplies output direction; direction changes only through an environment converter" is superseded for Transfer. Redirect Rail remains a possible deterministic environment converter but is no longer the P0 L2 mechanism (see `ADR-003-camera-driven-linear-reroute.md`).
| Convert | An explicit environment rule deterministically changes direction or type. | Input and output signatures are simultaneously readable from the converter geometry. |

## Player Contract

- Player kit: movement, jump, third-person shoulder-aim selection, Capture, and Transfer; no normal attack.
- Capture and Transfer are explicit actions over one shared target-selection language.
- Soft-cone assistance and target stickiness may improve usability but must not change rules or hide which target won selection.
- Target priority is stable: reticle angle, occlusion, distance, then Motion State compatibility.
- The Player cannot create motion, copy motion, hold more than one state, or redirect motion through free aim; Transfer reroute is driven by the gameplay camera through the canonical resolver, never by an arbitrary aim vector.
- Death, room Reset, and level transition clear Player carrying state and restore the room's authoritative starting snapshot.

## Invariants

1. **Single ownership:** one Motion State belongs to exactly one Source, Player, or Target at a time; Capture and Transfer never duplicate it.
2. **Atomic movement:** a successful transaction clears the previous owner and assigns the next owner as one operation.
3. **Direction preservation:** direction remains unchanged through Capture and Carry; the only P0 direction change is the deterministic camera-driven canonical reroute at Transfer (v0.3).
4. **Explicit compatibility:** valid and invalid targets are readable before committed input.
5. **Observable failure:** type, direction, magnitude, occlusion, or timing rejection produces a distinct inspectable result; failure never silently consumes the state.
6. **Deterministic conversion:** the same input signature entering the same converter direction produces the same output signature.
7. **Recoverable resources:** critical Sources respawn, loop, or restore on room Reset; eliminating an enemy cannot create an unrecoverable soft lock.
8. **Stable selection:** overlapping targets use deterministic ranking and brief lock stability rather than flickering between candidates.
9. **Gameplay physics:** critical paths constrain bounce, roll, friction, and Chaos variance when uncontrolled physics would break prediction.
10. **Consistent language:** Player, Enemy, and Environment use one visual grammar for state ownership and motion signatures.

## P0 Interaction Matrix

| Interaction | Promoted rule | First level |
| --- | --- | --- |
| Moving Source / Prop → Player | Capture removes its Linear state; the Source stops and Player carries the same direction and magnitude. | L1 |
| Player → Transfer Crate / Carrier | A valid Transfer preserves ownership and magnitude while applying the gameplay-camera canonical reroute; arbitrary aim vectors never author direction. | L1 |
| Player → matching Linear Receiver | Receiver accepts only the canonical direction resolved by the gameplay camera (plus compatible magnitude) and consumes it into a function. | L1–L2 |
| Moving Carrier → Redirect Rail | **Superseded for P0:** v0.3 uses camera-driven canonical reroute at Transfer; rail geometry may return as a deterministic environment converter. | L2 candidate |
| Charging Charger → Player | Capture is legal only during the committed dash window; success stops/staggers the Charger and yields its high-magnitude Linear state. | L3 |
| Player → Charger | **Open:** direct enemy reception is not authorized until the L3 target decision is promoted. | L3 candidate |
| Carrier / Charger → Hazard or Wall | Collision, displacement, stagger, or elimination is a motion consequence, not a direct damage ability. | L3 |

## Paper Extension Contract

- L4 may introduce a deterministic Crank that converts Linear input into Clockwise Angular output.
- An Angular Receiver may validate axis, clockwise/counter-clockwise direction, and magnitude.
- L5 introduces no new verbs or mechanisms; it recombines Source, Carrier, Redirect Rail, Crank, Receiver, and Charger into at least two legal routes.
- P1 paper rules do not authorize P0 implementation before L1–L3 pass their gates.

## Combat Contract

- Enemy attacks are dynamic motion Sources before they are damage events.
- P0 includes one Charger with `Telegraph → Dash → Recovery` only.
- Capturing a dash changes the Charger from high-speed motion to stop/stagger; it does not directly deal damage.
- Combat outcomes come from collision, walls, hazards, displacement, and resource routing.
- The loop must retain decisions without health bars, ordinary attacks, or enemy-count difficulty.

## Puzzle Contract

Puzzles ask:

1. What Motion State currently exists?
2. What signature does the Receiver require?
3. Which deterministic direction/type conversion is missing?

They must not collapse into color-slot matching, one-off scripted keys, or aim-based direction rewriting.

## Current Level Usage

All five levels are currently **Paper**, not playable evidence.

| Level | Priority | Duration | Learning objective | Required proof |
| --- | --- | ---: | --- | --- |
| L1 — Capture & Transfer | P0 | 1.5 min | Motion can be taken from A and given to B without copying or losing ownership; Transfer may apply the canonical camera reroute. | Moving Source → Player → Crate → Gate. |
| L2 — Direction Matters | P0 | 2.0 min | Direction belongs to Motion State and changes only through the deterministic camera-driven resolver. | Camera-relative Right Linear → Transfer reroute → Up Linear → Lift (final proof locked in Part 2 brief). |
| L3 — Enemy as Source | P0 | 2.5 min | A Charger dash is both threat and high-magnitude resource. | Capture Dash → Stop Charger → Transfer to environment outcome; direct enemy Target remains open. |
| L4 — Motion Conversion | P1 | 2.5 min | A deterministic mechanism can change motion type. | Right Linear → Crank → Clockwise Angular → Mechanism. |
| L5 — System Synthesis | P1 | 3.0 min | Existing rules form a recoverable network with multiple routes. | Angular brake chain plus redirected Linear lift chain; no new rule. |

## Validation Gates

- Do not expand L2/L3 before the minimal L1 Capture/Carry/Transfer loop is readable and worth repeating.
- Do not add enemies before direct transfer and Relay/Redirect comprehension have been tested.
- L1–L3 must respectively prove toy quality, directional insight, and threat reversal.
- A clean three-level run must support Reset / Fail / Restart and complete in 5–7 minutes.
- Five-player blind test targets: comprehension 4/5, voluntary reuse 3/5, L2 prediction 4/5, threat reframing 3/5, no repeated selection/state-loss failure, and median completion in 5–7 minutes.

## Open Decisions

These are intentionally not promoted as fixed implementation behavior:

- continuous Source velocity versus normalized weak/medium/strong magnitude;
- instant Capture versus a 0.12–0.25 second short attach phase;
- invalid Transfer being blocked, returning state, or adding a short recovery window;
- whether L3 allows Player → Charger Transfer or environment Targets only;
- the minimum Angular representation beyond axis, direction, and magnitude.
- Resolved (v0.3, 2026-09-01): camera-driven six-direction reroute at Transfer; see ADR-003.

## Change Rule

Design exploration and evidence remain in Notion. Once a rule is accepted, update this contract first, then reconcile `ARCHITECTURE.md`, the relevant ADR, implementation, tests, and `STATE.md`.

## Promotion Sources

- [Transmit — 米哈游](https://app.notion.com/p/3c96dbf617ac801d86d8f74173199c20)
- [01 — Vision & Pillars](https://app.notion.com/p/3c96dbf617ac818798d6c45ca669f73c)
- [02 — Gameplay Lab](https://app.notion.com/p/3c96dbf617ac8141b2fd-dc14786d9bd2)
- [03 — Level Design](https://app.notion.com/p/d09793d5089e4296b03282c3ded038c)

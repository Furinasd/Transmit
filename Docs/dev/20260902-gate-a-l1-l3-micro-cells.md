# Gate A L1-L3 Micro-Cell Validation Blueprint

- Timestamp: 2026-09-02 +08:00
- Status: **Superseded 2026-09-05 by the Final v0.4 design lock** — historical v0.3 / Gate A blueprint retained for provenance; not the current plan or authority
- Scope: L1-L3 gameplay capability validation in `L_TestChamber`; no final level production

> [!WARNING]
> **本文档已被 Final v0.4 design lock（2026-09-05）取代。** 以下内容保留为 v0.3 / Gate A 决策与验证历史，不再代表当前排期或实现 authority。Final v0.4 只保留两个 P0 runtime delta：Directional Carrier / Re-capture 与 Boss High Motion direction lock；正式 playable 是单张 `L_Transmit`（Zone 1 Learn → Zone 2 Route → Zone 3 Weaponize），L1 / L2 / L3 不再对应三张生产图。仍继承有效的回归项：ownership、原子事务、rejection preservation、Preview = Commit、Charger timing、Reset。`RequiredCanonicalDirection` 降级为 compatibility / regression capability；High Motion 不再 camera-reroute。当前规则以 `DESIGN_CONTRACT.md` / `ARCHITECTURE.md` / `STATE.md` 为准。

## Goal

Close Gate A by proving that the v0.3 Motion core can support three distinct player-facing ideas without adding another underlying rule:

- **L1 — Ownership:** Motion can leave a Source, become Player-owned, and move to a Target as one readable causal event.
- **L2 — Direction:** Transfer can reroute Linear Motion into one of six deterministic canonical directions, and Preview is a trustworthy promise.
- **L3 — Threat Reversal:** a Charger dash can be captured as high-magnitude Motion and rerouted into an environment outcome.

The result is not three final levels. It is a stable capability baseline from which final level design can later compose space, pacing, and presentation without reopening Motion ownership, transaction, direction, or Charger rules.

Explicit non-goals:

- final `L_01/L_02/L_03` maps, level dressing, lighting, narrative, or tutorial flow;
- new verbs, a seventh free direction, Redirect Rail as a required L2 mechanic, or a general converter framework;
- HP, damage, ordinary attacks, weapons, a second enemy, BT/EQS, GAS, or a combat framework;
- large VFX, animation, UI, or Niagara production before the interaction language is accepted.

## Source-of-Truth Boundary

Implementation decisions follow:

1. [`Docs/DESIGN_CONTRACT.md`](../DESIGN_CONTRACT.md)
2. [`Docs/ARCHITECTURE.md`](../ARCHITECTURE.md)
3. current repository code and assets
4. [`Docs/STATE.md`](../STATE.md)

Notion supplies design intent, learning objectives, failure hypotheses, and later Level Craft possibilities:

- [Goal 09 — Gate A Closure](https://app.notion.com/p/3cf6dbf617ac811e9ed1e798f1931545)
- [L1 — Capture & Transfer](https://app.notion.com/p/3c96dbf617ac81469a98e66f17db7aaa)
- [L2 — Direction Matters](https://app.notion.com/p/3c96dbf617ac81cbac83ea527149534e)
- [L3 — Enemy as Source](https://app.notion.com/p/3c96dbf617ac81fabd14cb25b1033f62)

Notion exploration does not automatically authorize implementation. This blueprint promotes only the micro-cell proofs already consistent with the repository contract.

## Current Constraints

- L1's direct Capture/Transfer loop and Reset have prior PIE evidence; Gate A must regress them under the v0.3 direction semantics rather than rebuild L1.
- Target selection and direction resolution are separate. Aim chooses an Actor; gameplay camera pose resolves one canonical output direction.
- `FMotionDirectionResolution` must be shared by Preview and Commit. A presentation that independently recomputes direction is not acceptable evidence.
- a direction mismatch must remain inspectably different from invalid target, occlusion, no Motion, or missed input.
- a rejected transaction must preserve the current owner.
- Charger Capture is legal only during the committed Dash window; Telegraph and Recovery are rejection states.
- Charger movement must use a sweep-capable root collision shape or an equivalent movement solution. No-Sweep movement cannot prove L3.
- Room Reset must restore authoritative state and transforms and clear transient selection/Preview state.
- binary map changes remain narrow, reviewable transactions. `L_TestChamber` is the validation environment; it must not silently become a final level.

## Design Derivation

### 1. One map, three isolated questions

Three separate production maps would suggest that Level Craft has started before Gate B. One undifferentiated test room, however, makes failures hard to attribute.

The smallest useful structure is therefore one `L_TestChamber` containing three spatially separated micro cells. Each cell asks one primary question and reuses the same stable runtime capabilities:

```text
L1: Who owns Motion, and can I read the transfer?
  ↓
L2: Which canonical direction will the same Transfer commit?
  ↓
L3: Can a timed threat become that same transferable resource?
```

This preserves a single reset/debug environment while keeping the learning and failure signals separable.

### 2. L1 remains the causal baseline

L1 does not test direction reasoning. Its composition should make the useful camera direction natural so the player can focus on ownership:

```text
Moving Source -> Capture -> Player Loaded -> Preview -> Receiver -> Reset
```

Presentation responsibilities remain distinct:

- Source movement/trail answers how the observed Source is moving.
- Player Loaded feedback answers who owns Motion.
- Projected direction answers what the selected Target will receive.
- target compatibility answers whether the current request is legal.

A large arrow attached to the Player's head collapses ownership and predicted output into one debug symbol. The low-cost Gate A candidate is a ground/world-space projected indicator driven by the same Preview result as Commit. It is still prototype presentation, but it tests the correct information boundary.

### 3. L2 uses a paired Receiver comparison

One Receiver can prove a success path but cannot show whether a failure came from direction compatibility or target selection. A Forward/Up pair produces the minimum controlled comparison:

| Camera resolution | Forward Receiver | Up Receiver | Required state result |
| --- | --- | --- | --- |
| Forward | Accept | `IncompatibleDirection` | successful commit or preserved Player ownership |
| Up | `IncompatibleDirection` | Accept | successful commit or preserved Player ownership |

The pair also exposes the Forward/Up pitch boundary for hysteresis observation without introducing a Redirect Rail, Carrier chain, or final spatial puzzle.

L2 succeeds only if the player can distinguish two questions:

1. Which Actor is selected?
2. Which canonical direction will that Actor receive?

If the feedback answers only one of them, the micro cell has found a presentation gap even when the resolver code is correct.

### 4. L3 separates collision truth from timing truth

A Charger that passes through blocking geometry cannot serve as a reliable threat or resource. L3 therefore has two ordered proofs:

1. **Movement/collision truth:** a swept Charger reaches a blocking wall, stops predictably, and does not tunnel, stick in repeated hits, or remain an active threat after its blocking response.
2. **Timing/resource truth:** Telegraph rejects Capture; the Dash window accepts it; success stops the Charger and grants the same high-magnitude Linear state to Player.

Only after both proofs should the Player reroute the captured state into one simple Environment Receiver/Outcome:

```text
Telegraph -> Dash -> Capture -> Charger Recovery
                           -> Player High Motion
                           -> Canonical Reroute
                           -> Environment Outcome
```

The wall is a diagnostic fixture, not a damage system. The Environment Receiver is an outcome fixture, not a final combat objective.

### 5. Reset is part of every proof

Reset is not a separate utility test at the end. Each cell must be replayable because the prototype depends on scarce, stateful resources:

- L1 restores Source ownership and clears Player/Receiver state.
- L2 restores both Receivers and clears direction Preview/stickiness.
- L3 restores Charger transform/FSM/Motion, Player ownership, and the Environment outcome.

A cell that works once but cannot restore its authored starting state is a Gate A failure.

## Capability and Fixture Boundaries

| Area | Stable capability under test | Micro-cell fixture | Must remain outside the fixture |
| --- | --- | --- | --- |
| L1 | ownership, atomic Capture/Transfer, Preview/Commit, Reset | one Source, Player, one Forward Receiver | direction puzzle, enemy, time pressure |
| L2 | six-direction resolver, hysteresis, Required Direction, mismatch preservation | shared Source/Player, Forward Receiver, Up Receiver | Redirect Rail, Carrier chain, final puzzle geometry |
| L3 | swept Charger movement, timed Capture, high magnitude, reroute, Reset | one Charger, one wall, one Environment Receiver | damage, HP, weapons, second enemy, AI framework |

Intentionally unchanged: Motion ownership model, transaction API, target-scoring contract, canonical direction set, magnitude policy, template maps, rendering/project settings, and final-level content.

## Data and Evidence Chain

```text
Repository contract
    |
    v
Code and instance properties
    |
    +--> Build / focused automation: deterministic and structural claims
    |
    +--> Blueprint validation / Map Check: asset integrity claims
    |
    +--> PIE state observation: runtime behavior claims
    |
    +--> Human play: readability, timing, prediction, and comprehension claims
```

Evidence cannot be promoted upward by assumption:

- a build does not prove a Transfer feels predictable;
- a resolver unit test does not prove the Preview is readable;
- an Actor state-machine test does not prove swept collision in the map;
- successful PIE does not prove a first player understands threat reversal.

## Risks

| Risk | Detection | Mitigation | Gate impact |
| --- | --- | --- | --- |
| Preview and Commit use different direction data | compare displayed direction and committed state across repeated poses | pass one `FMotionDirectionResolution` through both paths | P0 blocker |
| mismatch reads as invalid target or missed input | first-error observation in L2 | preserve structured rejection and give compatibility distinct presentation | P0 readability blocker |
| camera boundary flickers | slow sweep and jitter around Forward/Up threshold | retain canonical hysteresis until clearly crossing the boundary | P0 blocker if frequent |
| Charger tunnels or repeatedly hits a wall | full-speed wall test over repeated cycles | sweep-capable root collision and one predictable blocking response | P0 blocker |
| Charger Capture feels like an attack rather than theft | observe Source stop, Player Loaded, and explanation after first success | align Stop/Recovery and ownership feedback in one beat | product gate |
| high magnitude is technically present but unreadable | compare L1 and L3 Loaded/Outcome behavior | magnitude-sensitive presentation without a new rule | P1 unless outcome is ambiguous |
| Reset leaves stale ownership, FSM, Preview, or outcome | repeated resets from different phases | authoritative snapshots plus transient cleanup | P0 blocker |
| test chamber starts behaving like final level production | review asset scope and presentation changes | keep fixtures diagnostic and defer space/polish to Gate B | scope blocker |

## Implementation Stages

1. **L1 regression and presentation boundary**
   - Preserve the existing Capture/Transfer loop.
   - Separate Loaded ownership from projected output direction.
   - Exit: normal, rejection, and Reset paths remain playable under v0.3 semantics.

2. **L2 compatibility comparison**
   - Add one Up Receiver beside the existing Forward Receiver.
   - Author distinct stable participant identity and Required Direction.
   - Exit: Forward/Up accept/reject matrix, ownership preservation, hysteresis, and Preview=Commit are observable.

3. **L3 collision fixture**
   - Add one Charger and one blocking wall in an isolated lane.
   - Exit: repeated Dash cycles stop predictably on collision without tunneling or repeated-hit lock.

4. **L3 threat-reversal closure**
   - Add one Environment Receiver/Outcome.
   - Exit: Dash Capture stops the Charger, grants high Motion, reroutes through the canonical resolver, and produces the expected outcome.

5. **Gate A evidence pass**
   - rebuild the Editor module;
   - run all focused Motion automation;
   - validate Blueprints and Map Check;
   - run PIE regressions and repeated Reset;
   - record human observations separately from automated results.

Each binary asset stage is a separate review boundary. A failed binary transaction must be quarantined or corrected without rolling back unrelated source, configuration, or documentation work.

## Verification

### Structural and automated evidence

- the macOS Editor target builds with the project warning policy;
- all registered `Transmit.MotionTransfer` tests are discovered and report exact pass/fail counts;
- the Forward and Up Receiver instances expose distinct `ParticipantId` and Required Direction values;
- Charger root collision supports swept movement and Body presentation does not create a competing collision shape;
- invalid Transfer preserves the owner and reports `IncompatibleDirection` where appropriate;
- Blueprints compile and `L_TestChamber` Map Check has no blocker-level error.

### PIE behavioral evidence

- L1 Capture, Loaded, Preview, Transfer, rejection, and Reset work after the v0.3 changes;
- L2 executes both success paths and both mismatch paths without swallowing Motion;
- repeated identical camera poses produce identical canonical directions and outcomes;
- small camera jitter near Forward/Up does not rapidly switch Preview;
- Charger cycles through Telegraph, Dash, and Recovery;
- wall collision, legal Dash Capture, Stop/Recovery, high Motion ownership, reroute, Environment outcome, and Reset all execute in the placed cell.

### Human acceptance questions

Human acceptance focuses on claims that static or automated checks cannot make.

#### L1 — causal ownership

- Can the Player tell that Motion left the Source and now belongs to them?
- Is Loaded state readable without treating a head-mounted arrow as final presentation?
- Can the Player predict the Target's output before pressing Transfer?
- Does the result read as moving Motion rather than grabbing or shooting an object?

#### L2 — trustworthy direction

- Can the Player distinguish selected Target from projected output direction?
- After the first mismatch, can they identify direction incompatibility rather than assuming the input failed?
- Does the Preview remain stable enough to make a deliberate choice near the Forward/Up boundary?
- Does repeating the same camera pose build trust that Preview is a promise?

#### L3 — threat reversal

- Are Telegraph, Dash, legal Capture window, and Recovery distinguishable without a health bar?
- Does successful Capture read as stealing the dash rather than damaging the Charger?
- Do Charger Stop and Player Loaded occur as one understandable causal beat?
- Can the Player recognize and deliberately reroute the high-magnitude resource into the Environment outcome?
- After failure, do they start watching the dash window rather than searching for an attack button?

#### Recovery

- Can every cell be replayed after Reset without stale Preview, ownership, Charger state, or environment outcome?
- Does repeated Reset preserve confidence that experimentation cannot permanently break the room?

## Definition of Done

- L1 remains a readable and repeatable ownership baseline under v0.3 semantics.
- L2 proves deterministic six-direction reroute, Required Direction, distinct mismatch feedback, hysteresis, and Preview=Commit.
- L3 proves swept collision, timed Dash Capture, Stop/Recovery, high-magnitude ownership, reroute, Environment outcome, and Reset.
- build, focused automation, Blueprint validation, and Map Check have fresh evidence with claims limited to what each check proves.
- PIE behavior has been observed in the placed micro cells.
- human acceptance finds no P0 blocker in causal readability, direction trust, timing comprehension, threat reversal, or recovery.
- no new core mechanic or final-level production was introduced to obtain the result.

Passing this definition permits Core Freeze and movement into player inspection and Level Brief design. It does not itself prove final level quality, blind-player success rates, packaging, or release readiness.

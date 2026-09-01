# ADR-003: Camera-Driven Linear Reroute (v0.3 Direction Semantics)

- Status: Accepted (implementation in progress)
- Date: 2026-09-01
- Owners: Project design and engineering

## Context

EXP-001 proved the smallest ownership loop: `Source -> Player -> Receiver` with Capture / Carry / Transfer / Consume and 20/20 Room Reset. The pre-v0.3 rule was that the Player's aim selects a target but never supplies output direction; direction could change only through a deterministic environment converter such as a Redirect Rail.

The L1–L3 pipeline (08｜L1–L3 三阶段开发计划) promotes a v0.3 rule: at Transfer, the carried Linear direction is rerouted by the **gameplay camera** and deterministically quantized to one of six canonical directions — Forward / Back / Left / Right / Up / Down. This gives L2 a spatial puzzle without adding a second interaction grammar.

## Decision

Promote camera-driven canonical reroute as the P0 Transfer direction rule:

1. **Capture** preserves the Source's Linear direction and magnitude unchanged.
2. **Transfer** resolves the carried direction against the gameplay camera into exactly one canonical direction, and the Target receives the resolved world direction.
3. **One resolver drives Preview and Commit.** The same `FMotionCanonicalDirectionResolver` result travels in `FMotionTransferContext.DirectionResolution`; Preview and Transfer use identical direction and compatibility rules.
4. **Receivers declare a `RequiredCanonicalDirection`.** A mismatch returns `IncompatibleDirection` (DirectionMismatch) and never consumes Player Motion.
5. **Determinism and hysteresis.** Identical camera pose + identical carried direction must produce an identical result. Up / Down use an explicit pitch threshold; adjacent sectors use boundary hysteresis so selection does not flicker.

## Supersedes

- Pre-v0.3 rule "aim never supplies output direction; direction changes only through an environment converter" for the Transfer verb.
- Redirect Rail as the P0 L2 mechanism. Rail geometry may return as a deterministic environment converter, but it is no longer required for L2.

## Rationale

- The camera is already the player's shared selection and readability instrument; quantization to six directions keeps output legible and predictable.
- A single resolver result for Preview and Commit removes the largest correctness risk in the pipeline (preview showing one direction, commit applying another).
- Camera-relative canonical directions make L2 a spatial problem (where do I stand / where does the camera point) instead of a free-aim shooting mechanic.
- World-space Up / Down map directly to level outcomes such as lifts, keeping the gameplay grammar unchanged.

## Consequences

- `UMotionTransferComponent` direction rule changes from a world-space dot check to a canonical-direction requirement; Receiver authoring uses `RequiredCanonicalDirection`.
- The transferred state's `Direction` becomes the resolved world direction, so downstream presentation and conversion see the same signature the Player previewed.
- `UMotionInteractorComponent` must compute and carry the resolution for both preview and commit; commit revalidates the target but reuses the same resolution snapshot.
- Direction mismatch must be distinguishable from `InvalidTarget / Occluded / SourceEmpty` and must preserve ownership.
- Resolver thresholds (pitch enter/exit, sector hysteresis) are centralized tuning values, not Blueprint-local constants.

## Rejected Alternatives

- **Keep pre-v0.3 aim-never-supplies-direction:** no spatial hook for L2; the level would depend on environment-only conversion with higher geometry and readability cost.
- **Redirect Rail as the only converter:** authored geometry is expensive to iterate and harder to read than the camera-relative canonical mapping during the P0 window.
- **Free aim vector becomes output direction:** recreates an energy-shooting mechanic and violates the Motion State grammar.

## Acceptance Gate

- Deterministic resolver tests: identical input + camera pose always resolves identically; hysteresis cases hold at both pitch and sector boundaries.
- Preview = Commit tests: the resolution used for preview compatibility equals the resolution used for the committed Transfer.
- Direction mismatch preserves Player Motion and returns a distinguishable rejection.
- L2 micro cell (six-direction resolver + Required Canonical Direction + mismatch feedback) and L1 micro cell regress independently of any final level.

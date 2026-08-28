# Agent Handoff

> Rebuildable and non-authoritative. Durable product rules belong in `DESIGN_CONTRACT.md`; durable technical decisions belong in `ARCHITECTURE.md`, ADRs, and Git history.

Last inspected: 2026-08-28
Plan status: **Awaiting review — do not begin implementation until this plan is approved.**

## Current Milestone

Deadline vertical slice — ship a readable, complete L1–L3 Windows build by the internal submission deadline of **2026-09-06**.

- Focused development window: 2026-08-29 through 2026-09-06.
- Capacity: approximately 54 focused hours at 6 hours/day.
- 2026-09-06 is the internal submission deadline.
- 2026-09-07 is reserved for upload and P0 emergency fixes only; it is not a development day.

## Authoritative Baseline

- Windows Git is authoritative for repository state.
- Confirmed baseline supplied on 2026-08-28: `HEAD fcf0d8d`, clean working tree, `main` one commit ahead of `origin/main`.
- The earlier “253 modified files” risk was disproven. No work remains to audit hundreds of binary changes.
- UE third-person template assets and project configuration are present.
- The latest local UE 5.8.1 Editor log records successful initialization and a map check with 0 errors and 0 warnings.
- A `CompileAllBlueprints` commandlet run completed with 0 Blueprint errors, 0 Blueprint warnings, and 0 load failures, although the process exited 1 because the workstation's Installed DDC/Zen cache had no writable node.

## Frozen Scope

Required:

- EXP-001 becomes the production foundation of L1; it is not throwaway prototype work.
- L1: one Linear Source → Player Capture/Carry → compatible Receiver, readable feedback, and reliable room Reset.
- L2: one deterministic Linear redirect path that proves direction belongs to Motion State.
- L3: one Charger threat-reversal path using the same Capture/Carry/Transfer grammar.
- L1–L3 run from start to finish, including Reset / Fail / Restart and packaging.
- Preserve the current C++ core / Blueprint content boundary.

Explicitly out of scope:

- L4 and L5.
- Angular and Oscillation.
- A general combat framework.
- GAS or Behavior Tree unless a hard technical requirement emerges and the plan is explicitly revised.
- Opportunistic architecture, tooling, visual-polish, or content expansion that does not protect a hard gate.

## Hard Gates and 54-Hour Allocation

| Date | Hours | Required outcome | Gate / stop rule |
| --- | ---: | --- | --- |
| Aug 29 | 6 | Confirm the narrow L1 path in current architecture; begin reusable EXP-001/L1 C++ capability and Blueprint content in `L_TestChamber`. | No parallel framework work and no throwaway implementation. |
| Aug 30 | 6 | Complete the first playable Source → Player → Receiver loop with explicit ownership feedback and room Reset. | Protect end-to-end playability before polish. |
| Aug 31 | 6 | Harden EXP-001 as L1 and run 20 consecutive reset cycles. | **Gate:** playable loop and 20/20 clean resets. Do not start L2/L3 if this fails. |
| Sep 1 | 6 | Run the first-player blind test, record comprehension evidence, and fix only core readability/friction. | **Gate:** unlock L2/L3 only if the player completes without verbal guidance and understands that Motion State moved and where it resides. Otherwise continue L1 and re-test. |
| Sep 2 | 6 | Build and validate L2's single deterministic Linear redirect route using the L1 foundation. | No new verb or second gameplay grammar. |
| Sep 3 | 6 | Build and validate L3's single Charger threat-reversal route using the same state ownership and transfer path. | No combat framework; direct Player → Charger Transfer remains unauthorized unless separately promoted. |
| Sep 4 | 6 | Integrate and run L1–L3 from start to finish; repair only progression, reset, readability, and P0 blockers. | **Gate:** L1–L3 fully playable; feature freeze at end of day. |
| Sep 5 | 6 | Produce RC1 packaged Windows build and execute clean-machine-path smoke testing plus L1–L3 regression. | **Gate:** RC1 packaged build exists; post-freeze changes are bug fixes only. |
| Sep 6 | 6 | Complete final QA, PDF, H.264 video, source archive, hashes/file checks, and submission rehearsal. | **Gate:** all final artifacts complete before end of day. |
| Sep 7 | 0 planned | Upload verified artifacts; apply only P0 emergency fixes if upload or launch is blocked. | No features, scope recovery, or discretionary polish. |

## Gate Evidence and Definition of Done

- Aug 31: a playable `L_TestChamber` run plus a recorded 20/20 reset-cycle result with no duplicated/lost Motion State or stale Player carry state.
- Sep 1: observation notes from one first-time player; the pass/fail decision is explicit before L2/L3 work begins.
- Sep 4: one uninterrupted L1–L3 PIE run demonstrates start, completion, Reset / Fail / Restart, and the intended 5–7 minute flow.
- Sep 5: RC1 packaged build launches and completes the critical path outside the Editor on the submission machine; compilation alone is insufficient.
- Sep 6: final PDF opens correctly, video is H.264 and plays correctly, source archive extracts cleanly, packaged build launches, and the upload set matches the final checklist.

Codex can implement and statically/build-validate the scoped C++ and Blueprint-supporting changes, inspect diffs, and prepare reproducible test steps. Human Editor/playtest judgment is required for blind-test comprehension, gameplay readability, full PIE acceptance, capture of the submission video, and final upload confirmation.

## Current Blockers and Risks

- No Motion State, Capture, Carry, Transfer, Receiver, or room Reset runtime path has yet been verified in PIE.
- The highest-risk assumption is comprehension: players may read the loop as object grabbing or energy shooting rather than moving motion. Sep 1 is the decision gate, not a ceremonial test.
- The C++ component/interface boundary and atomic ownership transaction are not yet proven in UE runtime.
- The Sep 1 comprehension gate leaves only three focused days before feature freeze; a failure requires reducing presentation and content ambition, not bypassing the gate.
- L3 still depends on an approved environment outcome because direct Player → Charger Transfer remains open in the Design Contract.
- Local DDC/Zen permissions or data-path configuration must be repaired or safely worked around before automated commandlets can return a clean process exit.
- The `.uproject` uses a GUID engine association; another workstation may require reassociation. Cross-platform compatibility is not part of the deadline acceptance unless separately required.

## Immediate Next Step

Review and approve or revise this plan. After approval, begin only the Aug 29 EXP-001-as-L1 work. Do not implement gameplay before that review.

## Update Policy

At the end of a task, replace this snapshot with only the current milestone, working/broken paths, immediate next step, and live risks. Never use this file as the sole record of a requirement or architectural decision.

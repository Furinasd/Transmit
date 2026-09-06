# Charger Editor Authoring Crash Postmortem

Date: 2026-09-05
Status: diagnosis captured; causal A/B fix still pending
Tracking issue: https://github.com/Furinasd/Transmit/issues/7
Observed baseline: `4db656bbacc15bbabfedebffc9485e22be26853f`
Likely introduction point: `0edbd84637613194d40e18a19e4470ea41f1ee30`

## Why this record exists

This incident exposed a validation blind spot rather than only a local crash.

At the time of the crash, the current Gate 4 machine-side evidence was green:

- fresh `passelyEditor` build;
- `Transmit.MotionTransfer` 16/16 automation;
- touched Blueprint compile with 0 errors;
- `L_TestChamber` Map Check with 0 errors / 0 warnings.

Yet opening / authoring the Charger in Unreal Editor still crashed reproducibly in the user's workflow.

The durable lesson is that runtime correctness, static data correctness, and Editor authoring correctness are separate verification surfaces.

## Evidence boundary

### Observed facts

From Issue #7 and the archived crash evidence:

- UE 5.8.2 MacEditor reported `SIGBUS: invalid attempt to access memory`.
- Top frames include `FMemory::Realloc`, delegate allocation, and `FDetailPropertyRow` creation.
- The repeating stack pattern is dominated by PropertyEditor child/layout generation:
  `GenerateChildrenForPropertyNode -> GenerateLayout -> OnGenerateChildren -> ...`.
- `ATransmitChargerActor::StateMachine` is exposed as an `Instanced, EditAnywhere` property in category `Motion|Charger`.
- `UMotionChargerStateMachine` is `DefaultToInstanced, EditInlineNew`.
- Its six editable child properties also use category `Motion|Charger`.
- The Charger constructor creates the StateMachine default subobject before gameplay starts.

These facts localize the failing boundary to Editor Details generation for an inline-instanced UObject. The available evidence does not implicate Charger Dash/Tick logic, Motion ownership transactions, or the direction-policy seam.

### High-confidence causal hypothesis

The inline StateMachine children reuse the same hierarchical category name as the outer actor property. PropertyEditor category lookup can therefore resolve the child rows back into the outer `Motion|Charger` subcategory, causing layout re-entry and recursive Details-tree generation until the process fails at a terminal allocation / stack boundary.

This is still a hypothesis until the metadata-only A/B test is executed. Do not record the root cause as closed before that test passes.

## Smallest discriminating test

Change only the six editable StateMachine child-field categories from `Motion|Charger` to a distinct flat category such as `Charger State`.

Then:

1. run full UHT / C++ rebuild;
2. fully restart Unreal Editor;
3. repeat the original Charger opening / Details expansion workflow multiple times;
4. verify the StateMachine fields can be edited, saved, and reopened;
5. rerun `Transmit.MotionTransfer` regression automation;
6. rerun relevant Blueprint compile and Map Check;
7. rerun Charger PIE / Reset behavior.

If the PropertyEditor recursion persists, stop and inspect actual property/category nodes before expanding the fix.

Do not use this incident as justification to redesign the Charger FSM, change Motion ownership, remove inline instancing, patch engine code, or refactor unrelated gameplay architecture without new evidence.

## Technical lessons

### 1. Reflection metadata is executable Editor behavior

`UPROPERTY` metadata is not merely presentation text. `Category`, `Instanced`, `EditInlineNew`, `DefaultToInstanced`, Blueprint exposure, and component ownership participate in real Editor-side behavior, serialization, CDO/archetype behavior, and designer authoring paths.

Treat reflection-facing metadata changes as functional changes, not formatting changes.

### 2. Runtime green does not prove authoring safety

The existing validation chain covered build, automation, Blueprint compile, Map Check, and runtime behavior. None of those exercises the same code path as generating and editing a Details tree for an inline UObject.

A Designer-facing API is not validated until the real authoring surface has been exercised.

### 3. Test the contract where it is consumed

Use the verification surface that matches the claim:

- runtime contract -> automation / PIE;
- designer-facing contract -> Editor authoring workflow;
- player-facing contract -> human playtest;
- release contract -> clean package / fresh launch.

Evidence from one surface must not masquerade as evidence for another.

### 4. Read crash stacks for repeating structure

`FMemory::Realloc` identifies where the process died, not necessarily why it died.

The repeated PropertyEditor generation cycle is more diagnostic than the terminal allocator frame.

Reusable triage rule:

> Terminal frame tells where it died; repeating structure tells how it died.

### 5. Fix the smallest causal edge

Preserve the experiment. Change one causal variable, rebuild, reproduce, and only then widen the fix.

A crash is not permission for opportunistic architecture cleanup.

## Process upgrade: Editor Authoring Smoke Gate

Any task that modifies one or more of the following must include Editor-authoring validation in addition to source/runtime checks:

- `UCLASS` / `USTRUCT` / `UPROPERTY` exposure;
- `Instanced` / `EditInlineNew` / `DefaultToInstanced` semantics;
- Details `Category` layout;
- Blueprint-visible property or function exposure;
- component ownership / hierarchy that changes designer-facing Details state.

Minimum smoke path:

```text
Place or load actor
-> select / open
-> inspect Details
-> expand exposed structs or inline objects
-> modify one representative value
-> save
-> reopen
```

For inherited Blueprint authoring, also verify:

```text
Open Blueprint
-> inspect inherited properties
-> compile
-> change default
-> save / reopen
```

If an agent cannot execute or observe the relevant Editor surface, it must leave that validation as an explicit human gate rather than self-certifying completion from source tests.

## TD / interview value

The high-value story is not "a Category string crashed the Editor."

It is:

1. a fully green runtime/static matrix still missed a deterministic Designer-authoring crash;
2. crash-stack analysis localized the failure to a different system boundary;
3. the diagnosis preserved epistemic boundaries between observed fact and causal hypothesis;
4. the fix strategy kept the A/B experiment minimal;
5. the incident upgraded the project validation model with an explicit Editor Authoring Gate.

That demonstrates Technical Design judgment across gameplay code, Unreal reflection, editor usability, validation design, and AI-assisted production discipline.

# Transmit Project Working Agreement

This file defines how Codex should work inside this repository.
It does not define gameplay design itself.

## Gameplay Implementation Gate

When implementing gameplay:

1. Read Docs/IMPLEMENTATION_CONTRACT.md
2. Read relevant architecture/code
3. Inspect current implementation
4. Only then implement

Notion contains design exploration and is not
automatically implementation-authoritative.

## Source of Truth

Use the following information hierarchy for gameplay implementation:

1. `Docs/IMPLEMENTATION_CONTRACT.md`
   - implementation-approved gameplay rules
   - authoritative for what the current prototype is intended to do

2. `Docs/ARCHITECTURE.md`
   - authoritative description of the current implemented architecture

3. Current repository code/assets
   - authoritative for what actually exists

4. `Docs/STATE.md`
   - current milestone, blockers, working/broken status
   - operational and disposable

Notion contains broader design exploration, hypotheses, rejected ideas,
worldbuilding, playtest evidence, and future possibilities.

Do not treat brainstorm or future ideas from Notion as implementation-approved
unless they have been promoted into `IMPLEMENTATION_CONTRACT.md`.

If design documentation and implementation disagree, report the discrepancy.
Do not silently choose one and rewrite the other.

---

## Project Priorities

Optimize for:

1. Playable core interaction
2. Gameplay readability
3. Fast iteration
4. Robustness
5. Extensibility
6. Visual polish

Do not optimize for architectural sophistication for its own sake.

Prefer the smallest architecture that cleanly supports the currently approved
gameplay and obvious near-term extensions.

---

## UE Development Boundary

Prefer:

- C++ for stable gameplay capability, state ownership, reusable contracts,
  validation, and low-level logic.
- Blueprint for gameplay behaviour, encounter scripting, presentation,
  rapidly changing parameters, and content authoring.
- Unreal Editor as the playable validation environment.
- `L_TestChamber` as the first validation environment for new gameplay
  capability before integration into production levels.

Do not introduce GAS, complex subsystems, large frameworks, or new plugins
unless explicitly approved.

Do not create abstractions only to support hypothetical future systems.

---

## Learning-Oriented Pair Programming

The human developer is actively learning Unreal Engine and Rider.

Do not treat implementation as a black-box code-generation task.

For meaningful code changes, finish with exactly these sections:

### What changed
Briefly explain the implemented behaviour and the important files/classes.

### Why here
Explain why the responsibility belongs in this Component/Class/Blueprint layer,
including the key architecture decision.

### What to inspect in Rider
Name the 1-3 most useful files/symbols to inspect manually and explain what
concept to pay attention to.

Keep this educational explanation concise and directly related to the current
change.

Do not turn every task into a general Unreal Engine tutorial.

---

## Planning Rules

For implementation plans:

- inspect the current repository before proposing architecture
- preserve working architecture unless there is a concrete reason to change it
- distinguish required work from optional/stretch work
- prefer vertical gameplay slices over large horizontal infrastructure phases
- every meaningful gameplay milestone must end in a playable Unreal validation
- state explicitly what Codex can implement and what requires human Editor/playtest judgment
- include a clear Definition of Done
- identify the highest-risk assumption early and validate it first

Plans should minimize simultaneous new concepts for the human developer.

Do not introduce several unfamiliar Unreal systems in one milestone unless
strictly necessary.

---

## Validation

Compilation success is not equivalent to gameplay success.

After gameplay implementation:

1. run appropriate build/static validation
2. identify the exact Unreal Editor manual validation required
3. do not claim gameplay behaviour is verified until it has actually been tested

For gameplay work, prefer:
Code/Blueprint change
→ Build
→ `L_TestChamber`
→ PIE
→ Human observation
→ Iterate

Never claim macOS compatibility based solely on Windows compilation.

---

## Agent Safety

Before broad Unreal Editor or MCP operations:

- require a clean or understood Git working tree
- prefer development/test assets
- avoid bulk changes to production `.uasset` / `.umap` assets
- report changed binary assets clearly

Do not redesign gameplay without explicit instruction.

Do not update Notion design decisions automatically.
You may identify documentation drift and recommend an update.

---

## Definition of Done

A task is not done merely because code was written.

It is done when:

- implementation matches the approved contract
- relevant build checks pass
- required manual Unreal validation is stated
- architecture/docs are updated only if the implementation actually changed them
- the final response explains What changed / Why here / What to inspect in Rider

When the requested task is complete, stop.
Do not continue opportunistic refactoring.

# Transmit Project Working Agreement

This file defines how Codex operates inside the Transmit repository.

It defines implementation authority, automation boundaries, validation rules, and repository safety.

It does **not** define gameplay design itself.

---

## 1. Operating Principle

Optimize for producing the smallest reliable playable result that satisfies the currently approved design.

Priority order:

1. Playable core interaction
2. Gameplay readability
3. Fast iteration
4. Robustness
5. Reusable authoring capability
6. Extensibility
7. Visual polish

Do not optimize for architectural sophistication, automation coverage, procedural complexity, or tool-building for their own sake.

Prefer a vertical playable slice over broad infrastructure.

Use the principle:

> Second use before abstraction.

Do not create abstractions solely for hypothetical future systems.

---

## 2. Gameplay Implementation Authority

Before implementing or modifying gameplay:

1. Read `Docs/DESIGN_CONTRACT.md`
2. Read relevant sections of `Docs/ARCHITECTURE.md`
3. Inspect the current implementation
4. Inspect `Docs/STATE.md` when milestone context matters
5. Only then plan or implement

Notion contains design exploration and is not automatically implementation-authoritative.

### Source of Truth

For gameplay implementation, use this hierarchy:

**1. `Docs/DESIGN_CONTRACT.md`**

Implementation-approved gameplay rules.

Authoritative for what the current prototype is intended to do.

**2. `Docs/ARCHITECTURE.md`**

Authoritative description of the currently accepted implementation architecture.

**3. Repository code and Unreal assets**

Authoritative for what actually exists.

**4. `Docs/STATE.md`**

Current milestone, blockers, known working/broken state, and immediate handoff.

Operational and disposable.

Notion may contain hypotheses, rejected ideas, future mechanics, worldbuilding, references, playtest evidence, and design exploration.

Do not implement a Notion idea unless it has been explicitly promoted into the repository contract or the human explicitly scopes it for implementation.

If design documentation and implementation disagree:

* identify the discrepancy;
* report which sources disagree;
* do not silently rewrite either side;
* do not guess which behaviour is intended.

---

## 3. Unreal Development Boundary

Prefer:

* **C++** for stable gameplay capability, state ownership, reusable contracts, validation, deterministic rules, and low-level systems.
* **Blueprint** for gameplay behaviour, encounter scripting, presentation, content authoring, rapidly changing parameters, and designer-facing configuration.
* **Data Assets / configuration assets** for reusable tunable data where repetition justifies them.
* **Unreal Editor** as the authoritative playable validation environment.
* **`L_TestChamber`** as the first runtime validation environment for new gameplay capabilities.

A new gameplay capability should normally progress through:

`Implementation → Build → L_TestChamber → PIE → Human Observation → Production Level`

Do not introduce GAS, large gameplay frameworks, complex subsystems, Behavior Trees, new plugins, or generalized data frameworks unless a concrete approved requirement justifies them.

---

## 4. Human vs Agent Authority

Codex may automate implementation and repetitive production work.

Codex does not own gameplay or level-design intent.

### Human-authoritative decisions

The human owns:

* level topology;
* gameplay metrics;
* encounter intent;
* teaching order;
* critical sightlines;
* player traversal;
* camera composition;
* hero moments;
* gameplay readability judgment;
* art direction;
* final aesthetic acceptance;
* playtest interpretation.

### Agent-authoritative execution

Within an explicitly approved scope, Codex may:

* implement C++ and text-based systems;
* create Editor automation scripts;
* assemble approved modular assets;
* configure approved Blueprint instances;
* generate constrained PCG dressing;
* create and validate environment manifests;
* perform repetitive asset operations through Unreal APIs;
* run static/build validation;
* identify likely regressions;
* report changed assets and unresolved risks.

Codex must not reinterpret a visual or gameplay goal merely because another arrangement is easier to automate.

---

## 5. Environment Authoring Boundary

Environment automation follows this rule:

> Human authors gameplay space. Automation executes visual structure inside that space.

The following are protected design data unless explicitly scoped for modification:

* gameplay actor transforms;
* player start;
* gameplay volumes;
* traversal width and height;
* encounter distances;
* Receiver / Source / Charger placement;
* camera-critical geometry;
* critical sightlines;
* puzzle topology;
* required interaction paths;
* reset/checkpoint logic.

Environment automation may alter approved visual geometry around these constraints but must not silently change them.

Prefer explicit protection mechanisms where useful:

* readability/exclusion volumes;
* protected splines;
* gameplay tags;
* environment manifests;
* approved asset palettes;
* PCG exclusion inputs.

PCG should execute authored constraints rather than invent level structure.

---

## 6. Environment Production Pipeline

Environment production begins only after the gameplay graybox for the scoped area is playable.

The graybox is the spatial contract.

Each production phase is a separate, reviewable transaction.

### Gate 0 — Gameplay Graybox

Required before visual production:

* intended gameplay loop is playable;
* gameplay actors are placed;
* traversal metrics are acceptable;
* critical sightlines are identified;
* camera behaviour is acceptable;
* reset/retry works;
* human explicitly marks the area ready for visual production.

Do not use environmental polish to compensate for unresolved gameplay.

### Phase 1 — Architectural Replacement

Goal:

Replace graybox surfaces with approved modular architecture while preserving the graybox spatial contract.

Allowed:

* floor/wall/ceiling replacement;
* architectural modules;
* stairs and structural shells;
* material assignment;
* modular snapping;
* non-gameplay collision cleanup.

Protected:

* gameplay topology;
* critical dimensions;
* gameplay actor transforms;
* intended routes;
* critical sightlines.

Acceptance Gate:

* gameplay bounds remain materially equivalent;
* all intended routes remain traversable;
* gameplay actors remain reachable and readable;
* no new camera obstruction or collision failure;
* scoped level completes successfully in PIE.

Stop after saving and report all binary assets changed.

### Phase 2 — Structural Composition

Goal:

Add secondary architectural rhythm without changing gameplay structure.

Examples:

* pillars;
* frames;
* beams;
* trims;
* recesses;
* arches;
* large silhouette elements;
* controlled structural repetition.

Structural additions must reinforce composition, navigation, scale, or focal hierarchy.

Do not add structure simply to fill empty space.

Acceptance Gate:

* entrance composition still exposes the intended focal information;
* Source / Receiver / Charger silhouettes remain readable;
* player route hierarchy remains clear;
* important negative space is preserved;
* no protected readability volume is violated;
* human visual review accepts the composition.

### Phase 3 — Dressing

Goal:

Add environmental density and local storytelling inside explicit design constraints.

Preferred tools:

* manually approved modular assets;
* PCG Graphs;
* PCG Volumes / Splines;
* Foliage where appropriate;
* controlled scatter tools.

Dressing must respect:

* exclusion/readability volumes;
* traversal clearance;
* gameplay actor clearance;
* camera sightlines;
* authored density limits;
* approved asset palette.

PCG seeds should remain deterministic during review unless random exploration is explicitly requested.

Acceptance Gate:

* zero blocking of protected traversal;
* zero obstruction of critical gameplay reads;
* asset density supports rather than competes with gameplay;
* no obvious floating/interpenetrating major assets;
* regenerated PCG output remains deterministic;
* human review approves density and visual hierarchy.

### Phase 4 — Surface Cohesion

Goal:

Make heterogeneous assets read as one authored environment.

Allowed:

* approved master material instances;
* tint/roughness/normal tuning;
* world-aligned material parameters;
* decals;
* vertex paint;
* controlled surface wear;
* grime, damage, dust, or local variation.

Do not alter gameplay readability colors without explicit approval.

Acceptance Gate:

* gameplay semantic colors remain immediately distinguishable;
* imported asset packs no longer read as unrelated visual sets;
* materials behave consistently under current lighting;
* surface noise does not overwhelm motion/readability indicators;
* no unsupported material or shader dependency is introduced.

### Phase 5 — Lighting and Final Readability

Goal:

Establish final spatial hierarchy, atmosphere, and gameplay readability.

Lighting is not allowed to redesign the encounter.

Protected:

* gameplay visibility;
* target readability;
* motion visualization;
* interaction feedback;
* player navigation.

Acceptance Gate:

* Source / Receiver / Charger remain readable in motion;
* required routes remain visually legible;
* hero composition works from intended gameplay cameras;
* no interaction cue is lost to exposure, bloom, fog, or contrast;
* scoped gameplay completes in PIE under final lighting;
* human explicitly approves the scene for polish freeze.

No phase may silently advance into the next phase.

---

## 7. Unreal Asset Operations

Unreal binary assets are not ordinary filesystem files.

For `.uasset`, `.umap`, and Unreal-managed content:

* never use `os.rename`, `shutil.move`, shell `mv`, filesystem copy/delete, or equivalent raw disk operations to rename, move, or delete project assets;
* use Unreal Editor asset operations instead;
* preferred Python APIs include `unreal.EditorAssetLibrary` and `unreal.AssetTools`;
* equivalent Unreal Editor scripting, Blueprint Editor Utility, or approved MCP operations are acceptable when they preserve Unreal references;
* use Unreal migration workflows when moving referenced content between projects.

Raw filesystem operations are acceptable for normal repository files such as:

* `.cpp`;
* `.h`;
* `.py`;
* `.md`;
* `.json`;
* `.ini` when explicitly scoped;
* other non-Unreal-managed text/source artifacts.

Never attempt to repair broken Unreal references by moving `.uasset` files directly on disk.

---

## 8. Binary Asset Transaction Safety

Binary Editor operations must be narrow and reviewable.

Before a binary write:

1. inspect Git status;
2. establish whether the working tree is clean or intentionally dirty;
3. name the exact assets or level scope to be modified;
4. state protected assets/configuration that must not change.

Do not combine unrelated categories of binary work in one transaction.

Examples of separate tasks:

* modify one Blueprint family;
* modify one scoped Level;
* generate one environment pass;
* edit Input assets;
* alter project configuration.

Do not bundle these together merely for convenience.

After meaningful binary writes:

`Save → Stop → Report changed assets → Human Inspect → Continue`

Do not immediately start another production phase.

Project Settings, Editor Preferences, rendering CVars, World Settings, global lighting settings, and plugin configuration are read-only by default unless explicitly included in the task.

A level-authoring task must not implicitly “fix” project configuration.

Never use broad cleanup or deletion to recover from an MCP or Editor automation failure.

Quarantine or revert suspect uncommitted binary assets instead.

A binary-asset failure must not trigger rollback or deletion of unrelated `Source/`, `.uproject`, `Config/`, or documentation work without evidence that those files caused the failure.

---

## 9. Agent Asset Discovery

Do not select environment assets arbitrarily from the entire Content Browser.

When automated environment assembly is requested, prefer an approved asset set or environment manifest.

A useful environment manifest may record:

* asset path;
* role/category;
* bounds;
* source pack;
* collision status;
* material slots;
* Nanite state where relevant;
* approved usage;
* PCG eligibility;
* scale constraints.

Codex may propose asset classifications.

Human approval determines the final visual palette when taste or art direction is involved.

Do not assume that an asset being technically usable makes it aesthetically approved.

---

## 10. Planning Rules

Before proposing implementation architecture:

* inspect the current repository;
* preserve working architecture unless a concrete problem justifies change;
* distinguish required work from optional/stretch work;
* identify the highest-risk assumption first;
* validate that assumption early;
* prefer vertical slices over horizontal infrastructure;
* minimize simultaneous unfamiliar Unreal systems for the human developer.

Every meaningful gameplay or level milestone must end in a playable Unreal validation.

Plans must explicitly distinguish:

**Codex-verifiable**

Examples:

* build result;
* asset existence;
* reference integrity;
* deterministic PCG regeneration;
* collision/static checks;
* changed-file inventory.

**Human-verifiable**

Examples:

* gameplay feel;
* spatial readability;
* composition;
* visual hierarchy;
* player understanding;
* aesthetic quality.

Every plan must include a clear Definition of Done.

---

## 11. Validation

Compilation success is not gameplay success.

Asset creation success is not level-design success.

After implementation or content-authoring work:

1. run appropriate build/static/editor validation;
2. report what is objectively verified;
3. identify exact Unreal Editor manual validation still required;
4. do not claim behaviour, readability, composition, or compatibility is verified until it has actually been observed.

For gameplay work, prefer:

`Code/Blueprint → Build → L_TestChamber → PIE → Human Observation → Iterate`

For production-level work, prefer:

`Approved Graybox → One Production Phase → Save → PIE → Human Inspection → Commit → Next Phase`

Never claim macOS compatibility based solely on Windows compilation.

---

## 12. Learning-Oriented Pair Programming

The human developer is actively learning Unreal Engine and Rider.

Do not treat implementation as black-box code generation.

For meaningful implementation changes, finish with exactly these sections:

### What changed

Briefly explain the implemented behaviour and important files/classes/assets.

### Why here

Explain why the responsibility belongs in this Component, Class, Blueprint, tool, or content layer and identify the key architecture decision.

### What to inspect in Rider

Name the 1–3 most useful files/symbols to inspect manually and state the Unreal/C++ concept worth noticing.

Keep this concise and directly related to the current change.

Do not turn each task into a general Unreal tutorial.

For content-only work where Rider inspection is irrelevant, state that no Rider inspection is required and identify the most useful Unreal Editor assets to inspect instead.

---

## 13. Definition of Done

A task is not done because code was written, a Blueprint compiled, or an Editor automation command completed.

A task is done only when:

* implementation matches the approved design contract;
* the requested scope has been completed without unapproved expansion;
* relevant build/static/editor checks pass;
* changed binary assets are explicitly reported;
* required manual Unreal validation is identified;
* architecture/docs are updated only when the implementation actually changed them;
* the final response follows the learning-oriented handoff format when applicable.

For an environment production phase, completion additionally requires that phase's Acceptance Gate.

When the requested task is complete, stop.

Do not continue opportunistic refactoring, cleanup, redesign, optimization, dressing, or polish outside the approved scope.

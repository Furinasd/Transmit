# Transmit

Third-person combat-puzzle prototype built around moving one **Transfer State** between the Player, Enemies, and Environment.

## Current Status

The repository is at **M0: contract and architecture bootstrap**. It currently contains the Unreal third-person Blueprint template, project configuration, and the design/architecture boundary. Transfer gameplay has not been implemented yet.

The GitHub repository is named `Transmit`; the existing Unreal project file and internal game name remain `passely` to avoid an unplanned asset/module rename.

## Open the Project

1. Install Git LFS and run `git lfs pull` after cloning.
2. Open `passely.uproject` with Unreal Engine 5.8.
3. Use `/Game/ThirdPerson/Lvl_ThirdPerson` as the current editor and game entry map.

The latest local inspection used Unreal Engine 5.8.1. Packaging and cross-platform compatibility have not been verified.

## Architecture and Project Rules

Read the project context in this order:

1. [`Docs/GOAL.md`](Docs/GOAL.md) — product outcome and non-goals
2. [`Docs/DESIGN_CONTRACT.md`](Docs/DESIGN_CONTRACT.md) — implementation-authoritative gameplay rules
3. [`Docs/ARCHITECTURE.md`](Docs/ARCHITECTURE.md) — current Blueprint skeleton and proposed Transfer architecture
4. [`Docs/Decisions/`](Docs/Decisions/) — proposed technical decisions and acceptance gates
5. [`Docs/STATE.md`](Docs/STATE.md) — temporary handoff snapshot

Do not implement a Transfer interaction until its state, direction, rejection behavior, feedback, and first level usage are promoted into the design contract.

## Repository Boundaries

- `Config/` contains project defaults and startup/runtime configuration.
- `Content/` contains Blueprint, map, input, mannequin, and prototyping assets.
- `Docs/` contains the durable product and technical boundary.
- `Source/` and project-local `Plugins/` are currently absent; there is no custom C++ module.
- Unreal/IDE caches and per-user content are ignored.
- Unreal binary assets are stored through Git LFS.

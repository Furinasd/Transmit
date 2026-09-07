# Boss rail correction

User-approved scope: player-directed 户晨风 dash, the actual Zone 2 carrier on a constant-speed rail, forward-only counterstroke, circular damage, guaranteed gate-front return. The later performance-review constraint keeps new Zone 2 designs on paper.

## Responsibility and cost

- ArenaCharger uses the existing four-state Charger FSM. Idle acquires the player's ground direction; telegraph locks it. Recovery retires uncaptured energy, interpolates home once, then leaves a punish window. A held dash is not regenerated.
- Ram remains the existing level-specific coordinator. It consumes the ordinary dock state atomically; the existing carrier becomes the consume endpoint. Its native controller pointer bypasses ordinary carrier translation only while armed. Reset restores the original endpoint and flags.
- Rail motion is O(1) triangular motion; the counterstroke uses one swept move per frame. The impact circle resolves once per stroke. No new tick Actor, gameplay subsystem, GAS, Niagara system or Chaos simulation is introduced.
- Presentation reuses the existing bounded instanced-mesh effect pools. Boss pressure/shards are drawn only on actual impacts. One temporary CameraActor exists only during the 2.4-second dock glance and is destroyed on completion, input, retry or reset.
- Chinese Canvas text uses the engine's runtime Slate font/fallback. Objective and hint wrapping is cached until text/viewport changes. Slate/SlateCore are engine dependencies, not new plugins.
- A tagged map-local sun changes smoothly with progression; updates stop within a small convergence threshold. Initial values are restored when presentation ends. No project renderer settings were changed.

Do not interpret scripted PIE frame times as a GPU benchmark: background Editor throttling and test-driver activity affect them. Existing instanced-stroke drawing remains a separate pre-existing cost; the technical owner should compare fixed camera captures in a packaged build for a performance budget decision.

## Zone 2 paper proposals — not implemented

1. **截停对接**: leave an intermediate landing beside the carrier lane. The player judges alignment, presses E while the bridge moves, crosses the stopped bridge, then recovers/transfers that same resource to the next bridge. Telegraph alignment with a fixed line; keep the current end stop as a forgiving fallback. Validate that a first player understands stopping changes position, not just resource ownership, before replacing the current route.
2. **身体制动**: a short optional side loop lets the player step onto a moving bridge to brake it near a landing, step off, recover its Motion with E, and send it onward with Q. No timer, switch or new resource. Keep it optional until standing collision behavior and camera readability are consistently understood.

Do not add either puzzle to production during this polish pass. A paper route sketch and first-play evidence should justify any later layout edit.

## Reference

World/dialogue: [提交版04｜任务、世界与演出](https://app.notion.com/p/a1d6dbf617ac83f091088183c88004ba).
Lighting direction uses the existing dynamic scene, consistent with Epic's [Lumen dynamic lighting documentation](https://dev.epicgames.com/documentation/unreal-engine/lumen-global-illumination-and-reflections-in-unreal-engine).

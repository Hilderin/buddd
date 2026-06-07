# Implementation Contract Review — Engine Ownership Refactor

## Summary

**Verdict: ACCEPTED** — Re-review (2026-06-06): All 4 previous issues are RESOLVED. The contract now correctly:
1. Keeps `#include <chrono>` in CubeSceneApp (and explicitly in TexturedCubeApp)
2. States "26 files (13 header+source pairs)" (correct file count)
3. Done criteria #10 correctly allows `asset_manager_` matches in `hot_reload_gltf_app.*` only
4. TexturedCubeApp explicitly states `<chrono>` is kept

The contract is thorough, precise, and correctly addresses all spec requirements, spec-critic warnings (including the `AssetManager&` → `AssetManager*` fix), and frame-ordering concerns. No new issues found. Ready for human validation.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **CubeSceneApp — `#include <chrono>` must NOT be removed (section 6.5, line 401)**

  The contract instructs: *"Remove `#include <chrono>` (start_time_ stays but is used in setup and on_frame_begin)"*.

  This is contradictory and WOULD BREAK COMPILATION:
  - `CubeSceneApp` declares `std::chrono::steady_clock::time_point start_time_;` as a member in the header.
  - No transitive include chain (`entity.h`, `world.h`, `engine_context.h`, `component.h`) provides `<chrono>` — verified by source inspection.
  - Removing `#include <chrono>` from the header would cause a compilation error because `std::chrono::steady_clock::time_point` would be an unknown type.
  - The `.cpp` file also uses `std::chrono::steady_clock::now()` and `std::chrono::duration<float>` in the new `on_frame_begin()`.

  **Fix**: Replace the instruction with *"Keep `#include <chrono>` (start_time_ stays — required for `std::chrono::steady_clock::time_point`)"* or simply remove the `Remove #include <chrono>` line.

## Warnings

Non-blocking concerns for awareness:

1. **TexturedCubeApp (section 6.6) follows "Same changes as CubeSceneApp"** — Since the CubeSceneApp instructions contain a bug regarding `<chrono>`, the "same changes" reference for TexturedCubeApp could propagate the error if interpreted literally. The contract should explicitly state: *"Keep `#include <chrono>` (start_time_ stays)"* for all apps that have `start_time_`. Affected apps: `CubeSceneApp`, `TexturedCubeApp`, `PhongApp`, `AssetDemoApp`, `CubeApp`, `MultiMaterialApp`.

2. **Line 56: File count mismatch** — The contract says *"App subclasses (23 header+source pairs: 13 headers + 10 with source implementations)"* but then lists 26 files (13 headers + 13 source files) at lines 57–82. The annotation text is incorrect. Either correct the count to *"26 files (13 header+source pairs)"* or clarify the distinction between files listed vs files with significant implementation changes.

3. **Done criteria #10 (line 785): grep criteria is self-contradictory** — The criterion header says *"grep -r 'asset_manager_' src/cmd/apps/ returns zero matches"* but the parenthetical acknowledges that `HotReloadGltfApp` still has `asset_manager_` (as a raw pointer). This grep will NOT return zero matches. The criterion should be rephrased to *"grep -r 'asset_manager_' src/cmd/apps/ returns matches only in hot_reload_gltf_app.*"* or equivalent.

4. **Done criteria #11 (line 787): grep for `Entity::create`** — The criterion says *"grep -r 'Entity::create' src/engine/ returns zero matches (except entity.h private constructor comment)"*. The private constructor is `Entity(World& world, EntityId id)` which does NOT match the regex `Entity::create`. This is fine as-is, but the parenthetical is misleading — there should be no matches at all if `Entity::create` is fully removed. The parenthetical about "private constructor comment" seems to refer to a documentation comment that might mention `Entity::create` — this should be clarified.

5. **All 13 app headers should keep `<chrono>` if they have `start_time_`** — The contract correctly does NOT instruct `<chrono>` removal for most apps, which is good. Only CubeSceneApp's instruction erroneously says to remove it. Once fixed, the contract will be consistent.

## Required changes

Concrete, actionable changes requested:

1. **Section 6.5 (CubeSceneApp header changes, line 401)**: Change *"Remove `#include <chrono>` (start_time_ stays but is used in setup and on_frame_begin)"* to *"Keep `#include <chrono>` (start_time_ stays and requires `<chrono>`)"*.

## Suggested improvements

Optional ideas (not required):

1. **Explicitly document the 7 apps needing `on_frame_begin()` frame-ordering fix** — The edge case section already lists them, but a cross-reference in each app migration section would reduce risk. Currently, the per-app sections are generally clear about what goes where.

2. **Add a compile-time sanity check for EngineContext field ordering** — Consider suggesting a `static_assert(sizeof...(EngineContext) >= 7)` or similar to catch regressions.

3. **Clarify that `TexturedCubeApp` chrono include is NOT to be removed** — Since section 6.6 says "Same changes as CubeSceneApp", it's ambiguous whether the `<chrono>` removal instruction applies. TexturedCubeApp has `start_time_` and needs `<chrono>`. An explicit statement would prevent accidental breakage.

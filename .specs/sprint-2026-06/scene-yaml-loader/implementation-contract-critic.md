# Implementation Contract Review — Scene YAML Loader

## Re-review (2026-06-09)

All 3 blocking issues resolved. Contract accepted.

## Re-review (2026-06-10) — Model directive update

All model directive changes correctly applied. No new blocking issues. Contract accepted.

One minor warning added (see below).

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **YAML auto-detection priority contradicts spec (AC-015, API compatibility)** — **RESOLVED**: YAML check is now the first `else if` branch (before `scene == "triangle"`), with file-existence check and immediate error on missing file. Matches spec priority requirement.

- [x] **`prefab:` entity relationship incomplete (AC-005)** — **RESOLVED**: Renamed from `use_prefab:` to `prefab:` consistently. Entity with `prefab:` IS the prefab root entity (no wrapper). Instance overrides (name, transform composition, extra components, children) applied directly to it.

- [x] **Missing `#include <unordered_set>` in `scene_loader.h`** — **RESOLVED**: Added `#include <unordered_set>` to the includes block in `scene_loader.h`.

## Warnings

Non-blocking concerns for awareness:

- **`add_component_raw()` not listed in "Files allowed to change" table** — Step 5 adds `add_component_raw()` to `world.h`/`world.cpp`, but the "Files allowed to change" table (lines 73-77) only mentions the name-related changes for these files. The summary table should be updated to include the `add_component_raw` modification to avoid confusion.

- **`world.h` may need explicit `#include <string>`** — The header currently does not include `<string>`. Adding `std::string name_` to `EntityNode` and `get_name()`/`set_name()` methods that return `const std::string&` requires `std::string`. While it may be included transitively, the project convention is to include what you use. Should be explicitly added.

- **AC-010 and AC-011 not covered by unit tests** — The recommended 7-test set does not include tests for missing prefab file error (AC-010) or prefab with >1 entity error (AC-011). While "at least 7" allows this, these are nontrivial behaviors without regression coverage. Consider adding them.

- **Test 7 alternative example is misleading** — The contract shows an alternative test (lines 676-688) that uses `load_from_yaml` with a scene containing children hierarchy, not a prefab flow test. If this is meant to demonstrate Test 8 (children hierarchy) rather than Test 7 (prefab composition), the framing is confusing. Clarify which test the code corresponds to.

- **`add_component_raw` on World vs Entity** — The contract adds `add_component_raw()` as a public method on `World` (consistent with existing `add_component<T>()` template). An alternative would be to add it on `Entity` (e.g., `Entity::add_component_raw(unique_ptr<Component>)`) which keeps the entity-facing API unified. Not a blocking issue, but worth considering for encapsulation.

- **`<>` vs `""` for includes comment style** — Convention 1 says `#include "..."` for project headers and `<...>` for external/system headers. The contract's `scene_loader.h` snippet uses `#include <yaml-cpp/yaml.h>` correctly (external), but the note about include style is already covered by existing conventions. No action needed.

## Required changes — Resolution status

All required changes from the first review cycle have been addressed:

1. **[DONE]** YAML detection moved to FIRST branch in dispatch chain (before `scene == "triangle"`).
2. **[DONE]** Entity relationship clarified: entity with `prefab:` IS the prefab root entity, no wrapper.
3. **[DONE]** `#include <unordered_set>` added to `scene_loader.h`.
4. **[DONE]** "Files allowed to change" table updated with `add_component_raw` for `world.h`/`world.cpp`.
5. **[DONE]** `#include <string>` note added to `world.h` entry in allowed files table.
6. **[DONE]** Test 7 (compose_transform) and Test 8 (children hierarchy) clearly distinguished with separate titles and descriptions.
7. **[DONE]** AC-010 and AC-011 test coverage noted with explicit testing approaches described.

## Suggested improvements

Optional ideas (not required):

- For the `add_component_raw` addition: consider placing it on `Entity` rather than `World` for a cleaner public API. `Entity` already owns `add_component<T>()` and is the natural surface for component injection.
- Add a brief note about the `#include <string>` requirement in `world.h` for completeness, even if transitively available.

### New warning (Loop 4 — model directive update)

- **`demo.yaml` light colour differs from spec.** The contract's `demo.yaml` (line 581) uses `colour: [1.0, 1.0, 1.0]` for the directional light, but the spec's `demo.yaml` (spec.md line 124) uses `colour: [1.0, 1.0, 0.9]`. While this does not break AC-020 (which only checks entity count/names, not colour values), the contract should match the spec's demo YAML to avoid a visual difference in the warm/cool tone of the light. Recommend aligning to `[1.0, 1.0, 0.9]`.

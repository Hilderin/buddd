# Implementation Contract Review — cli-edit-scene-path

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Missing `#include <optional>` in `editor_app.h`** — The contract adds `std::optional<std::string>` as a constructor parameter and member variable in `editor_app.h`, but did not specify adding `#include <optional>`. Neither `editor_app.h` nor any of its transitive includes (`app.h` → `app_config.h`) currently include `<optional>`. This will cause a compilation error. The `Files allowed to change` section for `editor_app.h` must explicitly include `#include <optional>`.

**Resolution (2026-06-13)**: Fixed by author. `#include <optional>` is now explicitly specified in:
- `Files allowed to change` section for `editor_app.h` (line 50: "Add `#include <optional>`")
- Required implementation behavior section (line 84: "Add `#include <optional>` at the top of the file")
- Done criteria (line 304)

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- [x] **T1 / T4 test redundancy** — Both T1 (`buddd edit --frame 2` — no scene, just flags) and T4 (`buddd edit --frame 2` — flag, not scene path) specify the exact same input. They verify different ACs (T1: AC-001/AC-010; T4: AC-005 flag branch) but are functionally identical. Consider consolidating into a single test case or making T4 clearly distinct (e.g., `buddd edit --capture 2:path`).  
  **Resolution (2026-06-13)**: Fixed. T4 now uses `--capture 2:/tmp/test_cap.png` instead of `--frame 2`.

- [x] **`to_string()` missing `be::` namespace prefix in code snippet (line 114)** — The code snippet in section 2 uses `to_string(open_result.error())` without the `be::` prefix. The existing `editor_app.cpp` has `namespace be = buddd::engine;` and the codebase convention (see `main.cpp` line 58: `be::to_string(args.error())`) uses the explicit prefix. The snippet should read `be::to_string(...)` to match conventions.  
  **Resolution (2026-06-13)**: Fixed. The code snippet now reads `be::to_string(open_result.error())`.

- [x] **T7 / T8 exit code expectation ambiguous** — The contract states "Exit 1 (headless: EditorApp::setup fails because no display)". However, the default build config has `BUDDD_HAS_DISPLAY=ON` (see `CMakeLists.txt` line 23), and `run_buddd()` already sets `SDL_VIDEO_DRIVER=offscreen`. With `BUDDD_HAS_DISPLAY=ON`, `EditorApp::setup()` succeeds and the editor runs normally, producing exit 0. The done criteria note ("the exact assertion depends on headless vs. display build") only appears in the T1 note, not for T7/T8. Clarify expected behavior for both `BUDDD_HAS_DISPLAY=ON` and `=OFF` builds consistently across all tests.  
  **Resolution (2026-06-13)**: Fixed. Both T7 and T8 now explicitly document expected behavior for both `BUDDD_HAS_DISPLAY=ON` and `=OFF` builds, with the key assertion being absence of `"Scene file not found"`.

- [x] **`parse_running_args` silently ignores unknown args** — The contract documents that `edit scene.yaml extra_arg` is silently handled because `parse_running_args` ignores unknown arguments. This differs from the `run` command which emits a warning for extra positional args. This is correct per spec but should be confirmed as intentional during implementation review — implementers may not expect this difference.  
  **Resolution (2026-06-13)**: The contract now explicitly documents this difference in the edge-case table entry for `buddd edit scene.yaml extra_arg` (line 262), noting that the `run` command's extra-arg warning does not apply to `edit`.

- [x] **`is_yaml_path` lambda vs `is_yaml_file` naming** — The spec calls the helper `is_yaml_file` (spec A-05, main.cpp lines 99-108) but the contract's code snippet names it `is_yaml_path`. This is cosmetic and doesn't affect correctness, but the name mismatch could cause confusion during code review.  
  **Resolution (2026-06-13)**: Fixed. The lambda is now named `is_yaml_file` in the contract, matching the spec's naming convention.

## Required changes

Concrete, actionable changes requested:

1. **Add `#include <optional>` to `editor_app.h`** in the `Files allowed to change` section under `editor_app.h`, before or alongside the `scene_path_` member.
2. **Fix `to_string(...)` → `be::to_string(...)`** in the code snippet at line 114 of the contract.
3. **Clarify T7/T8 exit code expectation** to document behavior under both `BUDDD_HAS_DISPLAY=ON` and `=OFF` builds, not just the `=OFF` case.
4. **Consolidate T1 and T4** to avoid redundant test cases, or make T4 use a distinct invocation (e.g., `--capture` instead of `--frame`).

## Suggested improvements

Optional ideas (not required):

- Rename `is_yaml_path` lambda to `is_yaml_file` to match the `run` command's naming convention from spec A-05 and main.cpp lines 99-108.
- In the edge-case table, note that the `to_string()` call in `EditorApp::setup()` uses `be::to_string()` to match existing code patterns.
- Consider extracting the `is_yaml_file` lambda into a small shared helper in `main.cpp` (even though spec A-05 permits duplication, a single helper reducing copy-paste would improve maintainability — this is a suggestion, not a requirement).

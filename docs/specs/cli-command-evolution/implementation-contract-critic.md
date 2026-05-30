# IMPL-007 Critic Review — CLI Command Evolution: Demo System & Empty Run

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

IMPL-007 is a well-structured and generally thorough implementation contract that faithfully translates most of SPEC-007's requirements into actionable code. The file change list is precise, the Done criteria map directly to all spec ACs/SCs, the tests cover the required CONST-002 testable paths, and the constitution compliance checks (CONST-001, CONST-002) are correctly referenced.

**However, a blocking code correctness issue exists**: the call to `run_triangle_demo()` in `demo_command.cpp` uses single-dereference (`*platform`, `*device`) where double-dereference (`**platform`, `**device`) is required. The code as written would not compile, because `*platform` evaluates to `std::unique_ptr<buddd::engine::Platform>&` (not `buddd::engine::Platform&`) and `*device` evaluates to `std::unique_ptr<buddd::engine::RenderDevice>&` (not `buddd::engine::RenderDevice&`). This is confirmed by the existing codebase pattern: `test_command.cpp` uses `**device` for `setup_triangle(RenderDevice&)` at line 54, and `run_command.cpp` uses `**device` at line 42 for the same signature.

This bug originates from a contradiction in the source spec (SPEC-007). The spec's **Key entities** section correctly stipulates `(**platform)` and `(**device)` (matching the `Result<unique_ptr<T>>` semantics), but the spec's **Required implementation behavior** section incorrectly shows `*platform` and `*device`. The contract copies the erroneous form.

**Verdict**: Rejected. The contract must be fixed to resolve the single-dereference bug before implementation can proceed. The fix is straightforward (change two characters in `demo_command.cpp`), but it is a blocking issue because the code is not compilable as written.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **B-01: `run_triangle_demo(*platform, *device, ...)` uses single-dereference instead of double-dereference in `demo_command.cpp`**

  **Description**: In the contract's `demo_command.cpp` (lines 265–266 of the contract), the call:
  ```cpp
  return bc::run_triangle_demo(*platform, *device, argc - 2, argv + 2);
  ```
  passes `*platform` and `*device`, but `platform` is `Result<std::unique_ptr<buddd::engine::Platform>>` and `device` is `Result<std::unique_ptr<buddd::engine::RenderDevice>>`. Dereferencing a `Result<unique_ptr<T>>` once gives `unique_ptr<T>&`, not `T&`. The `run_triangle_demo` function signature takes `buddd::engine::Platform&` and `buddd::engine::RenderDevice&`. The correct call requires double-dereference:
  ```cpp
  return bc::run_triangle_demo(**platform, **device, argc - 2, argv + 2);
  ```

  **Evidence**: The existing codebase confirms this pattern:
  - `src/cmd/commands/test_command.cpp` line 54: `bc::setup_triangle(**device)` — `setup_triangle` takes `RenderDevice&`
  - `src/cmd/commands/run_command.cpp` line 42: `bc::setup_triangle(**device)` — same signature
  - The spec's **Key entities > DemoCommand** section correctly says: _"Passes `(**platform)` and `(**device)` to the selected demo function."_

  The spec's **Required implementation behavior** section (which the contract copied) incorrectly uses `*platform, *device`. The contract should follow the correct semantics, not the erroneous spec text.

  **Fix**: Change `*platform` to `**platform` and `*device` to `**device` at the call site in `demo_command.cpp`.

## Warnings

Non-blocking concerns for awareness:

- [x] **W-01: Unnecessary `#include "demos/demo_helpers.h"` in `demo_command.cpp`**

  `DemoCommand::run()` does not call `setup_triangle()` or use any symbol from `demo_helpers.h`. The include is unnecessary. The triangle demo header (`triangle_demo.h`) already includes `demo_helpers.h` transitively through its `.cpp` file. While harmless (the include guard prevents actual recompilation cost), it violates the principle of minimal includes and creates a misleading dependency. Remove it.

- [x] **W-02: Misleading comment about `WindowConfig::title` type in `demo_command.cpp`**

  The contract's `demo_command.cpp` code block contains this comment:
  ```cpp
  // We construct the title in a local buffer since WindowConfig takes a
  // std::string_view — the buffer must outlive the create_window call.
  ```
  However, `WindowConfig::title` is `std::string` (defined in `src/engine/window/window.h` line 9), **not** `std::string_view`. The local `std::string` variable is unnecessary for lifetime reasons (the designated initializer copies into the `std::string` member), though the code compiles and works correctly regardless. The misleading comment suggests an incorrect understanding of the API and could confuse future maintainers. Update or remove the comment.

- [x] **W-03: Spec-internal contradiction propagated to contract**

  The source spec (SPEC-007) contains a contradiction between its **Key entities** section (which correctly says `(**platform)` and `(**device)`) and its **Required implementation behavior** section (which incorrectly says `*platform` and `*device`). The contract faithfully copies the erroneous version. While this warning is redundant with B-01, it is noted here separately because the spec should also be corrected to avoid confusion in future contract iterations.

## Required changes

Concrete, actionable changes requested:

1. [x] **Fix B-01**: In `src/cmd/commands/demo_command.cpp`, changed `*platform` to `**platform` and `*device` to `**device`.

2. [x] **Fix W-01**: Removed `#include "demos/demo_helpers.h"` from `src/cmd/commands/demo_command.cpp`.

3. [x] **Fix W-02**: Updated the comment in `src/cmd/commands/demo_command.cpp` to correctly state that `WindowConfig::title` is `std::string`.

4. [x] **(Spec-level) Resolve the `*platform`/`**platform` contradiction in SPEC-007**: The **Required implementation behavior** section in `docs/specs/cli-command-evolution/spec.md` was corrected to use `**platform` and `**device`.

## Suggested improvements

Optional ideas (not required):

- [ ] **Add explicit verification of the dereference pattern in the Done criteria**: Consider adding a Done criterion (or extending AC-022/AC-024) to verify that `demo_command.cpp` calls demo functions with `**` rather than `*` on Result-wrapped unique_ptrs. This would prevent this class of bug from recurring in future demo dispatch additions.

- [ ] **Add an include-dependency audit criterion**: Consider adding a check (or widening the existing AC-024 review) that verifies each `.cpp` file only includes what it directly uses, to prevent unnecessary includes like the `demo_helpers.h` issue identified in W-01.

- [ ] **Consider `std::format` or string concatenation for the window title**: The current window title construction concatenates two `std::string` temporaries. This is functionally correct but could be slightly clearer with `std::format` (available in C++26). However, this is purely stylistic and the project may not have adopted `std::format` yet, so this is a low-priority suggestion.

## Review summary

| Category | Count |
|---|---|
| Blocking issues | 1 |
| Warnings | 3 |
| Required changes | 4 (3 contract + 1 spec) |
| Suggested improvements | 3 |
| Verdict | Rejected |

## Detailed analysis

### 1. Does the contract faithfully implement SPEC-007?

**Mostly, with one blocking exception:**
- All user-visible output strings match the spec exactly ✓
- The dispatch logic, help text, demo usage text, unknown command handling are all correct ✓
- The file structure (create/remove/move/modify) matches the spec ✓
- The `argc`/`argv` contract (`argc - 2`, `argv + 2` passed to demos) is correct ✓
- Extra-arguments warning iterates from `argv[3]` as specified ✓
- **BLOCKING**: The `**platform`/`**device` dereference is wrong (see B-01)

### 2. Are all file changes precisely specified?

Yes. The contract explicitly lists every file to create, remove, modify, and leave unchanged, with exact code content for each. ✓

### 3. Are include paths correct?

All include paths correctly resolve through the `src/cmd/` include root (for `demos/demo_helpers.h`) and the engine's public include directory (for `platform/platform.h` etc.). ✓

One minor issue: `demo_helpers.cpp` (moved to `src/cmd/demo/`) uses `#include "demo_helpers.h"` which resolves to the sibling file via compiler's same-directory search — correct. ✓

### 4. Are all exact output strings matching the spec?

All verified:
- `k_demo_usage` matches spec, including trailing `\n` ✓
- `k_usage_text` (help) matches spec ✓
- Triangle demo diagnostics use `"Demo"` prefix, not `"Render test"` ✓
- Unknown demo/command messages match spec ✓
- Warning output format matches spec ✓
- RunCommand stdout messages match spec ✓

### 5. Are the Done criteria complete and verifiable?

All 32 Done criteria map to ACs 1–24 plus SCs 1–3 and the new test requirements. Each criterion specifies a verifiable condition (file existence, grep output, shell command output, stdout/stderr content, exit code, build success). ✓

### 6. Does the demo function signature match the spec?

```cpp
[[nodiscard]] auto run_triangle_demo(
    buddd::engine::Platform& platform,
    buddd::engine::RenderDevice& device,
    int argc, const char* const* argv) -> int;
```
Matches the spec exactly. ✓

### 7. Is the argc/argv contract consistent?

`DemoCommand::run()` correctly receives the full `argc`/`argv` and passes `argc - 2` / `argv + 2` to the per-demo function. The demo function receives `argv[0]` as the demo name. ✓

### 8. Are tests adequate for CONST-002 compliance?

Yes. The contract adds:
- `buddd demo with no name prints usage and exits 1` ✓
- `buddd demo unknownname prints error and exits 1` ✓
- `buddd test is unknown command` ✓
- `buddd demo triangle runs and completes` (display-guarded) ✓
- Existing help tests updated to check for `"demo"` instead of `"test"` ✓

All unconditionally testable paths are covered. CONST-002 is satisfied (assuming the blocking bug is fixed). ✓

### 9. Are there any contradictions or ambiguities?

- **Spec-internal contradiction**: The spec says `(**platform)` in Key entities but `*platform` in Required behavior. The contract propagates the latter. This is the root cause of B-01.
- **No other contradictions**: The contract is internally consistent and consistent with the rest of the spec and constitution.

### 10. Constitutional compliance

- **CONST-001**: All files under `src/cmd/` use only engine abstraction headers (`platform/platform.h`, `window/window.h`, `render/render_device.h`, `render/primitive_topology.h`). No SDL3, OpenGL, or GLM headers are included. The grep verification command is provided ✓
- **CONST-002**: Tests are provided for all unconditionally testable paths ✓
- **CONST-003**: Documentation impact is correctly assessed as minimal ✓
- **CONST-004**: Security impact is correctly assessed as none ✓

# Governance Review — CLI `buddd edit [<scene>]`

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec ↔ Implementation Contract**: Contract faithfully implements all spec requirements — 4-step dispatch, `is_regular_file()` validation, `std::optional<std::string>` constructor, flags at `argv[3..]`, help text update. No contradictions.
- [x] **Contract ↔ Code**: All 5 allowed files modified exactly as specified. `editor_app.h` has `#include <optional>`, parameterised constructor, `scene_path_` member. `editor_app.cpp` stores scene path and calls `open_scene()` after `setup()` succeeds. `main.cpp` has 4-step dispatch with correct validation. `help_command.h` updated. All 8 forbidden file categories untouched.
- [x] **Code ↔ Tests**: 8 integration tests cover all required scenarios (no arg, nonexistent file, unknown arg, flag-only, `.yaml` extension-only, valid YAML path, case-insensitive `.YML`). Tests verify dispatch correctness and error messages.
- [x] **Wiki ↔ Code**: Wiki accurately reflects the implemented behaviour:
  - `data-flow.md`: 4-step edit dispatch diagram matches `main.cpp` logic.
  - `module-map.md`: EditorApp constructor signature matches `editor_app.h`; subcommand behaviour matches `main.cpp`.
  - `business-rules.md`: CLI output table, exit codes, and observability messages all match spec and implementation.
- [x] **Spec ↔ Wiki**: Spec's "Existing documentation that must be updated" section lists the same 3 wiki pages that were updated. Changes are consistent.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-014 (CLI App System)**: The implementation respects ADR-014 — `EditorApp` is a subclass of `App` using `run_app()` lifecycle. The `edit` dispatch follows the same pattern as `run`. No changes to `App` base class or `run_app()`.
- [x] **ADR-027 (Editor Architecture)**: Implementation respects ADR-027 — `EditorApp` constructor extended with optional scene path parameter (backward-compatible). `Editor::open_scene()` called during `setup()`. No changes to `Editor` library internals. Architecture boundary preserved (no SDL3/OpenGL/GLM in `src/cmd/` or `src/editor/`).
- [x] **ADR-019 (Architecture Boundaries)**: Verified by `grep` — no SDL3, OpenGL, or GLM headers included in any modified `src/cmd/` file. The 4 modified source files (`editor_app.h`, `editor_app.cpp`, `main.cpp`, `help_command.h`) contain zero platform/graphics header includes.
- [x] **ADR-001 (Result<T> error pattern)**: `EditorApp::setup()` returns `Result<void>`, `Editor::open_scene()` returns `Result<void>` — both checked. Error propagation follows ADR-001 conventions (`make_error`, `be::to_string`, `[[nodiscard]]`).
- [x] **New ADR not required**: Implementation contract correctly determined no new ADR is warranted — changes are backward-compatible extensions within existing ADR-014 and ADR-027 decisions.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/architecture/data-flow.md`**: Updated with 4-step edit dispatch diagram (lines 17-24) and output table rows for edit variants (lines 64-66). Correct and consistent with implementation.
- [x] **`docs/wiki/architecture/module-map.md`**: Updated EditorApp description with `scene_path` constructor parameter (line 432), subcommand behavior section (lines 360-361), and CLI integration (line 444). Correct.
- [x] **`docs/wiki/domain/business-rules.md`**: CLI output behavior table includes edit entries (lines 25-28), exit codes include edit failures (lines 244-245), observability messages include edit errors (lines 210-211). Correct.
- [x] **Wiki does not become law**: All wiki content is operational documentation reflecting current state. No new rules or binding conventions introduced.

## Warnings

Non-blocking concerns for awareness:

- Test T1 (`buddd edit --frame 2`) asserts for `"Editor: layout file: buddd_editor.ini"` in stderr, which only appears when `BUDDD_HAS_DISPLAY=ON`. A `BUDDD_HAS_DISPLAY=OFF` build would fail this test. Acceptable given the project default build config.
- Test T4 (`buddd edit --capture ...`) does not assert exit code — only checks absence of error string. Minor robustness gap, not blocking.
- The contract's struct-like "Required tests" table (T1–T10) is informative; the actual test file contains 8 test cases (T1–T8). T9 and T10 (capture with scene/flags) are listed as manual/E2E only. This is acceptable per contract design.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None required. All ADRs are respected. Wiki is already updated. No new ADR needed.

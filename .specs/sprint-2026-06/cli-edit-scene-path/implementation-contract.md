# IMPL-035 — CLI `buddd edit [<scene>]` — open editor with scene path

## Source spec

`/home/guillaume/Documents/Projects/buddd/.specs/sprint-2026-06/cli-edit-scene-path/spec.md`

## Goal

Extend `buddd edit` to accept an optional scene YAML file path. When a path ending in `.yaml` or `.yml` is provided and the file exists, the editor opens with that scene loaded during startup. When no path is provided, the editor opens with an empty untitled scene (existing behaviour unchanged). Non-existent or non-regular files produce an error on stderr and exit code 1 without opening the editor. Flags `--frame` and `--capture` continue to work after the scene path.

## Non-goals

- No changes to `Editor::open_scene()`, `Editor::setup()`, or any existing `Editor` APIs (spec NG-03).
- No changes to `App` base class or `run_app()` lifecycle (spec NG-06).
- No changes to `SceneApp` or any other `App` subclass (spec NG-07).
- No changes to `run` command behaviour (spec NG-01).
- No new CLI flags for the editor (spec NG-02).
- No multi-scene support (spec NG-04).
- No changes to save system, dirty state, or file dialogs (spec NG-05).
- No changes to CMake build system, test infrastructure, or dependencies.
- No changes to test helpers (`tests/test_helpers.h`), which already support the required patterns.

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-014 | CLI App System — centralised render loop, `App` lifecycle, `run_app()`, `parse_running_args()`. The `edit` dispatch follows the same pattern as `run`. |
| ADR-027 | Editor Architecture — `EditorApp` is an `App` subclass, `Editor::open_scene()` exists and can be called during setup. Decision 2 (reuse `App` lifecycle) constrains how the scene path is passed. |
| ADR-001 | `Result<T>` error pattern — `Editor::open_scene()` returns `Result<void>`, `setup()` returns `Result<void>`. Error handling must follow this pattern. |

## Files to inspect

| File | Purpose |
|---|---|
| `src/cmd/main.cpp` | Current `edit` dispatch (lines 71–79), `is_yaml_file` lambda pattern (lines 99–108), `run` command YAML validation pattern with `std::filesystem::exists()` (line 111). |
| `src/cmd/apps/editor_app.h` | Current `EditorApp` declaration — constructor takes no args. Must be extended with `std::optional<std::string>` parameter. |
| `src/cmd/apps/editor_app.cpp` | Current `EditorApp::setup()` — calls `editor_->setup(ctx)` only. Must add `open_scene()` call after setup succeeds. |
| `src/cmd/commands/help_command.h` | `k_usage_text` constant — the `edit` line must be updated. |
| `src/cmd/app.h` | `App` base class and `run_app()` — both remain unchanged. Confirms no lifecycle changes needed. |
| `src/cmd/app_config.h` | `parse_running_args()` signature — takes `start` index. Used with `start=2` (no scene) or `start=3` (scene at argv[2]). |
| `src/editor/editor.h` | `Editor::open_scene(const std::string& path) -> Result<void>` signature and behaviour: on success sets `current_file_path_`, clears dirty flag, updates title. On failure shows error modal in editor. |
| `tests/cmd/cli_app_tests.cpp` | Existing integration tests — pattern for `run_buddd()` usage, `[cli][app]` tag convention. |
| `tests/test_helpers.h` | `run_buddd()` helper — spawns buddd with args, captures stdout/stderr/exit code. Already sets `SDL_VIDEO_DRIVER=offscreen`. |

## Files allowed to change

| File | Change summary |
|---|---|
| `src/cmd/main.cpp` | Replace the `edit` branch (lines 71–79) with dispatch logic that detects optional scene path and validates it. |
| `src/cmd/apps/editor_app.h` | Add `#include <optional>`. Add `EditorApp(std::optional<std::string> scene_path = std::nullopt)` constructor. Add `std::optional<std::string> scene_path_` member. |
| `src/cmd/apps/editor_app.cpp` | Store scene path in constructor. In `setup()`, after `editor_->setup(ctx)` succeeds, call `editor_->open_scene(*scene_path_)` if scene path is present. |
| `src/cmd/commands/help_command.h` | Update `k_usage_text` `edit` line from `"  edit      Open the editor [--frame N] [--capture N:path]\n"` to `"  edit      Open the editor (optionally with a scene file)\n"`. |
| `tests/cmd/cli_app_tests.cpp` | Add integration tests for all new behaviours (see Required tests section). |

## Files forbidden to change

| File | Reason |
|---|---|
| `src/cmd/app.h` | Spec NG-06 — no changes to App base class. |
| `src/cmd/app.cpp` | Spec NG-06 — no changes to `run_app()`. |
| `src/cmd/app_config.h` / `src/cmd/app_config.cpp` | Spec NG-06, NG-02 — parse_running_args unchanged. |
| `src/editor/editor.h` / `src/editor/editor.cpp` | Spec NG-03 — `Editor::open_scene()` is used as-is. |
| `src/editor/**` | No editor library changes. |
| `tests/test_helpers.h` | Already provides all needed helpers. No changes needed. |
| `CMakeLists.txt` or any `CMakeLists.txt` | No build system changes. |
| `docs/**` (wiki pages) | Changes to wiki documentation are handled by the wiki-agent step, not the code-implementer. |

## Existing conventions to follow

- **YAML auto-detection**: Reuse the same extension-check pattern as `main.cpp` lines 99–108 (find last `.`, case-fold to lowercase, compare to `"yaml"`/`"yml"`). Do NOT extract into a shared function — small lambdas duplicated across `run` and `edit` dispatch branches are acceptable (spec A-05).
- **File validation**: Use `std::filesystem::is_regular_file()` (not `exists()`) to reject directories and symlinks-to-non-files. The `run` command uses `exists()` (line 111) — the `edit` command intentionally uses a stricter check per spec.
- **Error message format**: Use `BUDDD_LOG_ERROR("Scene file not found: '{}'", path)` matching the `run` command pattern on line 114.
- **Editor window on corrupt YAML**: The editor opens, `Editor::open_scene()` fails internally (shows error modal), and exit code is 0 (editor lifecycle completed normally). Do NOT propagate `open_scene` failure from `setup()`.
- **Logging**: Add `BUDDD_LOG_INFO("Editor: opening scene: {}", path)` when `open_scene()` is called from `EditorApp::setup()` (spec Observability section).
- **Constructor pattern**: `EditorApp` currently has default constructor `EditorApp() = default`. The new constructor should use `= default` in the `.cpp` file or a member initializer list. The `scene_path_` member should be initialized via member initializer.
- **`[[nodiscard]]`**: All `Result<void>`-returning functions are marked `[[nodiscard]]`. The `open_scene()` call result must be checked (even though setup won't return error for it, the `[[nodiscard]]` attribute on `Editor::open_scene()` enforces checking).
- **Test conventions**: Use `run_buddd()` with `[cli][app]` tag. Use `std::string::npos` checks on `res.stderr_str`/`res.stdout_str`. For headless tests, `SDL_VIDEO_DRIVER=offscreen` is already handled by `run_buddd()`.
- **Error output format**: Errors use `BUDDD_LOG_ERROR` which produces `"[ERROR] [App] ..."` format on stderr.

## Required implementation behavior

### 1. EditorApp constructor change

`editor_app.h`: Add `#include <optional>` at the top of the file (with the other standard library includes). Add constructor with optional scene path:
```cpp
explicit EditorApp(std::optional<std::string> scene_path = std::nullopt);
```
Remove the existing `EditorApp()` default constructor declaration (replaced by the parameterised one with default).

`editor_app.h`: Add private member:
```cpp
std::optional<std::string> scene_path_;
```

`editor_app.cpp`: Change constructor to:
```cpp
EditorApp::EditorApp(std::optional<std::string> scene_path)
    : scene_path_(std::move(scene_path)) {}
```

### 2. EditorApp::setup() scene loading

In `editor_app.cpp`, modify `setup()` — after `editor_->setup(ctx)` succeeds (the `Result<void>` is valid), if `scene_path_` has a value:
1. Log: `BUDDD_LOG_INFO("Editor: opening scene: {}", *scene_path_)`
2. Call `editor_->open_scene(*scene_path_)` and discard the result (do NOT return error from setup — editor shows internal error modal on failure per spec and Observability table)

The code structure:
```cpp
if (scene_path_) {
    BUDDD_LOG_INFO("Editor: opening scene: {}", *scene_path_);
    auto open_result = editor_->open_scene(*scene_path_);
    if (!open_result) {
        // Error is handled internally by Editor (error modal) — don't propagate
        BUDDD_LOG_WARN("Scene load failed: {}", be::to_string(open_result.error()));
    }
}
```

The `BUDDD_LOG_WARN` is informational only — the editor already shows an error modal. This log exists so the Observability table's `"Scene load failed: {}"` signal is produced, matching existing behaviour from `editor.cpp`.

### 3. main.cpp edit dispatch rewrite

Replace the current `edit` branch (lines 71–79) with the following logic:

```cpp
if (cmd == "edit") {
    // Determine if a scene path was provided
    std::optional<std::string> scene_path;
    int flags_start = 2;

    if (argc >= 3 && argv[2] != nullptr && argv[2][0] != '\0') {
        std::string_view arg{argv[2]};

        // Check if it looks like a YAML file (case-insensitive .yaml/.yml)
        auto is_yaml_file = [](std::string_view path) -> bool {
            auto pos = path.rfind('.');
            if (pos == std::string_view::npos || pos == path.size() - 1)
                return false;
            std::string_view ext = path.substr(pos + 1);
            std::string lower_ext;
            for (auto c : ext)
                lower_ext.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            return (lower_ext == "yaml" || lower_ext == "yml");
        };

        if (is_yaml_file(arg)) {
            // YAML path: validate file existence before creating the editor
            if (!std::filesystem::is_regular_file(arg)) {
                BUDDD_LOG_ERROR("Scene file not found: '{}'", arg);
                return EXIT_FAILURE;
            }
            scene_path = std::string(arg);
            flags_start = 3;
        } else if (arg[0] == '-') {
            // Flag argument (e.g. --frame, --capture) — no scene, use default flags_start=2
        } else {
            // Unknown positional argument
            BUDDD_LOG_ERROR("Unknown argument for edit: '{}'", arg);
            return EXIT_FAILURE;
        }
    }

    bc::app::EditorApp editor_app{std::move(scene_path)};
    auto args = bc::parse_running_args(argc, argv, flags_start);
    if (!args) {
        BUDDD_LOG_ERROR("Error: {}", be::to_string(args.error()));
        return EXIT_FAILURE;
    }
    return bc::run_app(editor_app, *args);
}
```

Key details:
- The `is_yaml_file` lambda replicates the `run` command's `is_yaml_file` pattern (lines 99–108). It is intentionally duplicated (spec A-05).
- `std::filesystem::is_regular_file()` is used (not `exists()`) to reject directories and symlink-to-non-file cases (spec AC-003, edge-case table).
- `flags_start = 3` when a scene path is present at argv[2]; `flags_start = 2` otherwise. `parse_running_args` silently skips unknown-looking positional args at argv[2] when `flags_start=2` (it only processes `--frame` and `--capture`).
- The empty string check (`argv[2][0] != '\0'`) ensures `buddd edit ''` is treated as "no argument" (spec edge case table).

### 4. Help text update

In `help_command.h`, change the `edit` line from:
```
"  edit      Open the editor [--frame N] [--capture N:path]\n"
```
to:
```
"  edit      Open the editor (optionally with a scene file)\n"
```

The `--frame`/`--capture` flags line remains unchanged:
```
"Flags for run/edit: --frame N, --capture N:path\n"
```

### 5. No BUDDD_HAS_DISPLAY guard change

The existing `#ifndef BUDDD_HAS_DISPLAY` guard in `EditorApp::setup()` returns an error when no display is available. This runs BEFORE the scene-loading code, which is correct — the scene path is already parsed in `main.cpp`, and the error is returned before `open_scene()` is attempted.

## Required tests

All tests use `run_buddd()` from `tests/test_helpers.h` with tag `[cli][app]`.

### Unit-level tests (in `tests/cmd/cli_app_tests.cpp`)

Add the following test cases:

| # | Test case | What it verifies | AC trace |
|---|---|---|---|
| T1 | `buddd edit --frame 2` (no scene, just flags) | Exit 0, editor opens. The editor requires display, so in headless we check for the expected error from `EditorApp::setup()`. **Alternative**: use `run_buddd("edit --frame 2 --log-level=info")` and verify the editor-related INFO log (e.g., `"Editor: layout file: buddd_editor.ini"`) appears on stderr, proving the editor ran. | AC-001, AC-010 |
| T2 | `buddd edit nonexistent.yaml` | Exit 1, stderr contains `"Scene file not found"`, stdout does NOT contain `"Window opened"`. | AC-003 |
| T3 | `buddd edit somearg` (unknown positional arg) | Exit 1, stderr contains `"Unknown argument for edit"`. | AC-005 (unknown arg branch) |
| T4 | `buddd edit --capture 2:/tmp/test_cap.png` (capture flag, not scene path) | Exit 0 — flag should not be mistaken for an unknown arg. | AC-005 (flag branch) |
| T5 | `buddd help` shows updated `edit` description | stdout contains `"optionally with a scene file"` in the `edit` line. | AC-008 |
| T6 | `buddd edit .yaml` (extension-only, no filename) | Exit 1 — `is_regular_file(".yaml")` returns false unless a literal file `.yaml` exists, which is not expected in the test environment. | spec edge case |

### Tests requiring a temp YAML file (in `tests/cmd/cli_app_tests.cpp`)

Because the editor requires a display, headless verification of scene-loading success is limited. Instead, these tests verify the dispatch/cli behaviour up to the point of `EditorApp::setup()` (which fails in headless due to `BUDDD_HAS_DISPLAY` guard). The dispatch correctness is verified by checking exit codes and error messages, not by checking window title.

| # | Test case | What it verifies | AC trace |
|---|---|---|---|
| T7 | Create temp valid `.yaml` file, run `buddd edit <temp.yaml> --frame 2` | **`BUDDD_HAS_DISPLAY=ON` (default)**: `EditorApp::setup()` succeeds, exit 0. Editor runs headless with `SDL_VIDEO_DRIVER=offscreen` set by `run_buddd()`. **`BUDDD_HAS_DISPLAY=OFF`**: exit 1, stderr contains the expected "editor requires a display" error. **The key assertion regardless of build**: the exit is NOT exit 1 with "Scene file not found" — that means the file was found and accepted. | AC-002 (partial — full window verification requires display) |
| T8 | Create temp valid `.YML` file (upper-case), run `buddd edit <temp.yml>` | Same exit-code logic as T7 depending on display build. The critical assertion: produces "Scene file not found" error **neither** in `BUDDD_HAS_DISPLAY=ON` nor `=OFF` builds — confirming case-insensitive extension matching. | AC-004 |

### Tests that verify capture still works (headless-compatible)

| # | Test case | What it verifies | AC trace |
|---|---|---|---|
| T9 | `buddd edit --capture 2:/tmp/cap_edit.png --frame 2` | Exit 1 (headless/display), but the key point is the CLI dispatch doesn't crash. Alternatively, this test can be a **manual/E2E test** requiring display — see E2E section. | AC-010 (capture with no scene) |
| T10 | Create temp valid `.yaml`, run `buddd edit <temp.yaml> --capture 2:/tmp/cap_edit_scene.png --frame 2` | Exit 1 (headless: display required). Verifies flags after scene path are accepted (not rejected as unknown args). | AC-006 (partial) |

### E2E / Integration verification

| Method | Description | AC trace |
|---|---|---|
| Manual test with display | Launch `buddd edit assets/scenes/demo.yaml` on a display; verify window opens, title shows `"demo.yaml — Buddd Editor"`, entities visible in tree. | AC-002 (full) |
| Manual test with display | Launch `buddd edit nonexistent.yaml`; verify error on stderr and no window. | AC-003 (full) |
| Manual test with display | Launch `buddd edit` (no arg); verify title `"Untitled — Buddd Editor"`, empty scene. | AC-001 (full) |
| Manual test with display | Launch `buddd edit assets/scenes/demo.yaml --frame 2 --capture 2:/tmp/cap.png`; verify capture file created. | AC-006, AC-007 (full) |
| Corrupt YAML manual test | Create temp invalid YAML, run `buddd edit <path>`; verify editor opens with error modal, exit 0. | AC-009 |

The E2E manual tests are supplementary. The automated headless tests (T1–T10) provide the core dispatch verification.

## Edge cases

| Case | Required behavior |
|---|---|
| `buddd edit` (no arg) | No scene path → empty editor. `flags_start=2` (current behaviour). |
| `buddd edit ''` (empty string) | Empty string check (`argv[2][0] != '\0'`) → treated as no argument. Empty editor. |
| `buddd edit scene.yaml` (valid YAML file) | `is_yaml_file()` returns true, `is_regular_file()` returns true → path stored, `flags_start=3`. |
| `buddd edit nonexistent.yaml` (non-existent YAML) | `is_yaml_file()` returns true, `is_regular_file()` returns false → error + exit 1. |
| `buddd edit /path/to/dir/` (directory) | `is_regular_file()` returns false for directories → error + exit 1. |
| `buddd edit symlink_to_scene.yaml` (symlink to regular file) | `is_regular_file()` follows symlinks → returns true for symlink-to-file → accepted. |
| `buddd edit symlink_to_dir.yaml` (symlink to directory) | `is_regular_file()` returns false for symlink-to-directory → error + exit 1. |
| `buddd edit --frame 2` (flag, no scene) | `argv[2]` starts with `-` → treated as flag, no scene. `flags_start=2`. |
| `buddd edit --capture 2:/tmp/p.png` (capture flag, no scene) | Same as above — starts with `-`. No scene. `flags_start=2`. |
| `buddd edit somearg` (unknown arg) | Not YAML, not flag → error + exit 1. |
| `buddd edit .yaml` (just extension) | `rfind('.')` finds the dot at position 0, extension is `"yaml"` → `is_yaml_file` returns true. `is_regular_file(".yaml")` is checked → unless file `.yaml` literally exists, error + exit 1. |
| `buddd edit scene.YAML` (upper-case extension) | Case-folded to `"yaml"` → detected as YAML path. |
| `buddd edit scene.yml` (.yml extension) | Detected as YAML path. |
| `buddd edit scene.yaml --frame 2` (flags after scene) | `flags_start=3`, `parse_running_args` parses `--frame 2` from argv[3..] correctly. |
| `buddd edit scene.yaml extra_arg` (extra arg after scene) | `parse_running_args` with `flags_start=3` — same as `run` command's "extra positional argument" behaviour. Unknown flags are silently ignored by `parse_running_args`. The old `run` command's extra-arg warning code (lines 182–203) does NOT apply to `edit` — it only runs for the `run` code path. This is acceptable per spec edge case table. |
| `buddd edit --frame 2 scene.yaml` (flags before scene path) | Dispatch logic never reaches YAML check because `argv[2]` is `--frame` (starts with `-`). The scene path is mistakenly not parsed. The editor opens empty. This is a documented limitation — same as `run` (spec A-04, Q-04). |
| Corrupt YAML file | `is_regular_file()` passes, file is valid regular file. `EditorApp::setup()` calls `open_scene()` which fails internally. Error modal shown in editor. Exit code 0 (editor lifecycle completed). |
| `scene.yaml --frame -1` (invalid frame) | `parse_running_args()` returns error → `BUDDD_LOG_ERROR("Error: ...")`, exit 1. Editor never opens. |
| `scene.yaml --capture` without argument | `parse_running_args()` returns error → `BUDDD_LOG_ERROR`, exit 1. |
| Path with spaces | Passed through argv as-is. `std::filesystem::is_regular_file()` handles spaces. Shell quoting is user's responsibility. |
| Relative path | Passed as-is to `is_regular_file()` and `Editor::open_scene()`. Works relative to CWD. |
| Absolute path | Same as relative — passed as-is. Works. |

## Security impact

- The scene file path is user-provided (from argv). No automatic file discovery or globbing.
- `std::filesystem::is_regular_file()` is used for pre-validation — the OS handles path traversal and permission checks natively.
- The editor's existing permissions model is unchanged. No elevated privileges required.
- No new input vectors beyond argv processing.

## Data and migration impact

None. No schema changes, no data migrations, no seed data changes. The scene YAML files are loaded by the existing `Editor::open_scene()` method.

## API compatibility impact

- `EditorApp(std::optional<std::string>)` constructor is added. The old `EditorApp()` default constructor is removed (replaced by the parameterised one with default `std::nullopt`). This is backward-compatible for callers who used `EditorApp()` — they can continue using `EditorApp{}` or `EditorApp()` without changes.
- `k_usage_text` in `help_command.h` format string change is a minor user-facing text change. No programmatic consumers parse this string.
- `parse_running_args()` API unchanged.
- `Editor::open_scene()` API unchanged — used as-is.

## Documentation impact

- **README**: None — the README doesn't list per-command details.
- **Wiki pages**: The following files must be updated by the wiki-agent to reflect the new `edit` behaviour:
  - `docs/wiki/architecture/data-flow.md` — CLI data flow diagram and output table: update `edit` dispatch description (no longer a simple `EditorApp → run_app()` — now includes optional scene path).
  - `docs/wiki/architecture/module-map.md` — Update the "Subcommand behavior" section (line 359) and "CLI integration" section (lines 441–445) to describe optional scene argument.
  - `docs/wiki/domain/business-rules.md` — Add entries for `buddd edit <path.yaml>`, `buddd edit <nonexistent>.yaml`, `buddd edit <unknown>` in the CLI output behavior table.
- **Other specs**: None.

## ADR impact

This implementation does not warrant a new ADR. It extends `EditorApp` with an optional constructor parameter and updates `main.cpp` dispatch — both backward-compatible changes within the existing ADR-014 (CLI App System) and ADR-027 (Editor Architecture) decisions.

## Done criteria

- [ ] `editor_app.h` includes `#include <optional>` at the top of the file (verified by reading `editor_app.h`).
- [ ] `EditorApp` constructor accepts `std::optional<std::string>` with default `std::nullopt` and stores it in a `std::optional<std::string> scene_path_` member (verified by reading `editor_app.h`).
- [ ] `EditorApp::setup()` logs `"Editor: opening scene: {}"` and calls `editor_->open_scene(*scene_path_)` when `scene_path_` has a value, without propagating `open_scene` failure as setup error (verified by reading `editor_app.cpp`).
- [ ] `main.cpp` `edit` dispatch branch has the 4-step logic: (1) no arg/empty → no scene, (2) YAML extension → `is_regular_file()` check, (3) starts with `-` → flags only, (4) otherwise → unknown arg error + exit 1 (verified by reading `main.cpp` lines 71–79 replacement).
- [ ] `flags_start=3` is used when scene path is present; `flags_start=2` otherwise (verified by reading `main.cpp`).
- [ ] `std::filesystem::is_regular_file()` is used for YAML file validation (not `exists()`) (verified by reading `main.cpp`).
- [ ] `k_usage_text` in `help_command.h` shows `"  edit      Open the editor (optionally with a scene file)\n"` (verified by reading `help_command.h`).
- [ ] Test T1 passes: `run_buddd("edit --frame 2 --log-level=info")` exits 0 and produces editor-related output on stderr (or expected display-missing error in headless — the exact assertion depends on headless vs. display build; see clarification note below).
- [ ] Test T2 passes: `run_buddd("edit nonexistent.yaml")` exits 1 with `"Scene file not found"` on stderr.
- [ ] Test T3 passes: `run_buddd("edit somearg")` exits 1 with `"Unknown argument for edit"` on stderr.
- [ ] Test T5 passes: `run_buddd("help")` stdout contains `"optionally with a scene file"`.
- [ ] Test T6 passes: `run_buddd("edit .yaml")` exits 1 (unless `run_buddd` is executed in a directory with a file named `.yaml`).
- [ ] Test T7 passes: temp valid `.yaml` file, `run_buddd("edit <temp.yaml> --frame 2")` does NOT produce `"Scene file not found"` (confirms dispatch accepted the file). Expected exit code: 0 for `BUDDD_HAS_DISPLAY=ON` (default), 1 for `=OFF` — the key assertion is absence of the "not found" error.
- [ ] Test T8 passes: temp valid `.YML` file, `run_buddd("edit <temp.yml>")` does NOT produce `"Scene file not found"` (confirms case-insensitive matching). Expected exit code depends on display build, same as T7.
- [ ] All existing tests in `tests/cmd/cli_app_tests.cpp` continue to pass unchanged.
- [ ] Full test suite builds and passes with no regressions.
- [ ] No source files outside `Files allowed to change` list are modified.
- [ ] No changes to `Editor::open_scene()` or any code in `src/editor/`.
- [ ] No changes to `App` base class (`src/cmd/app.h` / `src/cmd/app.cpp`).
- [ ] No new dependencies introduced.
- [ ] All `[NEEDS CLARIFICATION]` markers resolved (none exist in this contract).

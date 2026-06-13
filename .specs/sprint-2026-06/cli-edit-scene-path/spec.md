# SPEC-035 — CLI `buddd edit [<scene>]` — open editor with scene path

## Problem

The `buddd edit` command launches the editor with an empty workspace. There is no way to open a scene YAML file directly from the command line — users must launch the editor and then use File > Open Scene (or `Ctrl+O`) to load a file. This slows down iteration for developers and content creators who work with specific scene files repeatedly. The `run` command already supports YAML auto-detection (`buddd run assets/scenes/demo.yaml`), but `edit` does not.

## Goals

| ID | Goal |
|---|---|
| G-01 | `buddd edit assets/scenes/demo.yaml` opens the editor with `demo.yaml` loaded into the Editor's World at startup. |
| G-02 | `buddd edit` (no argument) continues to open the editor with an empty, untitled scene ("Untitled — Buddd Editor"). |
| G-03 | YAML auto-detection matches the `run` command pattern: case-insensitive `.yaml` or `.yml` extension. |
| G-04 | Non-existent YAML file produces an error message on stderr and exit code 1, without opening the editor. |
| G-05 | Flags `--frame` and `--capture` continue to work when passed after `edit` (parsed via `parse_running_args()`). |
| G-06 | `k_usage_text` in `help_command.h` is updated to document `edit [<scene>]`. |
| G-07 | Integration tests verify all combinations: scene loaded, no arg, non-existent file, with flags. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | No changes to `run` command behavior, including its YAML path handling. |
| NG-02 | No new CLI flags for the editor (existing `--frame`/`--capture` are reused as-is). |
| NG-03 | No changes to `Editor::open_scene()`, `Editor::setup()`, or any existing `Editor` APIs. |
| NG-04 | No multi-scene support (single scene open at a time, matching existing editor constraint). |
| NG-05 | No changes to the save system, dirty state tracking, or file dialog behaviour. |
| NG-06 | No changes to the `App` base class or `run_app()` lifecycle. |
| NG-07 | No changes to `SceneApp` or any other `App` subclass. |

## Actors

| Actor | Description |
|---|---|
| **Developer** | A human running `buddd edit [<scene>]` from a terminal to quickly open a scene file in the editor. |
| **Content creator** | A non-developer user who saves scene files and wants to open them directly from the command line or a script. |
| **CLI maintainer** | A developer maintaining `main.cpp` dispatch, help text, and test coverage. |

## User-visible behavior

### `buddd edit` (no argument)

Unchanged current behaviour:
- Opens the editor with 1280×800 window, title "Untitled — Buddd Editor".
- Empty untitled scene (clean, no entities).
- Flags `--frame` and `--capture` parsed from `argv[2..]`.

### `buddd edit <scene>`

Where `<scene>` is a file path ending in `.yaml` or `.yml` (case-insensitive):

1. **Before creating the editor**: validate the file is a regular file using `std::filesystem::is_regular_file()`. If the path does not exist, is a directory, a symlink to a non-file, or any other non-regular-file entity, print an error to stderr and exit with code 1 — the editor window is never opened. This pre-check catches non-existent files, directories, symlinks to non-files, etc., all with the same error+exit behaviour.
2. If the file exists, the editor opens normally with the scene loaded.
   - The window title shows the loaded file name, e.g., `"demo.yaml — Buddd Editor"`.
   - The Editor's own World is populated with entities from the YAML file via `Editor::open_scene()`.
   - The scene is initially **clean** (dirty flag cleared), exactly as if the user had opened it via File > Open Scene.
3. Flags `--frame` and `--capture` are parsed from `argv[3..]` (after the scene path) via `parse_running_args()`.
   - `--frame N` limits the editor to N frames (useful in headless/CI mode).
   - `--capture N:path` captures frame N and saves to path.

### YAML auto-detection logic

Reuses the same approach as the `run` command (`main.cpp`, lines 99–108). The detection checks:

```
1. Find the last '.' in the argument
2. If no '.' or '.' is last character → not a YAML path
3. Extract the extension, case-fold to lowercase
4. If extension is "yaml" or "yml" → treat as scene file path
```

A path that matches the YAML extension pattern but is not a regular file (non‑existent, directory, symlink to non‑file, etc.) produces an error and exit code 1 (never opens the editor window).

### Help text update

The `k_usage_text` constant in `help_command.h` currently shows:

```
  edit      Open the editor [--frame N] [--capture N:path]
```

Updated to:

```
  edit      Open the editor (optionally with a scene file)
```

The `--frame`/`--capture` flags line remains unchanged:

```
Flags for run/edit: --frame N, --capture N:path
```

Full updated `k_usage_text`:

```
Usage: buddd <command> [<args>]

Commands:
  run       Run a scene or the interactive window (default)
  edit      Open the editor (optionally with a scene file)
  version   Print version information
  help      Show this help message

Flags for run/edit: --frame N, --capture N:path
For scene usage: buddd run --help
```

### Relationship to existing `Editor::open_scene()`

The `Editor::open_scene(path)` method already exists and is used by File > Open Scene in the editor's menu bar. The CLI `edit <scene>` path calls the same method, ensuring consistent behaviour:
- On success: editor World populated, `current_file_path_` set, dirty flag cleared, window title updated.
- On failure (corrupt YAML, permissions, etc.): error modal shown in the editor, previous (empty) scene preserved.

The key difference from File > Open Scene is that the CLI path loads the scene **during startup** (in `EditorApp::setup()` or immediately after), not in response to a user menu action. Error handling for corrupt YAML files defers to the editor's existing error modal — the editor window opens in all cases, displaying either the loaded scene or an error modal for load failures.

### Dispatch logic for `edit` subcommand

The `main.cpp` dispatch for `edit` follows this order of operations:

1. **No scene argument**: If `argc < 3` or `argv[2]` is an empty string, proceed with no scene. The editor opens empty (current default behaviour).
2. **YAML extension match**: If `argv[2]` ends with `.yaml` or `.yml` (case‑insensitive, matching the `run` command pattern):
   - Run `std::filesystem::is_regular_file(argv[2])`.
   - If not a regular file (non‑existent, directory, symlink to non‑file, etc.): print error to stderr, exit code 1, no editor window.
   - If it is a regular file: pass the path to `EditorApp`, which calls `Editor::open_scene()` during setup.
3. **Flag argument**: If `argv[2][0] == '-'` (the argument starts with a dash), treat it as a flag. The scene parameter is absent — proceed with no scene; `parse_running_args()` will parse the flags from `argv[2..]`.
4. **Unknown positional argument**: Otherwise, `argv[2]` is neither a YAML path nor a flag. Print an error to stderr and exit with code 1. No editor window opens.

This dispatch ensures that:
- `buddd edit` → step 1 → empty editor.
- `buddd edit ''` → step 1 (empty string check) → empty editor.
- `buddd edit scene.yaml` → step 2 → scene loaded.
- `buddd edit nonexistent.yaml` → step 2 → error + exit 1.
- `buddd edit --capture 2:path` → step 3 (starts with `-`) → flags only, empty editor.
- `buddd edit somearg` → step 4 → unknown arg error + exit 1.

### Existing documentation that must be updated

| Document | Section / location | Change required |
|---|---|---|
| `docs/wiki/architecture/data-flow.md` | CLI data flow diagram, output table lines 52–61 | Update `buddd edit` dispatch description to include optional scene path. |
| `docs/wiki/architecture/module-map.md` | Subcommand behavior (line 359), CLI integration (lines 441–445) | Document that `edit` now accepts an optional scene argument. |
| `docs/wiki/domain/business-rules.md` | CLI output behavior table | Add entry or update row for `edit` with/without scene path. |

## User stories

### Story 1 — Open editor with scene file (Priority: P1)

As a developer, I want to run `buddd edit assets/scenes/demo.yaml` and see the editor immediately with that scene loaded, so that I can start editing without navigating the File menu.

**Given** the file `assets/scenes/demo.yaml` exists and is a valid YAML scene
**When** I run `buddd edit assets/scenes/demo.yaml`
**Then** the editor opens with the window title showing `"demo.yaml — Buddd Editor"`
**And** the editor's entity tree contains the entities from the scene file
**And** the scene is clean (no dirty indicator)

### Story 2 — Open editor with flags (Priority: P1)

As a developer, I want to run `buddd edit assets/scenes/demo.yaml --frame 2` to open the editor with a scene and run it for exactly 2 frames (e.g., in a CI test).

**Given** the file `assets/scenes/demo.yaml` exists
**When** I run `buddd edit assets/scenes/demo.yaml --frame 2 --log-level=info`
**Then** the editor opens, loads the scene, runs for 2 frames, and exits with code 0

### Story 3 — Edit with no argument (Priority: P1)

As a developer, I want to run `buddd edit` and get the existing empty-editor behaviour unchanged.

**Given** the `buddd` binary is compiled with display support
**When** I run `buddd edit`
**Then** the editor opens with title "Untitled — Buddd Editor"
**And** the editor shows an empty scene with no entities

### Story 4 — Error on non-existent file (Priority: P1)

As a developer, I want a clear error message when I mistype a scene path, so that I know the file was not found.

**Given** the file `nonexistent.yaml` does not exist
**When** I run `buddd edit nonexistent.yaml`
**Then** the editor window is never opened
**And** stderr contains an error message indicating the file was not found
**And** the process exits with code 1

### Story 5 — Capture from editor (Priority: P2)

As a developer, I want to capture a frame from the editor with a scene loaded, so that I can verify scene rendering without manual interaction.

**Given** the file `assets/scenes/demo.yaml` exists
**When** I run `buddd edit assets/scenes/demo.yaml --capture 2:/tmp/cap.png --log-level=info`
**Then** the editor opens, loads the scene, runs for at least 2 frames, saves frame 2 to `/tmp/cap.png`, and exits with code 0
**And** stdout contains `"Captured: /tmp/cap.png"`

### Story 6 — Help text mentions scene argument (Priority: P2)

As a developer, I want the `buddd help` output to show that `edit` accepts an optional scene file path.

**Given** the `buddd` binary is compiled
**When** I run `buddd help`
**Then** the command list shows `edit` with a description indicating it optionally accepts a scene file

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `buddd edit` with no argument opens editor with empty scene (existing behaviour unchanged). | Run `buddd edit` (headless with timeout 2s via `SDL_VIDEO_DRIVER=offscreen`); verify stderr contains `"Window opened:"`, exit code 0. |
| AC-002 | `buddd edit assets/scenes/demo.yaml` (valid file) opens editor with scene loaded and window title reflects loaded file. | Run `buddd edit assets/scenes/demo.yaml --frame 2 --log-level=info`; verify exit code 0. Verify log output contains `"demo.yaml — Buddd Editor"` or the window-title log line produced by `Editor::open_scene()`. |
| AC-003 | `buddd edit nonexistent.yaml` prints error and exits 1 without opening window. | Run `buddd edit nonexistent.yaml`; verify exit code 1, stderr contains error message, stdout does not contain `"Window opened"`. |
| AC-004 | YAML auto-detection matches `.yaml` and `.yml` case-insensitively. | Run `buddd edit assets/scenes/demo.YAML --frame 2` (if file exists under different case) or equivalent test; verify exit code 0. For `.yml`: create temp `.yml` scene, run `buddd edit <path> --frame 2`; verify exit code 0. |
| AC-005 | Non-YAML, non-flag positional argument (e.g., `buddd edit somearg`) produces an "unknown argument" error and exits with code 1. The dispatch distinguishes this from flags (which start with `-`) and YAML paths (which end with `.yaml`/`.yml`). | Run `buddd edit somearg`; verify exit code 1, stderr contains error message. Run `buddd edit --frame 2`; verify exit code 0 (not mistaken for unknown arg). |
| AC-006 | `buddd edit assets/scenes/demo.yaml --frame 2` runs for exactly 2 frames. | Run with `--frame 2` and `--log-level=info`; verify stderr does NOT contain "frame=2" (if such logging exists) or just verify exit 0. Log verification: stderr should show editor lifecycle with no errors. |
| AC-007 | `buddd edit assets/scenes/demo.yaml --capture 2:/tmp/cap.png` captures frame 2. | Run with `--capture 2:/tmp/cap.png --log-level=info` (and sufficient frame count); verify `/tmp/cap.png` exists and is a valid PNG, stdout contains `"Captured: /tmp/cap.png"`. |
| AC-008 | `k_usage_text` in `help_command.h` documents `edit` with optional scene argument. | Run `buddd help`; verify stdout contains `"edit"` with description mentioning scene file. |
| AC-009 | A corrupt YAML file opens the editor (window appears) but shows an error load modal. | Create a temp invalid YAML file, run `buddd edit <path> --frame 2`; verify exit code 0 (editor runs), and stderr or window logs show load error. |
| AC-010 | `edit` with `--capture` but no scene path (no YAML arg) still works. | Run `buddd edit --capture 2:/tmp/cap_empty.png` (with `--frame 2`); verify exit code 0 and capture file created. Note: the YAML check fails on `--capture` (not a YAML path), so `edit` proceeds with no scene. |

## E2E Verification

| Method | Description |
|---|---|
| **Automated integration tests** | All AC items verified via `run_buddd()` integration tests in `tests/cmd/cli_app_tests.cpp` (tagged `[cli][app]`). Tests run headless with `SDL_VIDEO_DRIVER=offscreen` and `--frame` to limit execution time. |
| **Manual smoke test (display)** | Launch `buddd edit assets/scenes/demo.yaml` with real display; verify window opens, scene entities visible in tree, title correct. Launch `buddd edit nonexistent.yaml`; verify error on stderr and no window. |
| **Capture file verification** | Generated PNG files inspected programmatically (file size > 0, valid PNG header). |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-01 | A developer can open any valid YAML scene file in the editor with a single CLI command. | All AC-002, AC-004, AC-006 tests pass. |
| SC-02 | Non-existent file path produces immediate error without opening any window. | AC-003 passes. |
| SC-03 | All existing editor and CLI tests continue to pass unchanged. | Run full test suite; no regressions. |
| SC-04 | `buddd edit` with no argument behaves identically to pre-feature behaviour. | AC-001 passes; compare stderr/stdout with pre-change baseline. |

## Edge cases

| Case | Expected behavior |
|---|---|
| `buddd edit ''` (empty string as scene path) | Empty string is treated identically to "no argument" — the dispatch logic checks `argv[2]` for empty string (or `argc < 3`), and if empty, proceeds with no scene. The editor opens with an empty untitled scene. No error. |
| `buddd edit .yaml` (just an extension, no filename) | `std::filesystem::is_regular_file(".yaml")` is checked; unless a file literally named `.yaml` exists as a regular file, error + exit 1. |
| `buddd edit path/to/scene.yaml` with relative path | Path passed as-is to `std::filesystem::is_regular_file()` and `Editor::open_scene()`. Works relatively to CWD. |
| `buddd edit /absolute/path/scene.yaml` | Absolute path handled identically to relative — checked via `is_regular_file()`, passed to `Editor::open_scene()`. |
| `buddd edit scene.yaml --frame 0` | `--frame 0` means no limit (interactive mode). Editor runs until window close. In headless CI, a timeout (e.g., `timeout 2`) is needed. |
| `buddd edit scene.yaml --frame -1` | `parse_running_args()` returns an error (invalid frame); error printed to stderr, exit code 1. |
| `buddd edit scene.yaml --capture` without arguments | `parse_running_args()` returns an error; error printed to stderr, exit code 1. |
| `buddd edit scene.yaml extra_arg` | Extra positional argument after the scene path. The `parse_running_args()` from `flags_start=3` may warn about unexpected arguments but will proceed. |
| `buddd edit --frame 2 scene.yaml` (flag before scene path) | `edit` handler is called with argv starting at `edit`. The current dispatch does not reorder args. `parse_running_args(argc, argv, 2)` parses from argv[2] onward. If argv[2] is `--frame`, that's parsed as a flag, not a scene path. The scene path check would see argv[3] (`scene.yaml`) but that's consumed as flag value. `parse_running_args` handles `--frame N` by consuming the next argument. After parsing, no scene path would be identified. The editor opens empty. Currently the `run` command does NOT support flags before the scene name either — this is consistent. |
| `buddd edit scene.yaml --frame 2 --capture 2:/tmp/cap.png` | All flags parsed correctly after scene path. Flags start at index 3. |
| Path with spaces or special characters | Passed as-is through argv. `std::filesystem::is_regular_file()` handles spaces in paths. Shell quoting is the user's responsibility. |
| Path is a directory (not a file) | The pre‑open check uses `std::filesystem::is_regular_file()`, which returns `false` for directories. The path is rejected before the editor opens: error printed to stderr, exit code 1, no editor window. |

## Error cases

| Case | Expected behavior |
|---|---|
| Scene file does not exist | stderr: error message (e.g., `"Scene file not found: 'nonexistent.yaml'"`). Exit code 1. Editor window is never opened. |
| Scene file is a directory | `is_regular_file()` returns false. Pre‑opening check catches this: error to stderr, exit code 1, editor never opens. |
| Corrupt YAML file | File exists. Editor opens. `Editor::open_scene()` fails with a parse error. Error modal shown in editor. Editor remains open with empty scene. Previous (empty) scene preserved. Exit code 0 (editor ran successfully, just the load failed). |
| `--frame` parse failure | `parse_running_args()` returns error. Error printed to stderr. Exit code 1. Editor never opens. |
| `--capture` parse failure | Same as `--frame` failure. Error to stderr. Exit code 1. Editor never opens. |
| `Image::save()` fails for capture | Error printed to stderr for failed capture. Other captures proceed. Exit code depends on whether any captures succeeded. Editor lifecycle unaffected. |

## Permissions and security

- The CLI binary requires no elevated privileges to run.
- File existence check uses `std::filesystem::is_regular_file()` — no special permissions needed beyond read access to the scene file.
- The scene file path is user-provided (from argv). No automatic file discovery or globbing.
- The editor's existing permissions model is unchanged (see SPEC-F-01 Permissions and security).
- Path traversal: user provides the path directly; no sanitization needed beyond what `std::filesystem::is_regular_file()` provides (the OS handles path validation).

## Observability

| Signal | Source |
|---|---|
| `BUDDD_LOG_ERROR("Scene file not found: '{}'", path)` | When YAML path does not exist (before editor opens). |
| `BUDDD_LOG_INFO("Editor: opening scene: {}", path)` | When `editor.open_scene()` is called during EditorApp::setup() (new log line, added as part of this feature). |
| `BUDDD_LOG_WARN("Scene load failed: {}", error.message())` | When `Editor::open_scene()` returns an error (existing log in `editor.cpp`). |
| `"Window opened: 1280x800"` | stdout — existing `run_app()` log. |
| Exit code | `$?` after process exits. |

## Out of scope

- Changes to `run` command behaviour (including YAML path handling for `run`).
- New CLI flags for the editor.
- Changes to `Editor::open_scene()`, `Editor::setup()`, or any existing `Editor` APIs.
- Multi-scene support.
- Changes to save system, dirty state, or file dialogs.
- Changes to the `App` base class or `run_app()` lifecycle.
- Changes to `SceneApp` or any other `App` subclass.
- Globbing, autocomplete, or shell integration.
- Relative- vs absolute-path normalisation (pass through as-is from argv).
- Support for non-YAML scene files (future binary format).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `Editor::open_scene(path)` exists, works as documented, and can be called from `EditorApp::setup()` after `Editor::setup(ctx)`. |
| A-02 | `Editor::open_scene()` is idempotent and safe to call during editor startup (before any user interaction). |
| A-03 | The `App` lifecycle (`setup()`, `update()`, `on_render()`, `shutdown()`) is unchanged and `EditorApp` can store a path string passed in its constructor. |
| A-04 | `parse_running_args()` with `start=2` works correctly when a scene path is at argv[2] and flags are at argv[3..]. With `start=2`, `parse_running_args` parses from argv[2] — if argv[2] is a scene path and not a flag, it will be silently skipped (unknown flags are silently ignored per `app_config.h`). This is the same pattern used for `run` with no scene. |
| A-05 | The `is_yaml_file` lambda from `main.cpp` lines 99–108 can be reused or replicated. If reused, it should be extracted to a shared location (or duplicated — duplication is acceptable for a small lambda). |
| A-06 | `std::filesystem::is_regular_file()` correctly returns `false` for directories, symlinks to non‑files, and non‑existent paths. |
| A-07 | The `--frame`/`--capture` flags are already tested to work with the editor via `run_app()`. Since `EditorApp` is an `App` subclass, `run_app()` handles all flag logic identically. |
| A-08 | The test helper `run_buddd()` in `tests/test_helpers.h` can be used for all integration tests, including those with `edit` commands. |
| A-09 | Test scene files (e.g., `assets/scenes/demo.yaml`) exist in the repository. If not, tests should create temporary YAML scene files. |
| A-10 | The existing help text line `"Flags for run/edit: --frame N, --capture N:path"` already mentions `edit`, confirming that `edit` already supports these flags. Only the `edit` command description line needs updating. |

## Open questions

No `[NEEDS CLARIFICATION]` markers remain. All questions have been resolved through the provided architecture context and existing code analysis.

| ID | Question | Resolution |
|---|---|---|
| Q-01 | Should `EditorApp` store the optional scene path in its constructor or should `main.cpp` call `open_scene` on the editor pointer? | **EditorApp stores the path** in its constructor. The path is used in `setup()` after `Editor::setup(ctx)` succeeds. This keeps main.cpp simple and follows the existing pattern (EditorApp owns the Editor instance and coordinates its lifecycle). |
| Q-02 | Should corrupt YAML produce exit code 1 (load failure) or exit code 0 (editor ran normally, just load failed)? | **Exit code 0**. The editor window opened and ran its lifecycle normally. The load failure is an error modal within the editor, not a CLI error. This matches `run` command behaviour for YAML files: if the file exists, `run` opens it and if loading fails during `SceneApp::setup()`, the error propagates to exit code 1. However, `EditorApp` is different — the editor is designed to stay open on load failure and show an error modal. So exit 0 is correct. |
| Q-03 | How does `parse_running_args()` handle the scene path at argv[2] when flags_start=2? | Unknown flags are silently ignored by `parse_running_args()`. The scene path at argv[2] (e.g., `demo.yaml`) is not a flag (doesn't start with `-`), so it is silently skipped. This is acceptable — the scene path is consumed by the dispatch logic before `parse_running_args` is called. |
| Q-04 | Should `edit` support `--frame`/`--capture` flags before the scene path (e.g., `edit --frame 2 scene.yaml`)? | **No**. This would require reordering argv or multi-pass parsing, adding complexity. The `run` command has the same limitation (flags must come after the scene name). This is documented as an edge case and users are expected to put flags after the path. |

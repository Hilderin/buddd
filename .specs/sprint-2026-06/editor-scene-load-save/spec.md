# SPEC-F-01 — Editor Scene Load/Save Integration

## Problem

The Buddd Editor currently has no way to persist or load scene files. Users launch the editor, see an empty workspace, and cannot save their work. The engine already provides `SceneLoader` and `SceneSaver` APIs for YAML-based scene serialization, and the Editor owns a `World` via `editor.world()`, but there is no UI wiring: no File menu items for New/Open/Save/Save As, no OS file dialogs, no dirty-state tracking, and no save-prompt modals to prevent accidental data loss. Every unsaved change is lost when the editor closes.

This feature wires the existing engine APIs into the Editor's File menu, establishing the fundamental save/open workflow that all subsequent editor features depend on.

## Goals

| ID | Goal |
|---|---|
| G-01 | **New Scene**: File > New Scene clears the Editor's World (with save prompt if dirty), resets to an untitled scene. |
| G-02 | **Open Scene**: File > Open Scene opens an OS file dialog (`.yaml` filter), loads the selected file via `SceneLoader::load_from_file()` into the Editor's World. Prompts save if current scene is dirty. |
| G-03 | **Save Scene**: File > Save Scene saves via `SceneSaver::save_to_file()` to the current file path. If untitled (no path), triggers Save As. Clears dirty indicator. |
| G-04 | **Save Scene As**: File > Save Scene As opens an OS file dialog, saves to the chosen path, updates the current file path. |
| G-05 | **Quit with dirty check**: File > Quit or OS window close button (X / Alt+F4) prompts save if the scene is dirty before exiting. Cancel aborts the exit. |
| G-06 | **Dirty state tracking**: Simple boolean `dirty_` on the `Editor` class. Set on any entity/component mutation. Cleared on Save. `*` shown in window title bar when dirty. |
| G-07 | **Save-prompt modal**: Modal dialog with Save / Don't Save / Cancel buttons when attempting to close, open, or create a new scene with unsaved changes. Cancel aborts the operation. |
| G-08 | **Error handling**: SceneLoader/SceneSaver failures (corrupt YAML, file not found, permissions) shown in an ImGui modal error dialog. Current scene preserved on load failure. |
| G-09 | **OS file dialogs**: ImGuiFileDialog library integrated via FetchContent, with `.yaml` file filter. |
| G-10 | **Menu integration**: Extended File menu with New Scene (`Ctrl+N`), Open Scene (`Ctrl+O`), Save Scene (`Ctrl+S`), Save Scene As (`Ctrl+Shift+S`), separator, Quit (`Ctrl+Q`). |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | No prefab tab saving (future F-17). |
| NG-02 | No Play mode interaction (future F-14). |
| NG-03 | No entity-level dirty tracking — simple scene-level boolean only. |
| NG-04 | No "Revert" or "Reload" operations. |
| NG-05 | No recent files list. |
| NG-06 | No Project panel double-click integration (future F-11). |
| NG-07 | No autosave. |
| NG-08 | No multi-tab dirty tracking — single Scene tab for MVP. |
| NG-09 | No changes to `SceneLoader` or `SceneSaver` engine APIs — they are used as-is. |
| NG-10 | No changes to `World`, `Entity`, `ComponentRegistry`, or `AssetManager` engine types. |
| NG-11 | No cross-session file path persistence (file path is runtime-only, reset on editor launch). |

## Actors

| Actor | Description |
|---|---|
| **User (content creator)** | Opens the editor, creates/modifies scenes, uses File menu to save and load. |
| **Editor** | Owns the World, dirty state, file path, and coordinates scene operations. Wraps SceneLoader/SceneSaver instantiation per-call. |
| **MenuBar** | Renders the File menu with New/Open/Save/Save As/Quit items. Dispatches to Editor via callbacks. |
| **SceneLoader** | Engine API (`SceneLoader::load_from_file`) that parses a YAML scene file and populates a World. |
| **SceneSaver** | Engine API (`SceneSaver::save_to_file`) that serializes a World to a YAML file. |
| **ImGuiFileDialog** | Third-party library providing OS-native file dialogs with `.yaml` filter. |
| **EngineContext** | Provides access to `EngineService` (for `registry()` and `assets()`), `Window` (for title), and `request_exit()`. |

## User-visible behavior

### File Menu Items

| Item | Shortcut | Behavior |
|---|---|---|
| **New Scene** | `Ctrl+N` | If dirty, show save-prompt modal. Clear World, reset file path to empty, set dirty=false, update window title to "Untitled — Buddd Editor". |
| **Open Scene** | `Ctrl+O` | If dirty, show save-prompt modal. Open OS file dialog (`.yaml` filter). On selection, load via `SceneLoader`. On failure, show error modal (previous scene preserved). |
| **Save Scene** | `Ctrl+S` | If untitled (no file path), behave as Save As. Otherwise save via `SceneSaver` to current path. Clear dirty flag. Update window title. On failure, show error modal. |
| **Save Scene As** | `Ctrl+Shift+S` | Open OS file dialog (`.yaml` filter). Save to chosen path. Update current file path. Clear dirty flag. Update window title. |
| **Quit** | `Ctrl+Q` | If dirty, show save-prompt modal. On Save/Don't Save, call `ctx.request_exit()`. On Cancel, abort. |

### Dirty State

- Simple boolean `dirty_` on the `Editor` class.
- Set to `true` when any entity or component mutation occurs (via `Editor::mark_dirty()`). This is a manual call — panels and commands call `mark_dirty()` when they modify the scene.
- Cleared to `false` after a successful Save or Save As.
- The editor window title includes `*` suffix when dirty (e.g., `"scene_01.yaml*"` or `"Untitled*"`).
- When `dirty_` is `false`, no `*` is shown.

### Window Title

Format: `"<file_name><dirty_suffix> — Buddd Editor"`

| Scenario | Title |
|---|---|
| Editor launch (untitled, clean) | `"Untitled — Buddd Editor"` |
| Untitled, modified | `"Untitled* — Buddd Editor"` |
| Scene loaded, clean | `"scene_01.yaml — Buddd Editor"` |
| Scene modified | `"scene_01.yaml* — Buddd Editor"` |

### Untitled Scenes

- A newly created scene (via New Scene or editor launch) is **"Untitled"** with no file path.
- The initial dirty state for a new untitled scene is `false` (clean). The scene becomes dirty on first modification.
- Save on an untitled scene triggers the Save As dialog automatically (no direct save possible).
- After Save As completes successfully, the file path is set and the dirty flag is cleared.
- If `dirty_` is `true` and the scene is untitled, the window title shows `"Untitled* — Buddd Editor"`.

**State transitions:**
| Action | `dirty_` | `file_path_` | Title |
|---|---|---|---|
| Editor launch / New Scene | `false` | `""` | `"Untitled — Buddd Editor"` |
| User modifies scene | `true` | `""` | `"Untitled* — Buddd Editor"` |
| Save (triggered on untitled → Save As) | `false` | `"path/to/scene.yaml"` | `"scene.yaml — Buddd Editor"` |
| Modify again | `true` | `"path/to/scene.yaml"` | `"scene.yaml* — Buddd Editor"` |
| Save (direct) | `false` | `"path/to/scene.yaml"` | `"scene.yaml — Buddd Editor"` |

### Save-Prompt Modal

A modal dialog with the message "Save changes to [scene name]?" and three buttons:

| Button | Behavior |
|---|---|
| **Save** | Save the current scene (Save As if untitled), then proceed with the operation. |
| **Don't Save** | Discard changes, proceed with the operation. |
| **Cancel** | Abort the operation entirely. Current scene remains open and unchanged. |

The modal uses `ImGui::OpenPopup` + `ImGui::BeginPopupModal`.

### Error Modals

When a SceneLoader or SceneSaver operation fails, an error modal is shown:

- **Title**: "Error" (or "Load Error" / "Save Error")
- **Message**: The error description from the `Result` error.
- **Button**: "OK" to dismiss.
- **Effect on scene**: On load failure, the previous scene is preserved unchanged. On save failure, the scene remains dirty.

Error modals follow the same `ImGui::OpenPopup` + `BeginPopupModal` pattern as the About popup.

### OS File Dialog (ImGuiFileDialog)

- Triggered by Open Scene and Save Scene As.
- Opens an OS-native file dialog.
- Filter: `\.yaml` files (`.yaml`, `.yml`).
- On Open: the selected path is passed to `SceneLoader::load_from_file()`.
- On Save As: the selected path is passed to `SceneSaver::save_to_file()` and stored as the current file path.
- If the dialog is cancelled, no action is taken.

## User stories

### Story 1 — Save and load a scene (Priority: P1)

As a content creator, I want to save my scene to a file and load it back, so that my work persists between sessions.

**Given** the Editor has a scene with entities
**When** I select File > Save Scene As, choose a path, and save
**Then** the scene is written to the chosen `.yaml` file
**And** the dirty indicator clears
**And** the window title updates to show the file name

**Given** the Editor is running with an empty scene
**When** I select File > Open Scene, choose a previously saved `.yaml` file
**Then** the scene loads into the Editor's World
**And** the hierarchy populates with the saved entities
**And** the window title shows the file name

### Story 2 — Dirty state tracking (Priority: P1)

As a content creator, I want to see when my scene has unsaved changes, so that I know to save before closing.

**Given** the Editor has a clean scene
**When** a modification occurs (entity added, component changed, etc.)
**Then** the window title shows `*` to indicate dirty state
**And** `*` is removed after a successful Save

### Story 3 — Save prompt on close with dirty scene (Priority: P1)

As a content creator, I want to be prompted to save when quitting with unsaved changes, so that I don't lose my work.

**Given** the Editor has a dirty scene
**When** I select File > Quit
**Then** a save-prompt modal appears with Save, Don't Save, and Cancel buttons
**When** I click Save
**Then** the scene is saved and the editor exits
**When** I click Don't Save
**Then** the editor exits without saving
**When** I click Cancel
**Then** the quit operation is aborted and the editor remains open

### Story 4 — Save prompt on Open/New with dirty scene (Priority: P1)

As a content creator, I want to be prompted to save before opening a different scene or creating a new one, so that I don't accidentally lose changes.

**Given** the Editor has a dirty scene
**When** I select File > New Scene
**Then** a save-prompt modal appears (Save / Don't Save / Cancel)
**When** I select Save
**Then** the current scene is saved, then a new untitled scene is created
**When** I select Cancel
**Then** the operation is aborted, current scene remains

**Given** the Editor has a dirty scene
**When** I select File > Open Scene
**Then** a save-prompt modal appears (Save / Don't Save / Cancel)
**When** I select Don't Save
**Then** the current scene is discarded and the file dialog opens

### Story 5 — Error handling on load/save failure (Priority: P2)

As a content creator, I want to see an error message when a file operation fails, so that I understand what went wrong.

**Given** the Editor is open
**When** I try to open a corrupt `.yaml` file
**Then** an error modal appears with the error message
**And** the current scene remains unchanged
**When** I dismiss the error modal
**Then** the editor continues with the previous scene

**Given** the Editor is open
**When** a save operation fails (e.g., permissions, disk full)
**Then** an error modal appears with the error message
**And** the scene remains dirty

### Story 6 — Round-trip save and reload (Priority: P2)

As a developer, I want to verify that a scene saved and then reloaded produces the same entity tree.

**Given** a scene with entities, components, and transforms
**When** I save it to a `.yaml` file
**And** load that file into a fresh Editor World
**Then** the new World has the same entity count, names, transforms, and components as the original

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-01 | File > Open Scene: OS file dialog filters `.yaml`, scene loads into Editor's World, tree populated. | Manual: launch editor, open a `.yaml` file, verify entities appear. |
| AC-02 | File > Save Scene (with path): scene saved to same path, no dialog shown. | Manual: open a scene, modify, save, verify file is written. |
| AC-03 | File > Save Scene As: OS dialog opens, save to chosen path, subsequent Save uses new path. | Manual: save as to new path, modify again, save (no dialog), verify second save writes to new path. |
| AC-04 | Dirty state: modify scene → `*` in window title, Save clears it. | Unit test: construct Editor, mark dirty, verify title contains `*`, save, verify `*` removed. |
| AC-05 | Dirty prompt on Open: modified scene → Open → Save/Don't Save/Cancel dialog, Cancel aborts, Save saves and proceeds. | Unit test: simulate dirty state, trigger open, verify callback behavior for each button. |
| AC-06 | Untitled scene: no file loaded → title shows "Untitled", Save triggers Save As (no-op redirect). | Unit test: construct Editor, verify `file_path()` is empty and title is "Untitled", call save and verify callback to Save As. |
| AC-07 | Quit with dirty: modified scene → Quit → Save/Don't Save/Cancel dialog. | Unit test: simulate dirty state, trigger quit, verify save prompt appears, verify Cancel aborts exit. |
| AC-08 | Error handling: load invalid YAML → error dialog, previous scene untouched. | Unit test: attempt `load_from_file` with corrupt YAML, verify error returned and World unchanged. |
| AC-09 | File > New Scene: if dirty, prompt → clear World → scene is untitled (empty, no file path). | Unit test: load a scene, mark dirty, trigger New, verify World cleared and file path empty. |
| AC-10 | Round-trip: load `.yaml`, save it (to temp), reload → identical entity tree (count, names, transforms, component values). | Integration test: load a known test `.yaml`, save to temp file, load into fresh World, compare entity structure. |

## E2E Verification

| Method | Description |
|---|---|
| **Manual smoke test (display)** | Launch `buddd edit`. Verify all File menu items are present with correct shortcuts. Create entities (via future panels or programmatically), save to `.yaml`, close editor. Re-open editor, open the saved file, verify scene restores correctly. Test dirty indicator, save prompts, and error dialogs. |
| **Automated unit tests (headless)** | All AC items marked "Unit test" verified via Catch2 `[editor]`-tagged tests in `tests/editor_tests.cpp`. These tests construct an Editor headlessly, exercise scene management methods directly (not via UI), and verify state. Display-dependent tests (AC-01, AC-02, AC-03) are marked `[.display]`. |
| **Integration round-trip test** | AC-10: Use a known test `.yaml` file, load via Editor method, save to temp, reload into fresh Editor World, compare entity tree. Run in headless mode. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-01 | A user can create a scene, save it to a `.yaml` file, close the editor, re-open, and load the same scene — all without data loss. | Manual E2E smoke test passes (see above). |
| SC-02 | All AC unit tests (AC-04 through AC-10) pass in headless CI. | `ctest` passes all `[editor]`-tagged tests. |
| SC-03 | File menu displays all 6 items (New, Open, Save, Save As, separator, Quit) with correct keyboard shortcuts. | Visual inspection. |
| SC-04 | Error handling: corrupt YAML file does not crash the editor — shows error modal, preserves current scene. | Manual test and unit test (AC-08). |
| SC-05 | Save-prompt modal appears for all three operations (New, Open, Quit) when the scene is dirty, with Cancel correctly aborting. | Unit tests (AC-05, AC-07, AC-09). |

## Edge cases

| Case | Expected behavior |
|---|---|
| **Save on clean scene** | Save is a no-op (file already saved). No dialog. No error. `dirty_` remains false. |
| **Save As on a scene with an existing file path** | Opens dialog regardless of existing path. After saving, updates path to new selection. |
| **Open Scene with clean scene** | No save prompt. File dialog opens directly. |
| **New Scene with clean scene** | No save prompt. World cleared immediately. |
| **Quit with clean scene** | No save prompt. Exits immediately. |
| **OS window close button (X button / Alt+F4) with dirty scene** | Same save-prompt modal as File > Quit. Save/Don't Save/Cancel dialog. Cancel aborts close (window stays open). |
| **Cancel file dialog (Open)** | No action. Current scene unchanged. No error. |
| **Cancel file dialog (Save As)** | No action. Current file path unchanged. Dirty state unchanged. |
| **File dialog selects non-`.yaml` file** | ImGuiFileDialog filter prevents this; only `.yaml` files are selectable. |
| **Open a `.yaml` that is not a valid scene** | Error modal. Previous scene preserved. |
| **Save to a path where file already exists** | Silent overwrite (no additional editor confirmation — ImGuiFileDialog may show platform-dependent warning independently). |
| **Multiple rapid File menu actions** | Each action completes before next is processed (single-threaded ImGui frame). No race conditions. |
| **Editor launches with no saved layout** | Works normally. Scene is "Untitled", clean, empty. |
| **Scene file path contains spaces or special characters** | Stored as-is. Passed to SceneLoader/SceneSaver as-is. |
| **Dirty flag after failed Save** | Scene remains dirty. Error modal shown. User can retry Save or Save As. |
| **Dirty flag after failed Save As** | Scene remains dirty. File path is NOT updated. Error modal shown. |
| **Window title length** | Title is truncated by the OS/Window if too long. Acceptable. The dirty `*` is the last character before the title suffix, so it's always visible. |

## Error cases

| Case | Expected behavior |
|---|---|
| **SceneLoader::load_from_file returns error (corrupt YAML, missing file, etc.)** | `Editor::open_scene()` returns error. Error modal displayed via `ImGui::OpenPopup` + `BeginPopupModal`. Previous scene preserved unchanged. |
| **SceneSaver::save_to_file returns error (permissions, disk full, etc.)** | `Editor::save_scene()` returns error. Error modal displayed. Scene remains dirty. File path unchanged. |
| **File dialog fails to initialize** | Operation silently aborts. No error modal (ImGuiFileDialog init failure is outside this feature's scope — logged at debug level). |
| **Engine service (ComponentRegistry, AssetManager) not available during setup** | `Editor::setup()` already stores `engine_` pointer. If null when scene ops are called, the operation returns an error. This should not happen in normal execution (guarded by `initialized_` flag). |
| **World is empty during Save** | SceneSaver saves an empty scene (`type: Scene, version: 1, entities: []`). This is valid — saves successfully. |
| **World is empty during Open** | SceneLoader populates the empty World with loaded entities. |
| **ImGuiFileDialog file selection returns empty path** | No action taken. Treat as cancellation. |

## Permissions and security

- File operations use OS-level file access via SceneLoader/SceneSaver (no elevated privileges).
- ImGuiFileDialog uses the OS-native file dialog, which inherits the process's file permissions.
- No network access required.
- No credentials, secrets, or sensitive data are handled.
- File operations are triggered by explicit user action through the File menu — no automated file I/O.
- Scene files are assumed to be trusted (developer/artist-generated content).
- Path traversal protection is handled by the OS file dialog (user selects a file from the dialog; arbitrary path input is not accepted).

## Observability

| Signal | Source |
|---|---|
| `BUDDD_LOG_INFO("Scene saved: {}", path)` | After successful `SceneSaver::save_to_file()`. |
| `BUDDD_LOG_INFO("Scene loaded: {}", path)` | After successful `SceneLoader::load_from_file()`. |
| `BUDDD_LOG_WARN("Scene load failed: {}", error.message())` | When `SceneLoader::load_from_file()` returns an error. |
| `BUDDD_LOG_WARN("Scene save failed: {}", error.message())` | When `SceneSaver::save_to_file()` returns an error. |
| `BUDDD_LOG_DEBUG("Scene marked dirty")` | When `Editor::mark_dirty()` is called. |
| `BUDDD_LOG_DEBUG("Scene dirty cleared")` | After successful save. |
| `BUDDD_LOG_INFO("Save prompt: Save/Don't Save clicked")` | User action on save-prompt modal. |
| `BUDDD_LOG_INFO("Save prompt cancelled")` | User clicked Cancel on save-prompt modal. |
| Editor window title | Visible dirty state indicator. |

All logs use the `"Editor:Scene"` log channel tag (or `"Editor"` if no sub-channel — TBD during implementation).

## Out of scope

- Prefab tab saving (future F-17).
- Play mode interaction (future F-14).
- Entity-level dirty tracking.
- "Revert" or "Reload" operations.
- Recent files list.
- Project panel double-click integration (future F-11).
- Autosave.
- Multi-tab dirty tracking.
- Cross-session file path persistence.
- Changes to engine APIs (SceneLoader, SceneSaver, World, ComponentRegistry, AssetManager).
- YAML schema validation beyond what SceneLoader provides.
- Binary or non-YAML serialization formats.
- Progress bars for long load/save operations (scenes are small in V1).

## Documentation to update

The following documents must be updated to reflect this feature:

| File | Changes needed |
|---|---|
| `docs/wiki/editor/scene-management.md` | Update from "future vision" to "current implementation" for F-01 operations (New/Open/Save/Save As/Quit with dirty state). Document Editor's `dirty_` flag, `file_path_`, and scene management methods. **North-star section**: correct from dirty-by-default to clean-by-default. |
| `docs/wiki/architecture/module-map.md` | Add `ImGuiFileDialog` dependency to `buddd_editor` module. Add Editor scene management methods (`new_scene`, `open_scene`, `save_scene`, `save_scene_as`). |
| `docs/adr/ADR-029-editor-ux-decisions.md` | Update AC-015 reference to match clean-by-default decision (F-01 starts untitled clean, not dirty). |
| `.specs/sprint-2026-06/editor-ux-design/spec.md` | Add note that F-01 implements AC-015 through AC-018 and AC-025. **Note**: AC-015 and Story 1 need correction to match clean-by-default (F-01 starts untitled clean, not dirty). |

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `SceneLoader` and `SceneSaver` exist and work as specified in their respective specs. They are instantiated per-call (not stored as Editor members). |
| A-02 | `SceneLoader` constructor takes `(World&, ComponentRegistry&, AssetManager&)` — the Editor has access to all three via `EngineContext::services.registry()` and `EngineContext::services.assets()`. |
| A-03 | `SceneSaver` constructor takes `(World&, ComponentRegistry&, AssetManager&)` — same dependencies as SceneLoader. |
| A-04 | `ImGuiFileDialog` is added via FetchContent in the root `CMakeLists.txt` and linked to `buddd_editor`. The library exposes a C++ API compatible with the existing ImGui integration. |
| A-05 | ImGuiFileDialog provides `ImGuiFileDialog::Instance()->OpenDialog()`, `ImGuiFileDialog::Instance()->Display()`, and file selection query methods (`isOk()`, `GetFilePathName()`, `GetCurrentFilter()`). |
| A-06 | The `Editor` class stores `engine_` (an `EngineService*`) after `setup()` — this provides access to `registry()` and `assets()` needed by SceneLoader/SceneSaver. |
| A-07 | The `Editor::world()` accessor returns a valid `World&` at any point during the Editor's lifetime (guaranteed by SPEC-029). |
| A-08 | `Window::set_title(std::string title)` exists on `buddd::engine::Window` base class. Implemented in WindowSDL3 via SDL_SetWindowTitle, and as a no-op in WindowHeadless. This method will be added as part of this feature. |
| A-09 | `SceneSaver::save_to_file()` creates the target file if it does not exist, and overwrites it if it does. |
| A-10 | `SceneLoader::load_from_file()` clears the World before populating it (or the Editor clears the World before calling load). The Editor clears the World explicitly before each load operation. |
| A-11 | A new, untitled scene has `dirty_ = false`, `file_path_ = ""`, and title "Untitled — Buddd Editor". |
| A-12 | ImGuiFileDialog's `Display()` is called within the Editor's `draw_ui()` frame, after the dockspace and panel rendering. |
| A-13 | The existing headless test infrastructure (`tests/editor_tests.cpp`) can be extended with new test cases that exercise Editor scene management methods directly without a display. |
| A-14 | `BUDDD_HAS_DISPLAY` CMake option controls whether display-dependent tests (`[.display]` tag) are compiled/run. |

## Open questions

No `[NEEDS CLARIFICATION]` markers remain. All questions were resolved during the grill-me step with the human.

| ID | Resolution |
|---|---|
| Q-01 | **Dirty state granularity**: Simple boolean only. No entity-level dirty tracking. |
| Q-02 | **SceneLoader/SceneSaver lifecycle**: Instantiated per-call, not stored as Editor members. |
| Q-03 | **Cancel behavior**: Cancel in save-prompt modal aborts the entire operation (no save, no proceed). |
| Q-04 | **Untitled scene initial dirty state**: `false` (clean). An empty untitled scene is clean until modified. |

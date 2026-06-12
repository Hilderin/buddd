# Scene Management

> **Current status (F-01 — editor-scene-load-save, June 2026):** File > New Scene, Open Scene, Save Scene, Save Scene As, and Quit are fully implemented with dirty state tracking (`*` in window title), OS-native file dialogs (ImGuiFileDialog), save-prompt modals, error modals for load/save failures, and OS close-button interception. All scene operations use the engine's `SceneLoader` and `SceneSaver` APIs for YAML serialization. Implementation and tests completed — see [SPEC-F-01](/.specs/sprint-2026-06/editor-scene-load-save/spec.md).

The Buddd Editor follows an **Unreal Engine-like** model: one scene open at a time in the dedicated Scene tab. All scene file operations (New, Open, Save, Save As, Quit) go through the **File** menu. The engine's `SceneLoader` and `SceneSaver` APIs handle YAML serialization/deserialization of entity hierarchies.

## How it works

### File Menu Operations

| Operation | Trigger | Shortcut | Behavior |
|---|---|---|---|
| **New Scene** | File > New Scene | `Ctrl+N` | Creates empty, untitled scene. Prompts save if current scene is dirty. Clears the World. |
| **Open Scene** | File > Open Scene | `Ctrl+O` | Opens OS file dialog (`.yaml` filter). Loads selected YAML via `SceneLoader`. Prompts save if current scene is dirty. |
| **Save Scene** | File > Save Scene | `Ctrl+S` | If scene has a file path, saves via `SceneSaver`. If untitled, behaves like Save As. Clears dirty indicator. Save on clean scene is a no-op. |
| **Save Scene As** | File > Save Scene As | `Ctrl+Shift+S` | Opens OS file dialog. Saves scene to chosen path. Updates scene's file path. |
| **Quit** | File > Quit | `Ctrl+Q` | Prompts save if the current scene is dirty. Exits application. |

### Dirty State

- **Simple boolean `dirty_`** on the `Editor` class — set via `Editor::mark_dirty()` on any entity/component mutation, cleared after successful Save or Save As.
- The editor window title shows an asterisk (`*`) suffix when dirty (e.g., `"scene_01.yaml* — Buddd Editor"` or `"Untitled* — Buddd Editor"`).
- A new untitled scene starts **clean** (`dirty_ = false`). The scene becomes dirty only on first modification.
- Save on a clean scene with a file path is a **no-op** (returns success, no dialog, no error).
- Scene-level boolean only — no entity-level dirty tracking. See [SPEC-F-01](/.specs/sprint-2026-06/editor-scene-load-save/spec.md), AC-04.

#### Window Title Format

Format: `"<file_name><dirty_suffix> — Buddd Editor"`

| Scenario | Title |
|---|---|
| Editor launch (untitled, clean) | `"Untitled — Buddd Editor"` |
| Untitled, modified | `"Untitled* — Buddd Editor"` |
| Scene loaded, clean | `"scene_01.yaml — Buddd Editor"` |
| Scene modified | `"scene_01.yaml* — Buddd Editor"` |

The title is built by `Editor::build_title_string()` and applied via `Window::set_title()` (engine API). `Window::set_title()` is a pure virtual method on the `Window` base class, implemented in `WindowSDL3` via `SDL_SetWindowTitle()` and as a no-op in `WindowHeadless`.

### Single-Scene Model

- **Only one scene is open at a time.** Opening another replaces the current scene (with save prompt if dirty).
- Multiple tabs (Scene, Prefab, Game) are **not yet implemented** — planned for future sprints.
- The Scene tab is always present and cannot be closed.

### Save-Prompt Modal

When the user attempts to close, open, or create a new scene with unsaved changes, a modal dialog appears:

> **"Save changes to [scene name]?"**

Three buttons:

| Button | Behavior |
|---|---|
| **Save** | Saves the current scene (Save As if untitled), then proceeds with the operation. |
| **Don't Save** | Discards changes, proceeds with the operation. |
| **Cancel** | Aborts the operation entirely. Current scene remains open and unchanged. |

The modal uses `ImGui::OpenPopup` + `ImGui::BeginPopupModal` with `ImGuiWindowFlags_AlwaysAutoResize` — the same pattern as the About popup.

**State machine**: The Editor uses a `PendingOp` enum (`None`, `NewScene`, `OpenScene`, `Quit`) to track the pending operation across frames. When `pending_op_ != None` and `dirty_ = true`, the save-prompt modal is drawn each frame until the user resolves it. On Cancel, `pending_op_` is cleared and the operation is aborted.

### Untitled Scene Behavior

- A newly created scene (File > New Scene) is **untitled** — it has no file path (`current_file_path_ = std::nullopt`).
- Initial dirty state is **`false`** (clean by default, not dirty).
- The window title shows `"Untitled — Buddd Editor"` (or `"Untitled* — Buddd Editor"` when modified).
- File > Save on an untitled scene **redirects to Save As** — the file dialog opens automatically.
- File > Save Scene As is always available and assigns a file path. After a successful Save As, `current_file_path_` is updated and `dirty_` is cleared.

### Error Modals

When a `SceneLoader` or `SceneSaver` operation fails, an error modal is shown:

- **Title**: "Error" (or "Load Error" / "Save Error")
- **Message**: The error description from the `Result` error.
- **Button**: "OK" to dismiss.
- **Effect on scene**: On load failure, the previous scene is preserved unchanged. On save failure, the scene remains dirty and the file path is NOT updated.

Error modals follow the same `ImGui::OpenPopup` + `BeginPopupModal` pattern as the About popup.

### OS File Dialogs (ImGuiFileDialog)

- **Library**: [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog) v0.6.7, fetched via `FetchContent` in the root `CMakeLists.txt`.
- Triggered by **Open Scene** and **Save Scene As** operations.
- Opens an OS-native file dialog.
- Filter: `\.yaml` files (`.yaml`, `.yml`).
- On OK: the selected path is passed to `open_scene(selected_path)` or `save_scene_as(selected_path)`.
- On Cancel: no action taken.

ImGuiFileDialog is compiled as part of `buddd_editor` (`ImGuiFileDialog.cpp` added to the static library sources, `${ImGuiFileDialog_SOURCE_DIR}` added as a system include path). `ImGuiFileDialog::Instance()` singleton pattern is used: `OpenDialog()` to open, `Display()` each frame, `IsOk()` to check result, `GetFilePathName()` to retrieve the selected path.

### OS Close Button (X / Alt+F4)

The OS window close button triggers the **same save-prompt** as File > Quit:

1. The OS sends `SDL_EVENT_QUIT` to `PlatformSDL3::poll_events()`.
2. The close-request callback (registered by the Editor in `Editor::setup()` via `Platform::set_on_close_request()`) is invoked.
3. If the scene is dirty, the callback sets `pending_op_ = PendingOp::Quit` and returns `false` (cancelling the close).
4. On the next frame, `draw_ui()` sees the pending quit and shows the save-prompt modal.
5. On **Save** → save scene, then call `ctx.request_exit()`.
6. On **Don't Save** → call `ctx.request_exit()`.
7. On **Cancel** → clear `pending_op_`, the close is aborted and the editor continues running.

If the scene is clean, the close-request callback returns `true` (allow close) and the editor exits immediately.

See `Platform::set_on_close_request(std::function<bool()>)` in `src/engine/platform/platform.h`. This is a **concrete** (non-virtual) method on the base class — the callback is stored in a `protected` member `close_request_callback_` accessed by `PlatformSDL3::poll_events()`.

### Scene File Format

Scenes are serialized as `.yaml` files via `SceneSaver` and loaded via `SceneLoader`. The YAML format includes entity hierarchy, component data, and type metadata. The `SceneLoader` and `SceneSaver` engine APIs are used as-is with no modifications — they are instantiated per-call (not stored as Editor members).

## Important conventions

### F-01 foundation (currently implemented)

- The Editor's File menu has **six items**: New Scene (`Ctrl+N`), Open Scene (`Ctrl+O`), separator, Save Scene (`Ctrl+S`), Save Scene As (`Ctrl+Shift+S`), separator, Quit (`Ctrl+Q`).
- Scene management is implemented via `Editor` public methods: `new_scene()`, `open_scene(path)`, `save_scene()`, `save_scene_as(path)`. All accept/return `be::Result<void>` for error propagation.
- Dirty state tracking with `dirty_` boolean + `*` window title suffix via `Editor::mark_dirty()` / `Editor::clear_dirty()` / `Editor::is_dirty()`.
- Save-prompt modal state machine using `PendingOp` enum and multi-frame `ImGui` popup pattern.
- Error modals for SceneLoader/SceneSaver failures, rendered in `draw_ui()` Phase 7.
- OS file dialogs via ImGuiFileDialog (FetchContent), integrated in `draw_ui()` Phase 6.
- OS close button interception via `Platform::set_on_close_request()` concrete method.
- `Window::set_title(std::string)` API on `Window` (pure virtual), `WindowSDL3` (SDL3 impl), `WindowHeadless` (no-op).
- Logging: all scene operations logged via `BUDDD_LOG_TAG("Editor")` with `INFO`, `WARN`, `DEBUG` levels.
- All docking layout is persisted via `buddd_editor.ini` in the CWD.
- The editor has a **two-phase lifecycle**: `Editor::update()` (shortcuts, state) + `Editor::draw_ui()` (menus, dockspace, panels, popups).
- The editor owns its own `World` via `std::unique_ptr<World>`, created in the **Editor constructor**, accessible via `editor.world()`, and destroyed in the **Editor destructor**.
- No SDL3, OpenGL, or GLM headers are included in `src/editor/` (per ADR-019).

### Future (post-MVP — not yet implemented)

- Prefab tab saving (future F-17).
- Play mode interaction (future F-14).
- Entity-level dirty tracking.
- "Revert" or "Reload" operations.
- Recent files list.
- Project panel double-click integration (future F-11).
- Autosave.
- Multi-tab dirty tracking.
- Cross-session file path persistence.

## Domain Concepts

| Concept | Description |
|---|---|
| **Scene** | A YAML file containing a complete entity hierarchy with all components and their properties. |
| **SceneLoader** | Engine API that parses YAML scene/prefab files and populates a `World` with entities and components. |
| **SceneSaver** | Engine API that serializes a `World` back to YAML, respecting entity source types (scene entities expanded, prefab/model entities referenced). |
| **World** | Container holding all entities, their hierarchy, and the component registry. |
| **Editor World** | The `Editor` class owns its own `World` instance via `std::unique_ptr<World>`. It is created in the **Editor constructor** (always empty on creation), exposed via `editor.world()`, and destroyed in the **Editor destructor** via `unique_ptr`. The World is valid for the entire Editor lifetime — it outlives `shutdown()` and is not reset by it. The Editor World is **separate** from `ctx.world` (the engine's demo-scene world). |

> **Lifecycle**: World is created in `Editor()` constructor → accessible via `world()` at any point (before `setup()`, after `setup()`, after `shutdown()`) → automatically destroyed by `~Editor()` destructor. No manual cleanup of the World is required in `shutdown()`. See [SPEC-029](/.specs/sprint-2026-06/editor-scene-state/spec.md).

## Related specs

- [SPEC-F-01 — Editor Scene Load/Save](/.specs/sprint-2026-06/editor-scene-load-save/spec.md) — Scene management implementation (current)
- [SPEC-028 — Editor Foundation](/.specs/sprint-2026-06/editor-foundation/spec.md) — Command system, menus, shortcuts, panels, docking persistence (v1 foundation)
- [SPEC-029 — Editor Scene State](/.specs/sprint-2026-06/editor-scene-state/spec.md) — Editor's own World lifecycle
- [SPEC-2026-06 — Editor UX Design (North-Star)](/.specs/sprint-2026-06/editor-ux-design/spec.md) — Complete editor UX design document (future vision)

## Related ADRs

- [ADR-027](/docs/adr/ADR-027-editor-architecture.md) — Editor architecture: separate static library, EditorApp adapter, namespace, architecture boundary
- [ADR-029](/docs/adr/ADR-029-editor-ux-decisions.md) — Editor UX decisions (panel layout, tab system, play mode)
- [ADR-014](/docs/adr/ADR-014-cli-app-system.md) — CLI App System (editor uses `run_app()`)
- [ADR-001](/docs/adr/ADR-001-error-result-pattern.md) — Result Error Pattern (used by scene management methods)
- [ADR-019](/docs/adr/ADR-019-architecture-boundaries.md) — Architecture boundaries (applies to `src/editor/`)
- [ADR-026](/docs/adr/ADR-026-imgui-integration.md) — Dear ImGui Integration (ImGuiFileDialog uses same context)

## Last reviewed

2026-06-12 — Updated for F-01 (SPEC-F-01): current implementation replaces north-star vision for scene management. Clean-by-default. ImGuiFileDialog, save-prompt, error modals, close-event hook documented.

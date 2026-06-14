# Scene Management

> **Current status (F-01 — editor-scene-load-save + F-02 — scene-panel-entity-tree + Editor Dialog Abstraction, June 2026):** File > New Scene, Open Scene, Save Scene, Save Scene As, and Quit are fully implemented with dirty state tracking (`*` in window title), OS-native file dialogs (SDL3 native dialogs via Platform abstraction), save-prompt modals, error modals for load/save failures, and OS close-button interception. All scene operations use the engine's `SceneLoader` and `SceneSaver` APIs for YAML serialization. **F-02 added** the `EditorContext` aggregate struct — panels now access the editor's World via `ctx.editor.world()` (instead of being limited to `EngineContext const&`). **Editor Dialog Abstraction**: Phase 4 of `draw_ui()` is now the general dialog rendering phase (replacing the old About popup phase). The About popup has been migrated to a `CustomDialog` instance. Save-prompt and error modals remain as separate ad-hoc modals (not yet ported to the Dialog abstraction). See [SPEC-F-01](/.specs/sprint-2026-06/editor-scene-load-save/spec.md), [SPEC-F-02](/.specs/sprint-2026-06/scene-panel-entity-tree/spec.md), and [Editor Dialog Abstraction spec](/.specs/sprint-2026-06/editor-dialog-abstraction/spec.md).

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

The modal uses `ImGui::OpenPopup` + `ImGui::BeginPopupModal` with `ImGuiWindowFlags_AlwaysAutoResize`. Unlike the About popup (which now uses the reusable `CustomDialog` abstraction), the save-prompt remains a direct ad-hoc modal — it has not yet been ported to the Dialog abstraction.

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

Error modals follow the same `ImGui::OpenPopup` + `BeginPopupModal` pattern as the old About popup. Like the save-prompt, they remain as direct ad-hoc modals (not yet ported to the Dialog abstraction).

### OS File Dialogs (SDL3 Native via Platform Abstraction)

- **Mechanism**: SDL3 native file dialogs, abstracted through the `Platform` interface in `src/engine/platform/platform.h`.
- Triggered by **Open Scene** and **Save Scene As** operations.
- Opens an OS-native file dialog via SDL3 (`SDL_ShowOpenFileDialog` / `SDL_ShowSaveFileDialog`).
- Filter: `"YAML Scene"` / `"yaml"` — only `.yaml` files are selectable.
- On selection: the Platform dialog callback invokes `open_scene(path)` or `save_scene_as(path)` directly.
- On Cancel or error: callback invoked with `std::nullopt`, no action taken.

The Platform defines a `FileDialogCallback` type alias (`std::function<void(std::optional<std::string>)>`) — editor code never includes SDL3 headers (per ADR-019). PlatformSDL3 heap-allocates the callback, passes it as `userdata` to the SDL3 C API; the C-lambda deletes the callback after invocation. The SDL3 dialog callback fires on the main thread during `SDL_PumpEvents` (inside `poll_events()`), so no mutex or intermediate queue is needed — Editor lambdas can safely access Editor state directly.

**API signatures** (on `Platform`):
- `show_open_file_dialog(FileDialogCallback, filter_name, filter_pattern)` — used by Open Scene.
- `show_save_file_dialog(FileDialogCallback, filter_name, filter_pattern, default_name)` — used by Save Scene As and for the untitled Save redirect.

**Headless backend**: `PlatformHeadless` immediately invokes the callback with `std::nullopt` (no-op — no file dialog in headless mode).

**Callback lifecycle**: The heap-allocated `std::function` is created before the SDL3 API call and destroyed by the SDL3 C-lambda after invocation. The `SDL_DialogFileFilter` struct is stack-allocated (SDL3 copies the data internally). No mutex, no synchronization, no intermediate result queue.

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
- **Editor Dialog Abstraction**: Phase 4 of `draw_ui()` is now the general dialog rendering phase — replaces the old About popup phase. Iterates `dialogs_`, opens popups via `ImGui::OpenPopup` with first-frame-only tracking via `opened_dialog_ids_`, renders via `ImGui::BeginPopupModal`, dispatches Escape to the topmost dialog only, and removes closed dialogs via `std::erase_if`. The About popup has been migrated to a `CustomDialog` instance. Save-prompt and error modals are **not** ported — they remain ad-hoc modals rendered in Phase 7 (error modals) and via the `PendingOp` state machine (save-prompt).
- OS file dialogs via Platform abstraction (SDL3 native dialogs), with direct callback invocation (no `draw_file_dialog()` or Phase 6 ImGuiFileDialog step — the Phase 6 slot was repurposed for `request_exit_next_frame_` check).
- OS close button interception via `Platform::set_on_close_request()` concrete method.
- `Window::set_title(std::string)` API on `Window` (pure virtual), `WindowSDL3` (SDL3 impl), `WindowHeadless` (no-op).
- Logging: all scene operations logged via `BUDDD_LOG_TAG("Editor")` with `INFO`, `WARN`, `DEBUG` levels.
- All docking layout is persisted via `buddd_editor.ini` in the CWD.
- The editor has a **two-phase lifecycle**: `Editor::update()` (shortcuts, state) + `Editor::draw_ui()` (menus, dockspace, panels, popups).
- The editor owns its own `World` via `std::unique_ptr<World>`, created in the **Editor constructor**, accessible via `editor.world()`, and destroyed in the **Editor destructor**.
- No SDL3, OpenGL, or GLM headers are included in `src/editor/` (per ADR-019).
- **F-02 addition**: `EditorContext` aggregate struct (`src/editor/editor_context.h`) introduced — panels and menus now receive `EditorContext const&` in their `update()`/`draw_ui()` methods, enabling panels to access the editor's World via `ctx.editor.world()` and engine services via `ctx.engine`. See [F-02 spec](/.specs/sprint-2026-06/scene-panel-entity-tree/spec.md).

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
| **Editor World** | The `Editor` class owns its own `World` instance via `std::unique_ptr<World>`. It is created in the **Editor constructor** (always empty on creation), exposed via `editor.world()`, and destroyed in the **Editor destructor** via `unique_ptr`. The World is valid for the entire Editor lifetime — it outlives `shutdown()` and is not reset by it. The Editor World is **separate** from `ctx.world` (the engine's demo-scene world). **Panel access (F-02)**: Editor panels access this World via `ctx.editor.world()` where `ctx` is the `EditorContext` passed to `update()`/`draw_ui()` — see [F-02 spec](/.specs/sprint-2026-06/scene-panel-entity-tree/spec.md). |

> **Lifecycle**: World is created in `Editor()` constructor → accessible via `world()` at any point (before `setup()`, after `setup()`, after `shutdown()`) → automatically destroyed by `~Editor()` destructor. No manual cleanup of the World is required in `shutdown()`. See [SPEC-029](/.specs/sprint-2026-06/editor-scene-state/spec.md).

## Related specs

- [SPEC-F-01 — Editor Scene Load/Save](/.specs/sprint-2026-06/editor-scene-load-save/spec.md) — Scene management implementation (current)
- [SPEC-F-02 — Scene Panel Entity Tree](/.specs/sprint-2026-06/scene-panel-entity-tree/spec.md) — `EditorContext` struct, panel World access via `ctx.editor.world()`
- [SPEC-028 — Editor Foundation](/.specs/sprint-2026-06/editor-foundation/spec.md) — Command system, menus, shortcuts, panels, docking persistence (v1 foundation)
- [SPEC-029 — Editor Scene State](/.specs/sprint-2026-06/editor-scene-state/spec.md) — Editor's own World lifecycle
- [SPEC-2026-06 — Editor UX Design (North-Star)](/.specs/sprint-2026-06/editor-ux-design/spec.md) — Complete editor UX design document (future vision)
- [Editor Dialog Abstraction spec](/.specs/sprint-2026-06/editor-dialog-abstraction/spec.md) — Reusable Dialog base class, CustomDialog, DialogButton, ID-based dedup, Phase 4 dialog rendering

## Related ADRs

- [ADR-027](/docs/adr/ADR-027-editor-architecture.md) — Editor architecture: separate static library, EditorApp adapter, namespace, architecture boundary
- [ADR-029](/docs/adr/ADR-029-editor-ux-decisions.md) — Editor UX decisions (panel layout, tab system, play mode)
- [ADR-014](/docs/adr/ADR-014-cli-app-system.md) — CLI App System (editor uses `run_app()`)
- [ADR-001](/docs/adr/ADR-001-error-result-pattern.md) — Result Error Pattern (used by scene management methods)
- [ADR-019](/docs/adr/ADR-019-architecture-boundaries.md) — Architecture boundaries (applies to `src/editor/`)
- [ADR-026](/docs/adr/ADR-026-imgui-integration.md) — Dear ImGui Integration (ImGui remains the UI library; ImGuiFileDialog was replaced by SDL3 native dialogs)

## Last reviewed

2026-06-13 — Updated for Editor Dialog Abstraction: Phase 4 is now the general dialog rendering phase (About popup migrated to CustomDialog). Save-prompt and error modals remain as separate ad-hoc modals. `editor_dialog.h` added with `Dialog`, `CustomDialog`, `DialogButton`. `Editor` gains `open_dialog()`, `dialogs_`, `opened_dialog_ids_`.

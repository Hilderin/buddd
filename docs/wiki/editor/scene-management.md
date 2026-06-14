# Scene Management

> **Current status (F-01 — editor-scene-load-save + F-02 — scene-panel-entity-tree + Editor Dialog Abstraction + Port Popups to Dialog, June 2026):** File > New Scene, Open Scene, Save Scene, Save Scene As, and Quit are fully implemented with dirty state tracking (`*` in window title), OS-native file dialogs (SDL3 native dialogs via Platform abstraction), save-prompt modals, error modals for load/save failures, and OS close-button interception. All scene operations use the engine's `SceneLoader` and `SceneSaver` APIs for YAML serialization. **F-02 added** the `EditorContext` aggregate struct — panels now access the editor's World via `ctx.editor.world()` (instead of being limited to `EngineContext const&`). **Editor Dialog Abstraction**: Phase 4 of `draw_ui()` is now the general dialog rendering phase (replacing the old About popup phase). The About popup has been migrated to a `CustomDialog` instance. **Port Popups to Dialog**: Save-prompt, error modals, and delete-confirmation have all been ported from ad-hoc ImGui popups to the Dialog abstraction. See [SPEC-F-01](/.specs/sprint-2026-06/editor-scene-load-save/spec.md), [SPEC-F-02](/.specs/sprint-2026-06/scene-panel-entity-tree/spec.md), [Editor Dialog Abstraction spec](/.specs/sprint-2026-06/editor-dialog-abstraction/spec.md), and [Port Popups to Dialog spec](/.specs/sprint-2026-06/port-popups-to-dialog/spec.md).

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

The modal now uses the Dialog abstraction (`CustomDialog` with button callbacks). A fixed ID (`"save-changes"`) provides dedup — only one save-prompt can be open at a time. The three buttons (Save, Don't Save, Cancel) each have a callback that executes the corresponding action directly (save scene with optional Save-As redirect, discard changes, or abort). Unlike the old state-machine approach, no `SavePromptResult` enum or polling is needed — callbacks fire synchronously and close the dialog automatically when they return `true`.

**State machine**: The Editor uses a `PendingOp` enum (`None`, `NewScene`, `OpenScene`, `Quit`) to track the pending operation across frames. When `pending_op_ != None` and `dirty_ = true`, `draw_pending_op_modal()` (Phase 5) opens a `CustomDialog` via `open_dialog()` with the save-prompt. The dialog is rendered in Phase 4 via the Dialog abstraction. On Cancel, `pending_op_` is cleared and the operation is aborted.

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

Error modals now use the Dialog abstraction (`CustomDialog`) with a randomly generated unique ID (`time() + random()`). Each error gets its own dialog instance, allowing multiple errors to stack independently. A convenience helper `Editor::open_error_dialog()` creates the CustomDialog and calls `open_dialog()`, replacing the old `show_error_modal()` / `draw_error_modals()` pattern.

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
- Save-prompt modal using `CustomDialog` via `open_dialog()` with button callbacks (Save/Don't Save/Cancel), driven by `PendingOp` enum. `SavePromptResult` enum removed. `draw_save_prompt_modal()` removed.
- Error modals for SceneLoader/SceneSaver failures, now using `CustomDialog` via `open_error_dialog()` convenience helper (random unique ID per instance). `draw_error_modals()` and `show_error_modal()` removed.
- **Editor Dialog Abstraction**: Phase 4 of `draw_ui()` is now the general dialog rendering phase — replaces the old About popup phase. Iterates `dialogs_`, opens popups via `ImGui::OpenPopup((dialog->title() + "###" + dialog->id()).c_str())` (the `"title###id"` pattern prevents ImGui ID collisions between same-titled dialogs), renders via `ImGui::BeginPopupModal`, dispatches Escape to the topmost dialog only, and removes closed dialogs via `std::erase_if`. The About popup has been migrated to a `CustomDialog` instance. **All popups (save-prompt, error modals, delete confirmation) now use the Dialog abstraction** — no ad-hoc ImGui popup code remains for these. `DialogButton::callback` type changed from `void()` to `bool()` (return `true` to close dialog). New convenience helpers: `open_message_dialog()`, `open_error_dialog()`, `open_confirm_dialog()`, `open_ok_cancel_dialog()`. A `defer()` mechanism queues actions for execution at the top of the next `draw_ui()` frame with a fresh `EditorContext`.
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

2026-06-14 — Updated for Port Popups to Dialog: All remaining popups (save-prompt, error modals, delete confirmation) ported to Dialog abstraction. `DialogButton::callback` changed to `bool()`. New convenience helpers: `open_message_dialog()`, `open_error_dialog()`, `open_confirm_dialog()`, `open_ok_cancel_dialog()`, `defer()`. Removed: `SavePromptResult`, `draw_save_prompt_modal()`, `draw_error_modals()`, `show_error_modal()`, 8 state members from `Editor`, 4 state members from `ScenePanel`. Phase 4 uses `"title###id"` pattern for ImGui popup IDs.

# IMPL-F-01 — Editor Scene Load/Save Integration

## Source spec

`.specs/sprint-2026-06/editor-scene-load-save/spec.md`

## Goal

Wire the existing engine SceneLoader/SceneSaver APIs into the Editor's File menu, establishing the fundamental save/open workflow that all subsequent editor features depend on. This includes extending the File menu with New/Open/Save/Save As/Quit, adding dirty state tracking with a `*` indicator in the window title, integrating ImGuiFileDialog for OS-native file dialogs, showing save-prompt modals when the user attempts to close/open/create a scene with unsaved changes, and displaying error modals on SceneLoader/SceneSaver failures.

## Non-goals

- No changes to `SceneLoader`, `SceneSaver`, `World`, `Entity`, `ComponentRegistry`, `AssetManager`, or any engine core type beyond `Window::set_title()` and `Platform::set_on_close_request()`.
- No changes to existing editor panels beyond `MenuBar`.
- No changes to `EditorApp` or `run_app()`.
- No entity-level dirty tracking — simple scene-level boolean only.
- No prefab tab saving, Play mode interaction, autosave, recent files, multi-tab dirty tracking, or cross-session file path persistence.
- No "Revert" or "Reload" operations.
- No changes to ImGui initialization, docking layout, or panel lifecycle.
- No new dependencies beyond ImGuiFileDialog.
- ImGuiFileDialog's internal dependencies (e.g., `nfd` for native dialogs) are fetched automatically by FetchContent.

## Relevant ADRs

| ADR | How it constrains implementation |
|---|---|
| ADR-019 (Architecture Boundaries) | No SDL3/OpenGL/GLM headers may appear in `src/editor/`. ImGuiFileDialog header inclusion in editor code is permitted (ImGui is already allowed). The `Window` abstraction is the only path for title changes. |
| ADR-027 (Editor Architecture) | Editor is a static library (`buddd_editor`) that links `buddd_engine` as PUBLIC. All engine access goes through `EngineContext`/`EngineService`. The Editor uses direct member variables (no PIMPL). Architecture boundary extends to `src/editor/`. |
| ADR-029 (Editor UX Decisions) | One-scene-at-a-time model (Decision 2). Scene tab is always present. Save-prompt modal matches the about-popup pattern for multi-frame modals. Decision 9 (Console persistence) and Decision 10 (Play mode indicators) are not directly affected. |
| ADR-001 (Result Error Pattern) | SceneLoader/SceneSaver return `Result<void>`. Error propagation must use the existing `Result<T>` pattern. |
| ADR-026 (Dear ImGui Integration) | ImGui docking branch. ImGuiFileDialog uses the same ImGui context. Init failure is fatal in display mode. |

## Files to inspect

The Code Agent must read these files before making any edits:

| File | What to look for |
|---|---|
| `src/engine/window/window.h` | Current virtual interface — insert `set_title()` alongside existing methods. |
| `src/engine/window/window_sdl3.h` | SDL3 `Window` subclass declaration — add `set_title` override. |
| `src/engine/window/window_sdl3.cpp` | SDL3 implementation — `SDL_SetWindowTitle` usage pattern. |
| `src/engine/window/window_headless.h` | Headless `Window` subclass declaration — add `set_title` override. |
| `src/engine/window/window_headless.cpp` | Headless implementation pattern (all methods are no-ops). |
| `src/engine/platform/platform.h` | Current Platform base class — pure virtual `poll_events()`, member storage pattern. |
| `src/engine/platform/platform_sdl3.cpp` | `SDL_EVENT_QUIT` handling in `poll_events()` — currently returns `false` immediately. |
| `src/engine/platform/platform_sdl3.h` | `PlatformSDL3` class declaration, existing private members. |
| `src/engine/platform/platform_headless.h` / `.cpp` | Headless Platform — no-op `poll_events()` pattern (always returns `true`). |
| `src/engine/engine_service.h` | `EngineService::platform()` accessor — Editor calls `ctx.services.platform().set_on_close_request(...)`. |
| `CMakeLists.txt` (root) | FetchContent pattern for Catch2 — replicate for ImGuiFileDialog. |
| `src/editor/CMakeLists.txt` | Current `buddd_editor` target link and source lists. |
| `src/editor/editor.h` | Existing members, lifecycle methods, forward declarations. |
| `src/editor/editor.cpp` | Existing setup/update/draw_ui/shutdown implementation, shortcut bindings, About popup pattern, World management. |
| `src/editor/panels/menu_bar.h` | Existing File > Quit menu layout, `set_on_about` callback pattern, `EditorMenu` base class API. |
| `src/editor/editor_menu.h` | Base class interface (`id()`, `update()`, `draw_ui()`). |
| `src/cmd/apps/editor_app.h` / `.cpp` | How Editor is created, setup, and destroyed — verify no changes needed. |
| `src/engine/engine_context.h` | `EngineContext` struct — `services`, `window`, `request_exit()` access. |
| `src/engine/engine_service.h` | `registry()` and `assets()` accessors used by SceneLoader/SceneSaver. |
| `src/engine/scene/scene_loader.h` | `SceneLoader(World&, ComponentRegistry&, AssetManager&)` constructor and `load_from_file(path)` signature. |
| `src/engine/scene/scene_saver.h` | `SceneSaver(World&, ComponentRegistry&, AssetManager&)` constructor and `save_to_file(path)` signature. |
| `tests/editor_tests.cpp` | Test conventions: Catch2 macros, `[editor]` tags, headless engine creation pattern, `#ifdef BUDDD_HAS_DISPLAY` guard, `Backend::Headless` usage. |
| `tests/CMakeLists.txt` | Build linkage — `buddd_editor` already linked to tests. Auto-discovery by `file(GLOB ... *_tests.cpp)`. |

## Files allowed to change

1. `src/engine/window/window.h` — add `virtual auto set_title(std::string title) -> void = 0;`
2. `src/engine/window/window_sdl3.h` — add `auto set_title(std::string title) -> void override;`
3. `src/engine/window/window_sdl3.cpp` — implement `set_title` via `SDL_SetWindowTitle(window_, title.c_str())`
4. `src/engine/window/window_headless.h` — add `auto set_title(std::string title) -> void override;`
5. `src/engine/window/window_headless.cpp` — implement `set_title` as no-op
6. `CMakeLists.txt` (root) — add FetchContent for ImGuiFileDialog
7. `src/editor/CMakeLists.txt` — link `imgui_file_dialog` to `buddd_editor`
8. `src/editor/editor.h` — add members, methods, enums for scene management
9. `src/editor/editor.cpp` — implement scene management, save-prompt, error modals, shortcut changes
10. `src/editor/panels/menu_bar.h` — add menu items and callbacks for New/Open/Save/Save As
11. `src/engine/platform/platform.h` — add `set_on_close_request(std::function<bool()>)` concrete method and `close_request_callback_` member
12. `src/engine/platform/platform_sdl3.cpp` — intercept `SDL_EVENT_QUIT` via close-request callback; if registered and returns `false`, swallow the quit event (return `true`)
13. `tests/editor_tests.cpp` — add F-01 unit and integration tests

## Files forbidden to change

- `src/engine/scene/scene_loader.h` / `.cpp` — no changes.
- `src/engine/scene/scene_saver.h` / `.cpp` — no changes.
- `src/engine/scene/world.h` / `.cpp` — no changes.
- `src/cmd/apps/editor_app.h` / `.cpp` — no changes.
- `src/cmd/app.h` / `.cpp` — no changes.
- Any file under `src/engine/scene/`, `src/engine/render/`, `src/engine/asset/` — no changes.
- Any existing `src/editor/panels/*` files other than `menu_bar.h`.
- Any existing test fixtures or test assets that are not explicitly for F-01.

## Existing conventions to follow

- **Naming**: `snake_case` for variables/methods, `PascalCase` for classes/enums. Use `auto` return type with trailing return type syntax.
- **Namespace**: All editor code in `namespace buddd::editor`. Engine types referenced as `be::TypeName` via `namespace be = buddd::engine;`.
- **Include style**: Use `#include "editor.h"` for local paths, `<imgui.h>` for system/third-party headers. Never include `<SDL3/` or `<yaml-cpp/` from editor code (ADR-019).
- **Shortcuts**: `ShortcutRegistry` with `shortcuts_.bind(KeyCode, Modifiers, callback)` in `Editor::setup()`. Callbacks receive `EngineContext const&`.
- **Callbacks**: `MenuBar` uses `std::function<void()>` callbacks set via `set_on_*` methods. Pattern: `set_on_about` in existing code.
- **Logging**: `BUDDD_LOG_TAG("Editor")` already set in `editor.cpp`. Use `BUDDD_LOG_DEBUG`, `BUDDD_LOG_INFO`, `BUDDD_LOG_WARN` for messages with the same tag.
- **Error handling**: `Result<T>` pattern with `make_error(Error::Category, message)`. Return `Result<void>` and check `.has_value()`.
- **ImGui popups**: `ShowAboutPopup` pattern — flag `show_about_` + `draw_about_popup()`. Open with `ImGui::OpenPopup()`, render with `BeginPopupModal`, use `ImGuiWindowFlags_AlwaysAutoResize`.
- **Test conventions**: Catch2 v3, `TEST_CASE("name", "[editor][tag]")`. Headless engine via `EngineService::create(Backend::Headless, ...)`. `#ifdef BUDDD_HAS_DISPLAY` guard for display-dependent tests.
- **World access**: `Editor::world()` returns a valid `World&` at all times. The world is created in the constructor and destroyed in the destructor.
- **EngineContext::request_exit()**: Only called from shortcuts or Update/draw_ui, not from constructors or setup.

## Required implementation behavior

### Step 1: Window::set_title() — Engine Window interface

1. In `src/engine/window/window.h`:
   - Add `virtual auto set_title(std::string title) -> void = 0;` after the existing pure virtual methods (after `on_resize`, before `set_mouse_capture`). Keep alphabetical ordering among virtual methods.

2. In `src/engine/window/window_sdl3.h`:
   - Add `auto set_title(std::string title) -> void override;` declaration.

3. In `src/engine/window/window_sdl3.cpp`:
   - Implement `WindowSDL3::set_title(std::string title)` as:
     ```cpp
     auto WindowSDL3::set_title(std::string title) -> void {
         SDL_SetWindowTitle(window_, title.c_str());
     }
     ```

4. In `src/engine/window/window_headless.h`:
   - Add `auto set_title(std::string title) -> void override;` declaration.

5. In `src/engine/window/window_headless.cpp`:
   - Implement as no-op (like `set_mouse_capture`).

### Step 2: ImGuiFileDialog dependency

6. In root `CMakeLists.txt`:
   - After the Catch2 FetchContent block, add:
     ```cmake
     FetchContent_Declare(
         ImGuiFileDialog
         GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog.git
         GIT_TAG v0.6.7
     )
     # Do NOT use FetchContent_MakeAvailable — handled manually below
     ```
   - After `FetchContent_MakeAvailable(Catch2)`, add:
     ```cmake
     # ImGuiFileDialog — header-only, but needs source compilation for impl
     FetchContent_GetProperties(ImGuiFileDialog)
     if(NOT ImGuiFileDialog_POPULATED)
         FetchContent_Populate(ImGuiFileDialog)
     endif()
     ```
   - The variable `ImGuiFileDialog_SOURCE_DIR` is available for CMake to reference.

7. In `src/editor/CMakeLists.txt`:
   - After the existing `target_link_libraries(buddd_editor ...)` block:
     ```cmake
     # ImGuiFileDialog (header-only — add include path)
     target_include_directories(buddd_editor SYSTEM PRIVATE
         ${ImGuiFileDialog_SOURCE_DIR}
     )
     ```
   - Add the single `.cpp` source to the `add_library(buddd_editor STATIC ...)` call:
     ```cmake
     add_library(buddd_editor STATIC
         editor.cpp
         command.cpp
         command_stack.cpp
         ${ImGuiFileDialog_SOURCE_DIR}/ImGuiFileDialog.cpp
     )
     ```
   - Also add an include directory for `ImGuiFileDialog/`:
     ```cmake
     target_include_directories(buddd_editor SYSTEM PRIVATE
         ${ImGuiFileDialog_SOURCE_DIR}
     )
     ```

   **Note**: The exact file path may be `ImGuiFileDialog_SOURCE_DIR/ImGuiFileDialog.cpp` or in a subdirectory. The Code Agent shall verify the actual path after FetchContent populates the directory and adjust accordingly.

### Step 3: Editor class — header additions (editor.h)

8. Add `#include <optional>` to the existing includes in `editor.h`.

9. Add a `SavePromptResult` enum inside the `buddd::editor` namespace (before the `Editor` class or as a nested enum in the class):
   ```cpp
   enum class SavePromptResult { Save, Discard, Cancel };
   ```

10. Add a `PendingOperation` enum:
    ```cpp
    enum class PendingOp { None, NewScene, OpenScene, Quit };
    ```

11. Add the following public methods to the `Editor` class:
    ```cpp
    /// Mark the current scene as having unsaved changes.
    auto mark_dirty() -> void;

    /// Clear the dirty flag (called after a successful save).
    auto clear_dirty() -> void;

    /// Returns true if the scene has unsaved changes.
    [[nodiscard]] auto is_dirty() const noexcept -> bool;

    /// Returns the current file path, or std::nullopt if untitled.
    [[nodiscard]] auto current_file_path() const noexcept -> const std::optional<std::string>&;

    /// Create a new untitled scene, replacing the current World.
    /// Empties the World (replaces with a fresh empty World).
    /// Resets file path to nullopt and clears dirty flag.
    auto new_scene() -> void;

    /// Load a scene from the given file path into the Editor's World.
    /// The Editor's World is replaced with a fresh empty World before loading.
    /// Returns error if the file cannot be loaded.
    [[nodiscard]] auto open_scene(const std::string& path) -> be::Result<void>;

    /// Save the current scene to the current file path.
    /// If untitled (no file path), returns an error — caller must use save_scene_as() instead.
    /// On success, clears dirty flag and logs the save.
    [[nodiscard]] auto save_scene() -> be::Result<void>;

    /// Save the current scene to the given file path.
    /// Updates current_file_path_ to the new path.
    /// On success, clears dirty flag and logs the save.
    [[nodiscard]] auto save_scene_as(const std::string& path) -> be::Result<void>;
    ```

12. Add the following private methods to the `Editor` class:
    ```cpp
    /// Update the OS window title based on current file path and dirty state.
    /// Format: "<basename><*> — Buddd Editor"
    /// Examples: "Untitled — Buddd Editor", "scene.yaml* — Buddd Editor"
    auto update_window_title() -> void;

    /// Helper: construct the title string without calling Window::set_title.
    /// Returns the formatted title string (e.g., "scene.yaml* — Buddd Editor").
    [[nodiscard]] auto build_title_string() const -> std::string;

    /// Save-prompt modal: "Save changes to <scene name>?"
    /// Returns SavePromptResult based on user action.
    auto draw_save_prompt_modal() -> SavePromptResult;

    /// Show an error modal with the given title and message.
    auto show_error_modal(const std::string& title, const std::string& message) -> void;

    /// Draw error modals (rendered every frame while flags are set).
    auto draw_error_modals() -> void;

    /// Draw the save-prompt modal (rendered every frame while pending_op_ is set and dirty_ is true).
    auto draw_pending_op_modal() -> void;

    /// Execute the pending operation after save-prompt resolves.
    auto execute_pending_op(be::EngineContext const& ctx) -> void;

    /// Check dirty state and show save-prompt, return true if the operation is allowed to proceed.
    /// If the user cancels, returns false and the operation is aborted.
    /// If the user saves, saves first (or redirects to Save As if untitled) then returns true.
    /// If the user discards, returns true without saving.
    [[nodiscard]] auto handle_dirty_before_op(be::EngineContext const& ctx, PendingOp op) -> bool;
    ```

13. Add the following private members to the `Editor` class:
    ```cpp
    // ── Scene management state ──
    bool dirty_ = false;
    std::optional<std::string> current_file_path_;

    // ── Pending operation (multi-frame save-prompt) ──
    PendingOp pending_op_ = PendingOp::None;
    std::optional<std::string> pending_file_path_; // for Open/SaveAs

    // ── Error modal state ──
    std::string error_modal_title_;
    std::string error_modal_message_;
    bool show_error_modal_ = false;

    // ── Save-prompt modal state ──
    bool show_save_prompt_modal_ = false;
    SavePromptResult save_prompt_result_ = SavePromptResult::Cancel;
    ```

    Place these after the existing `show_about_ = false;` member and before `std::unique_ptr<buddd::engine::World> world_;`.

### Step 4: Editor class — implementation (editor.cpp)

14. **mark_dirty()**: Set `dirty_ = true` and call `update_window_title()`. Log `BUDDD_LOG_DEBUG("Scene marked dirty")`.

15. **clear_dirty()**: Set `dirty_ = false` and call `update_window_title()`. Log `BUDDD_LOG_DEBUG("Scene dirty cleared")`.

16. **is_dirty()**: Return `dirty_`.

17. **current_file_path()**: Return `current_file_path_`.

18. **build_title_string()**: Format the window title:
    - If `current_file_path_` has a value, extract the filename portion (basename) using `std::filesystem::path::filename()`.
    - If `current_file_path_` is nullopt, use `"Untitled"`.
    - If `dirty_` is true, append `*` to the filename.
    - Append `" — Buddd Editor"`.
    - Return the full string.

19. **update_window_title()**: Call `window_->set_title(build_title_string())`. Guard with `if (!window_) return;`.

20. **new_scene()**: 
    - Replace `world_` with `std::make_unique<be::World>()`.
    - Set `current_file_path_` to `std::nullopt`.
    - Set `dirty_` to `false`.
    - Call `update_window_title()`.
    - Log `BUDDD_LOG_INFO("New scene created")`.

21. **open_scene(const std::string& path)**:
    - Save the current World aside (local copy of `world_` unique_ptr) before replacing, so that on failure we can restore it.
    - Create a fresh `be::World()` in `world_`.
    - Retrieve `ComponentRegistry&` from `engine_->registry()` and `AssetManager&` from `engine_->assets()`.
    - Construct a `SceneLoader` instance with `(*world_, registry, assets)`.
    - Call `loader.load_from_file(path)`.
    - If successful:
      - Set `current_file_path_` to `path`.
      - Set `dirty_` to `false`.
      - Call `update_window_title()`.
      - Log `BUDDD_LOG_INFO("Scene loaded: {}", path)`.
      - Return success.
    - If failure:
      - Restore the saved World (replace `world_` with the saved copy).
      - Log `BUDDD_LOG_WARN("Scene load failed: {}", error.message())`.
      - Return the error.
    - **Important**: The World replacement strategy (rather than clearing entities in-place) ensures all entity/node references are clean. The `world()` accessor returns a reference to the current `world_` — callers should not store references across this call.

22. **save_scene()**:
    - If `current_file_path_` is nullopt, return error (caller should use `save_scene_as`): `make_error(Error::Category::InvalidArgument, "No file path set — use save_scene_as instead")`.
    - Retrieve `ComponentRegistry&` and `AssetManager&` from `engine_`.
    - Construct a `SceneSaver` instance with `(*world_, registry, assets)`.
    - Call `saver.save_to_file(*current_file_path_)`.
    - If successful:
      - Set `dirty_` to `false`.
      - Call `update_window_title()`.
      - Log `BUDDD_LOG_INFO("Scene saved: {}", *current_file_path_)`.
      - Return success.
    - If failure:
      - Log `BUDDD_LOG_WARN("Scene save failed: {}", error.message())`.
      - Return the error.
      - **Do NOT clear dirty_ on failure. Do NOT change current_file_path_.**

23. **save_scene_as(const std::string& path)**:
    - Retrieve `ComponentRegistry&` and `AssetManager&` from `engine_`.
    - Construct a `SceneSaver` instance with `(*world_, registry, assets)`.
    - Call `saver.save_to_file(path)`.
    - If successful:
      - Set `current_file_path_` to `path`.
      - Set `dirty_` to `false`.
      - Call `update_window_title()`.
      - Log `BUDDD_LOG_INFO("Scene saved: {}", path)`.
      - Return success.
    - If failure:
      - Log `BUDDD_LOG_WARN("Scene save failed: {}", error.message())`.
      - Return the error.
      - **Do NOT clear dirty_ on failure. Do NOT update current_file_path_.**

24. **show_error_modal(title, message)**:
    - Set `error_modal_title_` = title, `error_modal_message_` = message, `show_error_modal_` = true.

25. **draw_error_modals()**:
    - If `!show_error_modal_` return.
    - Call `ImGui::OpenPopup(error_modal_title_.c_str())`.
    - Render with `ImGui::BeginPopupModal(error_modal_title_.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)`.
    - Show `error_modal_message_` with `ImGui::Text`.
    - A single "OK" button that calls `ImGui::CloseCurrentPopup()` and sets `show_error_modal_ = false`.
    - On Escape or click-outside dismiss, set `show_error_modal_ = false`.
    - Follow the exact same pattern as `draw_about_popup`.

26. **draw_save_prompt_modal()**:
    - Use `ImGui::OpenPopup("Save Changes")`.
    - Render via `ImGui::BeginPopupModal("Save Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)`.
    - Message: `"Save changes to " + scene_name + "?"` where scene_name is the basename of `current_file_path_` or `"Untitled"`.
    - Three buttons side-by-side: **Save**, **Don't Save**, **Cancel**.
      - **Save**: `ImGui::CloseCurrentPopup()`, set local result = `SavePromptResult::Save`.
      - **Don't Save**: `ImGui::CloseCurrentPopup()`, set local result = `SavePromptResult::Discard`.
      - **Cancel**: `ImGui::CloseCurrentPopup()`, set local result = `SavePromptResult::Cancel`.
    - On Escape or click-outside dismiss, treat as Cancel (`SavePromptResult::Cancel`).
    - Return the `SavePromptResult`.

27. **Handle dirty before operations** — state machine for multi-frame modals:

    The Editor uses a state machine for operations that may trigger a save-prompt:

    ```
    User action (shortcut/menu) → set pending_op_ = NewScene/OpenScene/Quit
                                 → set pending_file_path_ if applicable
    Every frame in draw_ui():
      if pending_op_ != None and dirty_:
        → draw_save_prompt_modal()
        → based on result:
           Save:     save current scene (or redirect to Save As if untitled),
                     then execute pending operation, clear pending_op_
           Discard:  execute pending operation, clear pending_op_
           Cancel:   clear pending_op_ (abort the operation, no save, no proceed)
      if pending_op_ != None and !dirty_:
        → execute pending operation immediately, clear pending_op_
    ```

    Execute pending operation means:
    - **NewScene**: call `new_scene()`.
    - **OpenScene**: call `open_scene(pending_file_path_.value())`.
    - **Quit**: call `ctx.request_exit()`.

    > **Important clarification for Open Scene flow**: The OS file dialog (ImGuiFileDialog) itself is multi-frame and cannot be combined with the save-prompt state machine in a single step. The correct two-pass flow is:
    > 1. **Pass 1 — save-prompt**: User clicks Open Scene. If dirty, set `pending_op_ = OpenScene`. Save-prompt is drawn in `draw_ui()` each frame until user clicks Save/Discard/Cancel.
    > 2. **Pass 2 — file dialog**: On Save/Discard (resolve), instead of immediately calling `open_scene()`, the Editor opens the ImGuiFileDialog. The dialog runs multi-frame. On selection, `open_scene(selected_path)` is called.
    > 
    > The implementation must NOT try to show the save-prompt and the file dialog in the same modal. The pending_op_ state machine only covers the save-prompt. The file dialog is handled separately in `draw_ui()` using ImGuiFileDialog's own `Display()`/`IsOk()` pattern.

    Therefore, add a second pending flag for the file dialog:
    ```cpp
    bool show_file_dialog_ = false;       // true = ImGuiFileDialog should open
    std::string file_dialog_action_;      // "Open" or "SaveAs"
    ```

    When save-prompt resolves for OpenScene/NewScene:
    - On Save: save first, then set `show_file_dialog_ = true`, clear `pending_op_`.
    - On Discard: set `show_file_dialog_ = true`, clear `pending_op_`.
    - On Cancel: clear `pending_op_`, do nothing.

    When the file dialog resolves:
    - On OK: call `open_scene(selected_path)` or `save_scene_as(selected_path)`.
    - On Cancel: do nothing.

28. **draw_ui() changes** — integrate into `Editor::draw_ui()`:

    After the existing Phase 4 (About popup), add:

    - **Phase 5**: `draw_pending_op_modal(ctx)` — handles save-prompt state machine.
    - **Phase 6**: `draw_file_dialog()` — handles ImGuiFileDialog display.
    - **Phase 7**: `draw_error_modals()` — handles error popups.

    The order ensures: save-prompt → file dialog → error modals all take priority.

29. **draw_pending_op_modal(be::EngineContext const& ctx)**:
    - If `pending_op_ == None` and `!show_file_dialog_` and `!show_save_prompt_modal_`: return.
    - If `pending_op_ != None` and `dirty_`:
      - Draw save-prompt modal via `draw_save_prompt_modal()`.
      - On `Save`: do save (or redirect to Save As if untitled), then execute the pending op (or open file dialog for OpenScene), clear `pending_op_`.
      - On `Discard`: execute the pending op (or open file dialog), clear `pending_op_`.
      - On `Cancel`: clear `pending_op_`, log `BUDDD_LOG_INFO("Save prompt cancelled")`.
    - If `pending_op_ != None` and `!dirty_`:
      - Execute pending op directly, clear `pending_op_`.
    - Show file dialog if `show_file_dialog_` is true:
      - Call ImGuiFileDialog::Display("ChooseFileDlgKey").
      - If `isOk()`, get selected path, call appropriate action (open_scene / save_scene_as).
      - Reset `show_file_dialog_ = false`.

### Step 5: MenuBar — menu items and callbacks (menu_bar.h)

30. Add to `MenuBar` class:

    ```cpp
    // ── Scene file operation callbacks ──
    auto set_on_new_scene(std::function<void()> callback) -> void {
        on_new_scene_ = std::move(callback);
    }
    auto set_on_open_scene(std::function<void()> callback) -> void {
        on_open_scene_ = std::move(callback);
    }
    auto set_on_save_scene(std::function<void()> callback) -> void {
        on_save_scene_ = std::move(callback);
    }
    auto set_on_save_scene_as(std::function<void()> callback) -> void {
        on_save_scene_as_ = std::move(callback);
    }
    auto set_on_quit(std::function<void(be::EngineContext const&)> callback) -> void {
        on_quit_ = std::move(callback);
    }
    ```

31. Update `draw_ui()` in `MenuBar`:

    Replace the existing `File` menu block with:

    ```
    File menu:
      "New Scene"    Ctrl+N     → call on_new_scene_()
      "Open Scene"   Ctrl+O     → call on_open_scene_()
      separator
      "Save Scene"   Ctrl+S     → call on_save_scene_()
      "Save Scene As" Ctrl+Shift+S → call on_save_scene_as_()
      separator
      "Quit"         Ctrl+Q     → call on_quit_(ctx) instead of directly calling ctx.request_exit()
    ```

    > **Note**: Remove the direct `ctx.request_exit()` call from the File > Quit menu item. The Quit callback now invokes the registered handler (which checks dirty state and shows save-prompt).

32. Add private members to `MenuBar`:
    ```cpp
    std::function<void()> on_new_scene_;
    std::function<void()> on_open_scene_;
    std::function<void()> on_save_scene_;
    std::function<void()> on_save_scene_as_;
    std::function<void(be::EngineContext const&)> on_quit_;
    ```



### Step 6: Editor setup — integrate scene management and shortcut changes

33. In `Editor::setup()`, after creating the MenuBar:

    ```cpp
    auto menu_bar = std::make_unique<MenuBar>(command_stack_);
    menu_bar->set_on_about([this]() { show_about_ = true; });
    menu_bar->set_on_new_scene([this]() {
        if (dirty_) {
            pending_op_ = PendingOp::NewScene;
        } else {
            new_scene();
        }
    });
    menu_bar->set_on_open_scene([this]() {
        if (dirty_) {
            pending_op_ = PendingOp::OpenScene;
        } else {
            show_file_dialog_ = true;
            file_dialog_action_ = "Open";
        }
    });
    menu_bar->set_on_save_scene([this]() {
        // save_scene() returns error if untitled → fallback to file dialog
        auto result = save_scene();
        if (!result) {
            // Untitled or error: open Save As dialog
            show_file_dialog_ = true;
            file_dialog_action_ = "SaveAs";
        }
    });
    menu_bar->set_on_save_scene_as([this]() {
        show_file_dialog_ = true;
        file_dialog_action_ = "SaveAs";
    });
    menu_bar->set_on_quit([this](be::EngineContext const& ctx) {
        if (dirty_) {
            pending_op_ = PendingOp::Quit;
        } else {
            ctx.request_exit();
        }
    });
    add_menu(std::move(menu_bar));
    ```

34. Update shortcut bindings in `Editor::setup()`:

    The existing Ctrl+Q shortcut:
    ```cpp
    shortcuts_.bind(be::KeyCode::Q, {.ctrl = true}, [this](be::EngineContext const& ctx) {
        if (dirty_) {
            pending_op_ = PendingOp::Quit;
        } else {
            ctx.request_exit();
        }
    });
    ```

    Add shortcuts:
    ```cpp
    shortcuts_.bind(be::KeyCode::N, {.ctrl = true}, [this](be::EngineContext const&) {
        if (dirty_) {
            pending_op_ = PendingOp::NewScene;
        } else {
            new_scene();
        }
    });
    shortcuts_.bind(be::KeyCode::O, {.ctrl = true}, [this](be::EngineContext const&) {
        if (dirty_) {
            pending_op_ = PendingOp::OpenScene;
        } else {
            show_file_dialog_ = true;
            file_dialog_action_ = "Open";
        }
    });
    shortcuts_.bind(be::KeyCode::S, {.ctrl = true}, [this](be::EngineContext const&) {
        auto result = save_scene();
        if (!result) {
            show_file_dialog_ = true;
            file_dialog_action_ = "SaveAs";
        }
    });
    shortcuts_.bind(be::KeyCode::S, {.ctrl = true, .shift = true}, [this](be::EngineContext const&) {
        show_file_dialog_ = true;
        file_dialog_action_ = "SaveAs";
    });
    ```

35. Add `#include <ImGuiFileDialog.h>` at the top of `editor.cpp` for the ImGuiFileDialog API. Wrap in `#ifdef` guards if needed (ImGuiFileDialog is always available when building the editor).

36. Implement the file dialog integration in a `draw_file_dialog()` method:
    - Use `ImGuiFileDialog::Instance()` singleton.
    - Call `OpenDialog("ChooseFileDlgKey", "Choose File", "\.yaml", ".", 1, nullptr, ImGuiFileDialogFlags_None)` when `show_file_dialog_` transitions from false to true.
    - Call `Display("ChooseFileDlgKey")` every frame.
    - On `isOk()`:
      - If `file_dialog_action_ == "Open"`: call `open_scene(file_path_name)`.
      - If `file_dialog_action_ == "SaveAs"`: call `save_scene_as(file_path_name)`.
    - On cancel: no action.
    - Reset `show_file_dialog_ = false` after resolution.

37. In `Editor::setup()`, after setup succeeds, call `update_window_title()` to set the initial title to `"Untitled — Buddd Editor"`.

### Step 7: Editor lifecycle — integration notes

38. **Editor::shutdown()**: No changes needed beyond the existing implementation. The World is destroyed by the unique_ptr in the destructor. No auto-save is performed on shutdown — data loss on unexpected exit is acceptable (user is expected to save explicitly).

39. **OS close button (X / Alt+F4)**: The OS close button triggers the same dirty save-prompt as File > Quit. This is achieved via the engine close-event hook (see Step 8 below). When the user clicks X or presses Alt+F4:
    - The OS sends `SDL_EVENT_QUIT` to `PlatformSDL3::poll_events()`.
    - The close-request callback (registered by the Editor in `Editor::setup()`) is invoked.
    - The callback shows the save-prompt modal (implemented in `draw_pending_op_modal()` via a new `PendingOp::Close` enum value or by reusing the `PendingOp::Quit` flow).
    - Save → call `save_scene()`, return `true` only if save succeeded (allowing close).
    - Don't Save → return `true` (allow close, no save).
    - Cancel → return `false` (abort close — the quit event is swallowed, window stays open).
    - If the callback returns `false`, `poll_events()` continues returning `true` (the quit event is consumed without causing exit).
    - If the callback returns `true` (or no callback is registered), `poll_events()` returns `false` and the render loop exits normally.
    - In headless mode, no window exists so this mechanism is a no-op (`PlatformHeadless::poll_events()` already returns `true` indefinitely).

### Step 8: Platform close-event hook

The engine-level close-event hook allows the Editor to intercept OS window close requests (X button / Alt+F4) and show the save-prompt before the render loop exits.

**Platform base class** (`src/engine/platform/platform.h`):

40. Add a `set_on_close_request` method and a close-request callback member:
    ```cpp
    // In Platform class declaration (public section, after poll_events):
    /// Register a callback invoked when the platform receives a quit/close request.
    /// The callback returns true to allow the close, false to cancel it.
    /// If no callback is registered, the close proceeds normally.
    auto set_on_close_request(std::function<bool()> callback) -> void {
        close_request_callback_ = std::move(callback);
    }

    // In protected section:
    std::function<bool()> close_request_callback_;
    ```
    - `set_on_close_request` is a **concrete** (non-virtual) method on the base class — no override needed in subclasses.
    - The callback is stored in a `protected` member so that subclasses can access it in `poll_events()`.

41. Add `#include <functional>` to the includes in `platform.h` for `std::function`.

**PlatformSDL3::poll_events()** (`src/engine/platform/platform_sdl3.cpp`):

42. Change the `SDL_EVENT_QUIT` handler from returning `false` immediately to checking the close-request callback:
    ```cpp
    if (event.type == SDL_EVENT_QUIT) {
        if (close_request_callback_ && !close_request_callback_()) {
            // Callback returned false: cancel the close, swallow the event
            continue;   // skip return false, continue polling
        }
        return false;   // callback returned true or not set: allow close
    }
    ```
    - If a callback is registered and returns `false`, `poll_events()` does NOT return `false` — it continues the event loop (`continue`), effectively swallowing the quit event.
    - If no callback is registered, or the callback returns `true`, `poll_events()` returns `false` as before (normal exit).

**PlatformHeadless::poll_events()**: No changes needed. Headless mode has no window to close; `poll_events()` already returns `true` indefinitely. The `close_request_callback_` member exists on the base class but is never invoked by `PlatformHeadless`.

**Editor integration** (`src/editor/editor.cpp`):

43. In `Editor::setup()`, after setup succeeds and `update_window_title()` is called, register the close-request handler:
    ```cpp
    ctx.services.platform().set_on_close_request([this]() -> bool {
        if (!dirty_) {
            return true;  // clean scene: allow close
        }
        // Dirty scene: show save-prompt by setting pending_op_
        // The save-prompt will be drawn in the next draw_ui() frame.
        // We need a blocking approach: poll_events() is called from the render loop
        // BEFORE draw_ui(), so we cannot show the modal here.
        // Instead, set a flag that draw_ui() will process, and return false
        // to cancel the close temporarily. The actual close will be re-attempted
        // after the user resolves the save-prompt.
        pending_op_ = PendingOp::Quit;
        return false;  // cancel close — we'll re-request exit after user resolves
    });
    ```

44. The `PendingOp::Quit` resolution in `draw_pending_op_modal()` (from Step 4, item 27/29) must be updated:
    - On Save (successful): call `ctx.request_exit()` after saving.
    - On Discard: call `ctx.request_exit()`.
    - On Cancel: clear `pending_op_` (do nothing — the close was cancelled, the editor stays open).
    
    The `PendingOp::Quit` flow already does this from the shortcut/menu handler. The close-request callback initiates the same flow by setting `pending_op_ = PendingOp::Quit` and returning `false` (to prevent the immediate exit). When the user resolves the save-prompt, the pending op execution calls `ctx.request_exit()`.

    **Important detail**: The close-request callback runs inside `poll_events()`, which is called from the render loop's main thread before `draw_ui()`. Returning `false` from the callback causes `poll_events()` to swallow the quit event, so the loop continues. On the next frame, `draw_ui()` sees `pending_op_ = Quit` and `dirty_ = true`, and shows the save-prompt modal. After the user resolves it:
    - Save/Discard → calls `ctx.request_exit()` → next frame's `poll_events()` will see `is_exit_requested()` and the loop will exit cleanly.
    - Cancel → clears `pending_op_`, no exit — the close has been aborted and the editor continues running.

45. **Thread safety**: The close-request callback is called from the main thread (the same thread that calls `poll_events()` and `draw_ui()`), so no synchronization is needed. The callback sets `pending_op_` which is read in `draw_ui()` on the same thread.

### Step 9: ImGuiFileDialog file dialog details

46. **ImGuiFileDialog integration pattern** (in `Editor::draw_ui()`):
    - Before drawing the save-prompt or error modals, check `show_file_dialog_`.
    - On the first frame where `show_file_dialog_` becomes true:
      ```cpp
      IGFD::FileDialogConfig config;
      config.path = ".";
      config.filePathName = "";
      config.countSelectionMax = 1;
      config.flags = ImGuiFileDialogFlags_None;
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey",
          "Choose File", "\.yaml", config);
      ```
    - Every frame while dialog is open, call:
      ```cpp
      if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
          if (ImGuiFileDialog::Instance()->IsOk()) {
              std::string file_path = ImGuiFileDialog::Instance()->GetFilePathName();
              // action based on file_dialog_action_
          }
          ImGuiFileDialog::Instance()->Close();
          show_file_dialog_ = false;
      }
      ```

47. **Error handling for file dialog**: If `ImGuiFileDialog::Instance()` returns an empty path (edge case from spec), treat as cancellation — no action.



### Step 10: Clean save on clean scene

48. **Save on clean scene with path**: If `!dirty_` and `current_file_path_` has a value, `save_scene()` should be a no-op. Check `if (!dirty_ && current_file_path_.has_value()) return {};` at the start of `save_scene()`. No dialog, no error, dirty_ remains false. If `!dirty_` but untitled (no file path), do NOT return early — let the method reach the file-path null check which returns an error (triggering Save As redirect from the caller). See also the edge case table below.

## Required tests

All tests must follow the existing conventions in `tests/editor_tests.cpp`:
- Catch2 v3 macros (`TEST_CASE`, `REQUIRE`, `SECTION`)
- `[editor]` tag
- Headless engine via `EngineService::create(Backend::Headless, ...)`
- No ImGui dependency (tests exercise logic, not modal rendering)

### Unit tests

| ID | What to test | Verification | Spec AC |
|---|---|---|---|
| UT-01 | **Dirty state**: construct Editor, verify `is_dirty()` is false, call `mark_dirty()`, verify `is_dirty()` is true, call `clear_dirty()`, verify false. | `REQUIRE_FALSE(editor.is_dirty())` → `editor.mark_dirty()` → `REQUIRE(editor.is_dirty())` → `editor.clear_dirty()` → `REQUIRE_FALSE(editor.is_dirty())` | AC-04 |
| UT-02 | **Window title**: construct Editor, verify `build_title_string()` returns `"Untitled — Buddd Editor"`. Mark dirty → verify returns `"Untitled* — Buddd Editor"`. Set file path to `"/path/scene.yaml"`, clear dirty → verify `"scene.yaml — Buddd Editor"`. Mark dirty → verify `"scene.yaml* — Buddd Editor"`. | `REQUIRE(editor.build_title_string() == "Untitled — Buddd Editor")` then mutate state and recheck. | AC-04 |
| UT-03 | **Untitled scene**: construct Editor, verify `current_file_path()` is nullopt, title is "Untitled". | `REQUIRE_FALSE(editor.current_file_path().has_value())` | AC-06 |
| UT-04 | **New scene**: create Editor with entities, call `new_scene()`, verify world entity count is 0, file path is nullopt, dirty is false. | `editor.world().add_entity()` → `REQUIRE(editor.world().entity_count() > 0)` → `editor.new_scene()` → `REQUIRE(editor.world().entity_count() == 0)` → `REQUIRE_FALSE(editor.current_file_path().has_value())` → `REQUIRE_FALSE(editor.is_dirty())` | AC-09 |
| UT-05 | **New scene with dirty**: mark dirty, call `new_scene()`, verify dirty cleared, world empty. | Same as UT-04 but with `editor.mark_dirty()` before `new_scene()`. | AC-09 |
| UT-06 | **Save on clean scene**: verify `save_scene()` returns success immediately when `!dirty_` and file path is set. | Set file path, save, then save again (clean) → verify second save returns success (no-op). | Edge case |
| UT-07 | **File path tracking**: save_scene_as("path/a.yaml") → verify `current_file_path()` returns "path/a.yaml". | `editor.save_scene_as("path/a.yaml")` → `REQUIRE(editor.current_file_path().value() == "path/a.yaml")` | AC-03 |
| UT-08 | **Quit with clean scene**: verify that when `!dirty_`, the quit handler calls `ctx.request_exit()`. | Use a mock context or check the exit_requested_ flag pattern. | AC-07 |
| UT-09 | **Save redirects to Save As on untitled**: verify `save_scene()` returns error when untitled (both clean and dirty). | First test: clean and untitled → `REQUIRE_FALSE(editor.save_scene().has_value())`. Second test: mark dirty, untitled → `REQUIRE_FALSE(editor.save_scene().has_value())`. The method must NOT silently no-op on clean untitled. | AC-06 |
| UT-10 | **Dirty flag after failed save**: simulate save failure (e.g., write to invalid path), verify dirty is NOT cleared. | Requires creating a headless engine with registry/assets, set file path to invalid location, verify error and dirty remains true. | Error case |
| UT-11 | **World replaced by new_scene**: verify `world()` returns a valid reference after `new_scene()`. | `auto& w1 = editor.world()` → `editor.new_scene()` → `auto& w2 = editor.world()` → both valid, entity count 0. | AC-09 |
| UT-12 | **Close-request callback — clean scene**: Register a close-request callback via `Platform::set_on_close_request()`. Verify that when `!dirty_`, the callback returns `true` (allow close). | Create a headless Platform, register callback that captures a `bool` flag, verify the flag indicates `true` (allow close). Can also test via a helper that wraps Editor's close logic. | AC-07 |
| UT-13 | **Close-request callback — dirty scene**: Mark dirty, register close-request callback, verify the callback returns `false` (cancel close) and sets `pending_op_ = PendingOp::Quit`. | Mark dirty, invoke close-request logic (via public method if exposed, or by testing the lambda behavior). Verify return is `false` and `pending_op_` is set to `Quit`. | Edge case |

### Integration tests

| ID | What to test | Verification | Spec AC |
|---|---|---|---|
| IT-01 | **Round-trip save/load**: Create a headless engine with engine service. Add entities to Editor's World with components. Save via `save_scene_as` to temp path. Create a new Editor (or call `new_scene()`), load via `open_scene(temp_path)`. Verify entity count, names match. | Uses a headless EngineService for ComponentRegistry/AssetManager. Requires a temporary writeable file. Compare entity_count, root_entity_count, names. | AC-10 |
| IT-02 | **Error handling — corrupt YAML**: Write a corrupt YAML string to a temp file. Call `open_scene(corrupt_path)`. Verify error returned and world still valid (entity count unchanged from before). | `REQUIRE_FALSE(editor.open_scene(corrupt_path).has_value())` and verify world state preserved. | AC-08 |

### Display-dependent tests (under `#ifdef BUDDD_HAS_DISPLAY`)

| ID | What to test | Verification | Spec AC |
|---|---|---|---|
| DT-01 | **Window::set_title SDL3**: Create an offscreen SDL3 engine, get window, call `set_title("Test Title")`, verify no crash. | Construct engine with `Backend::SDL3` and `SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen")`. Call `window.set_title("Test")`. | A-08 |
| DT-02 | **Window title updates on dirty**: Create SDL3 engine + Editor setup, mark dirty, verify window title contains `*`. Save, verify `*` removed. | Full Editor setup with SDL3 offscreen. | AC-04 |

### Headless window test

| ID | What to test | Verification | Spec AC |
|---|---|---|---|
| HT-01 | **Window::set_title headless**: Create headless engine, call `WindowHeadless::set_title("test")`, verify no crash/no-op. | Construct engine with `Backend::Headless`, get window, call `set_title`. | A-08 |

### Test YAML fixtures

For IT-01 (round-trip), the test should programmatically create entities in the Editor's World, save to a temp file, and reload. No pre-existing test `.yaml` scene file is needed — the round-trip is self-contained.

For IT-02 (error handling), create a temp file with invalid YAML content (e.g., just `corrupt: [unclosed`).

## Edge cases

The implementation must handle the following edge cases (from spec + additional):

| Edge case | Required behavior |
|---|---|
| **Save on clean scene with file path** | `save_scene()` is a no-op when `!dirty_` and `current_file_path_` has a value. Returns success. No dialog. |
| **Save on clean untitled scene** | `save_scene()` returns error (no file path set — same as dirty untitled). Caller opens Save As dialog. Not a silent no-op. |
| **Save As on scene with existing file path** | Opens ImGuiFileDialog regardless. After saving, updates `current_file_path_` to the new path. |
| **Open Scene with clean scene** | No save-prompt. File dialog opens immediately. |
| **New Scene with clean scene** | No save-prompt. World cleared immediately. |
| **Quit with clean scene** | No save-prompt. `ctx.request_exit()` called immediately. |
| **Cancel file dialog (Open)** | No action. Current scene unchanged. No error. |
| **Cancel file dialog (Save As)** | No action. Current file path unchanged. Dirty state unchanged. |
| **ImGuiFileDialog returns empty path** | Treat as cancellation. No action. |
| **Open corrupt YAML** | Error returned from `open_scene()`. Previous World preserved. Error modal shown. |
| **Save to invalid path (permissions, disk full)** | Error returned from `save_scene()` / `save_scene_as()`. Dirty not cleared. Error modal shown. File path NOT updated (for Save As). |
| **Dirty flag after failed Save** | Remains dirty. |
| **Dirty flag after failed Save As** | Remains dirty. File path NOT updated. |
| **Empty World during Save** | `SceneSaver` saves an empty scene (`type: Scene, version: 1, entities: []`). Valid — saves successfully. |
| **Empty World during Open** | Opens normally — entities populated by `SceneLoader`. |
| **Engine services null during scene ops** | If `engine_` is null (should not happen after `setup()`), the methods should return an error. |
| **Editor never set up** | `dirty_`, `current_file_path_`, `world()` are all valid before `setup()`. Scene ops work on the world but `update_window_title()` is a no-op without `window_`. |
| **File path with spaces/special chars** | Stored as-is. Passed to `SceneLoader::load_from_file` / `SceneSaver::save_to_file` as-is. |
| **Window title length** | Truncated by OS/Window. Acceptable. The dirty `*` is the last char before title suffix. |
| **Multiple rapid File menu actions** | Single-threaded (ImGui frame). Each action completes before next is processed. State machine ensures clean transitions. |
| **Operation cancelled at save-prompt** | `pending_op_` cleared. No state change. Current scene unchanged. Log `BUDDD_LOG_INFO("Save prompt cancelled")`. |
| **Clean scene, operation initiated while another pending** | Cannot happen in single-threaded ImGui (one action per frame). |
| **OS close button (X / Alt+F4)** | Triggers the same save-prompt as File > Quit. Close is intercepted via `Platform::set_on_close_request()`. Save → save then close. Don't Save → discard and close. Cancel → abort close, editor stays open. |

## Security impact

- File operations use OS-level file access via `SceneLoader`/`SceneSaver` (no elevated privileges).
- `ImGuiFileDialog` uses the OS-native file dialog, inheriting the process's file permissions.
- No network access. No credentials, secrets, or sensitive data handled.
- File operations triggered by explicit user action through the File menu — no automated file I/O.
- Path traversal protection handled by OS file dialog (user selects file; arbitrary path input not accepted).
- Scene files trusted (developer/artist-generated content).

## Data and migration impact

None. No schema changes, no data migrations, no seed data, no data loss risks. Scene files are user-created `.yaml` files on disk. No cross-session state is stored by the Editor (file path is runtime-only).

## API compatibility impact

| API | Change | Impact |
|---|---|---|
| `buddd::engine::Window` | Added `virtual auto set_title(std::string title) -> void = 0;` | Breaking change for any existing `Window` subclass not in this repo (none). All three existing subclasses (`WindowSDL3`, `WindowHeadless`) are updated. |
| `buddd::editor::Editor` | Added public methods: `mark_dirty()`, `clear_dirty()`, `is_dirty()`, `current_file_path()`, `new_scene()`, `open_scene()`, `save_scene()`, `save_scene_as()` | Extensions — no breaking changes. All existing consumers compile without changes. |
| `buddd::editor::MenuBar` | Added callbacks: `set_on_new_scene`, `set_on_open_scene`, `set_on_save_scene`, `set_on_save_scene_as`, `set_on_quit` | Extensions — no breaking changes. |
| `buddd::engine::Platform` | Added `auto set_on_close_request(std::function<bool()>) -> void` concrete method and `close_request_callback_` protected member. | Non-breaking: existing subclasses compile unchanged (no new pure virtual). The new method is concrete, so existing code linking against `Platform` is ABI-compatible for non-virtual calls. |
| `buddd::engine::PlatformSDL3` | `poll_events()` now checks `close_request_callback_` before returning `false` on `SDL_EVENT_QUIT`. | Behavioral change: OS close button now invokes the callback instead of immediately exiting. Backward-compatible: if no callback is registered, behavior is identical to before (immediate exit). |
| `buddd::engine::EngineService` | No changes. `registry()` and `assets()` are already public. | None. |
| Shortcut callbacks | Ctrl+Q callback changed from directly calling `ctx.request_exit()` to checking dirty state first. | Behavioral change: Ctrl+Q now shows save-prompt if dirty. Backward-compatible for clean-scene use case (exits immediately). |
| Close-request flow | OS close button now goes through the same save-prompt state machine as File > Quit. If user cancels, the close is aborted. | The `PendingOp::Quit` flow now handles both File > Quit AND OS close button uniformly. |

## Documentation impact

- **README**: no changes.
- **Wiki pages**: `docs/wiki/editor/scene-management.md` — update from "future vision" to "current implementation" for F-01 operations (New/Open/Save/Save As/Quit with dirty state). Correct north-star section from dirty-by-default to clean-by-default.
- **Wiki pages**: `docs/wiki/architecture/module-map.md` — add `ImGuiFileDialog` dependency to `buddd_editor` module. Add Editor scene management methods. Note the `Platform::set_on_close_request()` addition for OS close interception.
- **Wiki pages**: `docs/wiki/editor/scene-management.md` — update the OS close button section from "known limitation" to "fully supported — same save-prompt as File > Quit via engine close-event hook."
- **Other specs**: `.specs/sprint-2026-06/editor-ux-design/spec.md` — add note that F-01 implements AC-015 through AC-018 and AC-025, and that AC-015/Story 1 need correction to match clean-by-default.
- **ADR**: `docs/adr/ADR-029-editor-ux-decisions.md` — update AC-015 reference to match clean-by-default decision.

## ADR impact

No new ADR needed. The decisions in this contract are consistent with existing ADRs (ADR-019, ADR-027, ADR-029). The `Window::set_title()` addition is a straightforward extension of the existing `Window` abstraction (ADR-019). The `Platform::set_on_close_request()` addition follows the same engine-extension pattern and is backward-compatible (concrete method on base class). The use of ImGuiFileDialog is consistent with ADR-026 (Dear ImGui Integration). The clean-by-default behavior was confirmed by Q-04 in the spec grilling and does not contradict any accepted ADR.

The OS close button interception now matches the spec's G-05 requirement — same save-prompt as File > Quit. This resolves the previous known limitation and fully satisfies spec edge case for OS window close.

## Done criteria

The implementation is complete when all of the following are verifiable:

1. [ ] **Window::set_title** — `Window` base class has `virtual auto set_title(std::string title) -> void = 0;`. WindowSDL3 implements via `SDL_SetWindowTitle`. WindowHeadless implements as no-op. Verify by reading source files.

2. [ ] **ImGuiFileDialog integrated** — Root `CMakeLists.txt` has FetchContent for ImGuiFileDialog. `src/editor/CMakeLists.txt` links ImGuiFileDialog to `buddd_editor`. Verify by reading CMake files.

3. [ ] **Editor class has scene management** — `editor.h` declares `mark_dirty()`, `clear_dirty()`, `is_dirty()`, `current_file_path()`, `new_scene()`, `open_scene()`, `save_scene()`, `save_scene_as()`, `build_title_string()`, `update_window_title()`, `draw_save_prompt_modal()`, and state machine members (`pending_op_`, `show_file_dialog_`, etc.). Verify by reading `editor.h`.

4. [ ] **MenuBar has file items** — `MenuBar` has `set_on_new_scene`, `set_on_open_scene`, `set_on_save_scene`, `set_on_save_scene_as`, `set_on_quit` callbacks. File menu shows all 6 items (New, Open, separator, Save, Save As, separator, Quit) with correct shortcut labels (`Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`, `Ctrl+Q`). Verify by reading `menu_bar.h`.

5. [ ] **Shortcuts bound** — `Editor::setup()` binds `Ctrl+N`, `Ctrl+O`, `Ctrl+S`, `Ctrl+Shift+S`, and updates `Ctrl+Q` to check dirty state. Verify by reading `editor.cpp`.

6. [ ] **Initial title** — On launch, window title is `"Untitled — Buddd Editor"`. Verify by reading `editor.cpp` setup path calling `update_window_title()`.

7. [ ] **Dirty `*` in title** — `mark_dirty()` adds `*` to title, `clear_dirty()` / save removes it. Verify by UT-01 and UT-02.

8. [ ] **Save-prompt state machine** — When dirty and New/Open/Quit is triggered, `pending_op_` is set. `draw_ui()` draws save-prompt modal. Save/Discard/Cancel buttons produce correct side effects. Verify by reading `editor.cpp` implementation of `draw_pending_op_modal()`.

9. [ ] **Error modals** — `show_error_modal()` sets flags. `draw_error_modals()` renders ImGui popup. Verify by reading `editor.cpp`.

10. [ ] **All existing tests pass** — Run `ctest` (or build and run `buddd_tests`). All 508+ existing tests still pass with no regressions. Verify by running the test suite.

11. [ ] **All new tests pass** — Run `ctest`. New F-01 tests pass. Verify by running the test suite. At minimum:
    - UT-01 through UT-13 pass.
    - IT-01 (round-trip) and IT-02 (error handling) pass.
    - HT-01 (headless set_title) passes.
    - DT-01 and DT-02 (if display available) pass.

12. [ ] **Zero warnings from `src/` and `tests/`** — Build with `-Wall -Wextra -Wpedantic` (or project's default warning flags) produces zero warnings from changed or new files. Verify by building.

13. [ ] **Engine core changes are limited to Window::set_title and Platform::set_on_close_request** — `git diff` shows no modifications to `SceneLoader`, `SceneSaver`, `World`, `Entity`, `ComponentRegistry`, `AssetManager`, `EditorApp`, or `run_app()`. Changes to `Platform` are limited to the new `set_on_close_request()` method and member. Changes to `PlatformSDL3` are limited to the `SDL_EVENT_QUIT` handler. Verify by `git diff --stat`.

14. [ ] **ADR-019 compliance** — No `<SDL3/`, `<GL/`, or `<glm/` headers included in `src/editor/`. Verify by `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/`.

15. [ ] **Logging implemented** — Verify log calls exist for: scene saved, scene loaded, load/save failures, mark dirty, dirty cleared, save-prompt results.

16. [ ] **OS close button fully implemented** — OS close button (X / Alt+F4) triggers the same save-prompt as File > Quit. Verify by reading:
    - `src/engine/platform/platform.h` — contains `set_on_close_request()` and `close_request_callback_` member.
    - `src/engine/platform/platform_sdl3.cpp` — `SDL_EVENT_QUIT` handler checks `close_request_callback_` before returning `false`.
    - `src/editor/editor.cpp` — `Editor::setup()` registers close-request handler via `ctx.services.platform().set_on_close_request(...)`.
    - If callback returns `false`, the quit event is swallowed and the render loop continues.
    - If callback returns `true` or no callback registered, `poll_events()` returns `false` as before.

17. [ ] **Close-request callback behavior** — The Editor's close-request handler:
    - If scene is clean (`!dirty_`), returns `true` immediately (allow close, no prompt).
    - If scene is dirty, sets `pending_op_ = PendingOp::Quit` and returns `false` (cancel close temporarily). The save-prompt modal appears in the next `draw_ui()` frame.
    - On Save/Discard: calls `ctx.request_exit()` after save-prompt resolves.
    - On Cancel: clears `pending_op_`, no exit — editor stays open.
    Verify by reading `editor.cpp` in `Editor::setup()`. UT-12 (clean scene → allow close) and UT-13 (dirty scene → cancel close, set pending_op_) must pass.

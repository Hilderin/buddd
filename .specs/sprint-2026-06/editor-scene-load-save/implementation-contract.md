# IMPL-F-01 — Editor Scene Load/Save Integration (SDL3 Native Dialogs — Simplified Callback Design)

## Source spec

`.specs/sprint-2026-06/editor-scene-load-save/spec.md`

## Goal

Wire the existing engine SceneLoader/SceneSaver APIs into the Editor's File menu, establishing the fundamental save/open workflow that all subsequent editor features depend on. This includes extending the File menu with New/Open/Save/Save As/Quit, adding dirty state tracking with a `*` indicator in the window title, integrating SDL3 native file dialogs via the Platform abstraction using a simplified direct-callback design (no mutex, no intermediate result queue), showing save-prompt modals when the user attempts to close/open/create a scene with unsaved changes, and displaying error modals on SceneLoader/SceneSaver failures.

## Non-goals

- No changes to `SceneLoader`, `SceneSaver`, `World`, `Entity`, `ComponentRegistry`, `AssetManager`, or any engine core type beyond `Window::set_title()` and `Platform` dialog methods.
- No changes to existing editor panels beyond `MenuBar`.
- No changes to `EditorApp` or `run_app()`.
- No entity-level dirty tracking — simple scene-level boolean only.
- No prefab tab saving, Play mode interaction, autosave, recent files, multi-tab dirty tracking, or cross-session file path persistence.
- No "Revert" or "Reload" operations.
- No changes to ImGui initialization, docking layout, or panel lifecycle.
- No SDL3 headers in `src/editor/` (ADR-019) — the Platform abstraction defines `FileDialogCallback` so editor code never touches SDL3 dialog types.
- **No thread-safety mechanism for dialog callbacks** — the SDL3 dialog callback fires on the main thread during `SDL_PumpEvents` (called from `poll_events()`), so no mutex, atomic, or result queue is needed.
- **No intermediate dialog-result queue in Editor** — Platform dialog callbacks directly invoke `open_scene()` or `save_scene_as()`; there is no `pending_dialog_result_` or `pending_dialog_action_` pattern.
- No new external dependencies — SDL3 dialog APIs (`<SDL3/SDL_dialog.h>`) are already part of the existing SDL3 dependency.

## Relevant ADRs

| ADR | How it constrains implementation |
|---|---|
| ADR-019 (Architecture Boundaries) | No SDL3/OpenGL/GLM headers may appear in `src/editor/`. The Platform abstraction must define its own `FileDialogCallback` type so editor code never includes `<SDL3/SDL_dialog.h>`. PlatformSDL3 code in `src/engine/platform/` MAY include SDL3 headers. |
| ADR-027 (Editor Architecture) | Editor is a static library (`buddd_editor`) that links `buddd_engine` as PUBLIC. All engine access goes through `EngineContext`/`EngineService`. Architecture boundary extends to `src/editor/`. |
| ADR-029 (Editor UX Decisions) | One-scene-at-a-time model (Decision 2). Scene tab is always present. Save-prompt modal matches the about-popup pattern for multi-frame modals. |
| ADR-001 (Result Error Pattern) | SceneLoader/SceneSaver return `Result<void>`. Error propagation must use the existing `Result<T>` pattern. |
| ADR-026 (Dear ImGui Integration) | ImGui docking branch. Init failure is fatal in display mode. No ImGuiFileDialog dependency. |

## Files to inspect

The Code Agent must read these files before making any edits:

| File | What to look for |
|---|---|
| `src/engine/platform/platform.h` | Current pure virtual interface — must add `FileDialogCallback` type alias, `show_open_file_dialog()` and `show_save_file_dialog()` pure virtual methods, and `#include <optional>`. |
| `src/engine/platform/platform_sdl3.h` | `PlatformSDL3` class declaration, existing private members (`window_map_`, `input_system_`, etc.). Must add dialog method declarations and `get_sdl_window()` private helper. Note `window_map_` stores `Window*` pointers, not `SDL_Window*`. |
| `src/engine/platform/platform_sdl3.cpp` | Current `poll_events()` implementation, `SDL_EVENT_QUIT` handling, `create_window()` caching the `SDL_Window*` via `native_handle()`. Must add dialog method implementations with heap-allocated callback pattern. |
| `src/engine/platform/platform_headless.h` / `.cpp` | Headless Platform — no-op patterns. Must add dialog method declarations and implementations. |
| `src/engine/engine_service.h` | `EngineService::platform()` accessor — Editor calls `ctx.services.platform()....` for dialog methods. |
| `src/engine/window/window.h` | Current virtual interface — `set_title()` already exists from prior F-01 implementation. `native_handle()` returns `void*`. |
| `src/engine/window/window_sdl3.h` | SDL3 `Window` subclass — note `native_handle()` returns `static_cast<void*>(window_)`. |
| `src/editor/editor.h` | Current state: `show_file_dialog_`, `file_dialog_action_`, `draw_file_dialog()`, `handle_dirty_before_op()`, `show_save_prompt_modal_`, `save_prompt_result_` — all must be removed. |
| `src/editor/editor.cpp` | Current state: `draw_file_dialog()`, `#include <ImGuiFileDialog.h>`, `draw_pending_op_modal()` using `show_file_dialog_`/`file_dialog_action_` flags. All must be replaced with direct Platform dialog calls. |
| `src/editor/panels/menu_bar.h` | File menu layout — callbacks stay the same, only their implementations change. |
| `CMakeLists.txt` (root) | Current FetchContent section for ImGuiFileDialog — must be removed. |
| `src/editor/CMakeLists.txt` | Current `ImGuiFileDialog.cpp` source and include path — must be removed. |
| `tests/editor_tests.cpp` | Test conventions: `HeadlessTestContext` setup, Catch2 macros, `[editor][f01]` tags. Tests that check `show_file_dialog_` or `file_dialog_action_` must be removed. |
| `<SDL3/SDL_dialog.h>` (system header) | SDL3 dialog API: `SDL_ShowOpenFileDialog`, `SDL_ShowSaveFileDialog`, `SDL_DialogFileFilter`, `SDL_DialogFileCallback`. |

## Files allowed to change

1. `src/engine/platform/platform.h` — add `#include <optional>`, `FileDialogCallback` type alias, pure virtual `show_open_file_dialog()` and `show_save_file_dialog()`.
2. `src/engine/platform/platform_sdl3.h` — add dialog method declarations, `get_sdl_window()` private helper, `#include <SDL3/SDL_dialog.h>`.
3. `src/engine/platform/platform_sdl3.cpp` — implement dialog methods using SDL3 native APIs with heap-allocated callback pattern (no mutex, no intermediate queue).
4. `src/engine/platform/platform_headless.h` — add dialog method declarations.
5. `src/engine/platform/platform_headless.cpp` — implement dialog methods as no-ops (immediate callback with `std::nullopt`).
6. `CMakeLists.txt` (root) — **remove** ImGuiFileDialog FetchContent block (keep Catch2 block).
7. `src/editor/CMakeLists.txt` — **remove** `ImGuiFileDialog.cpp` from source list, **remove** ImGuiFileDialog include directory.
8. `src/editor/editor.h` — **remove** `show_file_dialog_`, `file_dialog_action_`, `draw_file_dialog()`, `show_save_prompt_modal_`, `save_prompt_result_`, `handle_dirty_before_op()`. **Remove** `#include <ImGuiFileDialog.h>`.
9. `src/editor/editor.cpp` — **remove** `draw_file_dialog()` implementation, **remove** `#include <ImGuiFileDialog.h>`, **remove** `handle_dirty_before_op()` definition. Replace all `show_file_dialog_`/`file_dialog_action_` flag sets with direct Platform dialog calls. Update `draw_pending_op_modal()` to directly invoke Platform dialog methods instead of setting flags. Update Phase 6 in `draw_ui()` (remove ImGuiFileDialog phase).
10. `tests/editor_tests.cpp` — remove any tests that directly reference `show_file_dialog_` or `file_dialog_action_`. Add UT-14 (PlatformHeadless dialog no-op test).

## Files forbidden to change

- `src/engine/scene/scene_loader.h` / `.cpp` — no changes.
- `src/engine/scene/scene_saver.h` / `.cpp` — no changes.
- `src/engine/scene/world.h` / `.cpp` — no changes.
- `src/engine/window/window.h` / `window_sdl3.h` / `window_sdl3.cpp` / `window_headless.h` / `window_headless.cpp` — already have `set_title()`; no changes needed.
- `src/engine/window/` — no other files need changes.
- `src/cmd/apps/editor_app.h` / `.cpp` — no changes.
- `src/cmd/app.h` / `.cpp` — no changes.
- `src/editor/panels/menu_bar.h` — no changes needed (callbacks stay the same).
- Any file under `src/engine/scene/`, `src/engine/render/`, `src/engine/asset/` — no changes.
- Any existing test fixtures or test assets that are not explicitly for F-01.

## Existing conventions to follow

- **Naming**: `snake_case` for variables/methods, `PascalCase` for classes/enums. Use `auto` return type with trailing return type syntax.
- **Namespace**: All editor code in `namespace buddd::editor`. Engine types referenced as `be::TypeName` via `namespace be = buddd::engine;`.
- **Include style**: Use `#include "editor.h"` for local paths, `<imgui.h>` for system/third-party headers. Never include `<SDL3/` from editor code (ADR-019).
- **Shortcuts**: `ShortcutRegistry` with `shortcuts_.bind(KeyCode, Modifiers, callback)` in `Editor::setup()`. Callbacks receive `EngineContext const&`.
- **Callbacks**: `MenuBar` uses `std::function<void()>` callbacks set via `set_on_*` methods. No changes needed.
- **Logging**: `BUDDD_LOG_TAG("Editor")` already set in `editor.cpp`. Use `BUDDD_LOG_DEBUG`, `BUDDD_LOG_INFO`, `BUDDD_LOG_WARN` for messages with the same tag.
- **Error handling**: `Result<T>` pattern with `make_error(Error::Category, message)`. Return `Result<void>` and check `.has_value()`.
- **ImGui popups**: `ShowAboutPopup` pattern — flag `show_about_` + `draw_about_popup()`. Open with `ImGui::OpenPopup()`, render with `BeginPopupModal`, use `ImGuiWindowFlags_AlwaysAutoResize`.
- **Test conventions**: Catch2 v3, `TEST_CASE("name", "[editor][tag]")`. Headless engine via `EngineService::create(Backend::Headless, ...)`. `#ifdef BUDDD_HAS_DISPLAY` guard for display-dependent tests.
- **World access**: `Editor::world()` returns a valid `World&` at all times. The world is created in the constructor and destroyed in the destructor.
- **EngineContext::request_exit()**: Only called from shortcuts, menu callbacks, or `draw_ui`/`update` — not from constructors, setup, or async callbacks. Use the `request_exit_next_frame_` flag for callbacks that fire outside `draw_ui()`.
- **Platform interface patterns**: Concrete (non-virtual) methods for simple setters (`set_on_close_request`), pure virtual for polymorphic behavior (`poll_events`). Dialog methods are pure virtual since each backend behaves differently.

## Required implementation behavior

### Step 1: Platform dialog abstraction (`src/engine/platform/platform.h`)

1. Add `#include <optional>` to the existing includes in `platform.h` (alongside existing `#include <functional>`).

2. Add a `FileDialogCallback` type alias in the `buddd::engine` namespace (before the `Platform` class declaration):

   ```cpp
   /// Callback invoked when a native file dialog completes.
   /// filepath is the selected path, or std::nullopt if cancelled/error.
   /// The callback is invoked on the main thread during poll_events().
   using FileDialogCallback = std::function<void(std::optional<std::string> filepath)>;
   ```

3. Add two pure virtual methods to the `Platform` class (after `delta_time()`, before the deleted copy/move operators):

   ```cpp
   /// Show a native "Open File" dialog (non-blocking).
   /// callback is invoked on the main thread (during poll_events()) when
   /// the user selects a file or cancels.
   virtual auto show_open_file_dialog(FileDialogCallback callback,
                                       const char* filter_name,
                                       const char* filter_pattern) -> void = 0;

   /// Show a native "Save File" dialog (non-blocking).
   /// callback is invoked on the main thread (during poll_events()) when
   /// the user selects a file or cancels.
   /// default_name is the suggested file name (may be nullptr).
   virtual auto show_save_file_dialog(FileDialogCallback callback,
                                       const char* filter_name,
                                       const char* filter_pattern,
                                       const char* default_name) -> void = 0;
   ```

### Step 2: PlatformSDL3 implementation (`platform_sdl3.h` + `platform_sdl3.cpp`)

4. In `src/engine/platform/platform_sdl3.h`:

   - Add `#include <SDL3/SDL_dialog.h>` to includes (it may not be transitively included by `<SDL3/SDL.h>`). Add it explicitly.
   - Add dialog method declarations to the public section:

     ```cpp
     auto show_open_file_dialog(FileDialogCallback callback,
                                const char* filter_name,
                                const char* filter_pattern) -> void override;

     auto show_save_file_dialog(FileDialogCallback callback,
                                const char* filter_name,
                                const char* filter_pattern,
                                const char* default_name) -> void override;
     ```

   - Add a private helper method:

     ```cpp
     /// Get the first available SDL_Window* for use as dialog parent.
     /// Returns nullptr if no window exists.
     [[nodiscard]] auto get_sdl_window() -> SDL_Window*;
     ```

5. In `src/engine/platform/platform_sdl3.cpp`:

   - **`get_sdl_window()`** implementation:

     ```cpp
     auto PlatformSDL3::get_sdl_window() -> SDL_Window* {
         for (auto& [id, win] : window_map_) {
             return static_cast<SDL_Window*>(win->native_handle());
         }
         return nullptr;
     }
     ```

   - **`show_open_file_dialog()`** implementation:

     ```cpp
     auto PlatformSDL3::show_open_file_dialog(FileDialogCallback cb,
                                               const char* filter_name,
                                               const char* filter_pattern) -> void {
         SDL_DialogFileFilter filter{filter_name, filter_pattern};
         // Heap-allocate the callback; the SDL C-lambda deletes it after invocation.
         auto* cb_ptr = new FileDialogCallback(std::move(cb));
         SDL_ShowOpenFileDialog(
             [](void* userdata, const char* const* filelist, int /*filter_index*/) {
                 auto* cb = static_cast<FileDialogCallback*>(userdata);
                 if (filelist && filelist[0]) {
                     (*cb)(std::string(filelist[0]));
                 } else {
                     (*cb)(std::nullopt);
                 }
                 delete cb;
             },
             cb_ptr,
             get_sdl_window(),
             &filter, 1,
             nullptr,   // default_location
             false      // allow_many
         );
     }
     ```

     **Thread safety**: The SDL3 dialog callback fires on the main thread (during `SDL_PumpEvents`/`SDL_PollEvent`). The heap-allocated `FileDialogCallback` is only accessed from the C-lambda on the main thread, then deleted. No mutex or synchronization is needed. The `SDL_DialogFileFilter` struct is stack-allocated; SDL3 copies the filter data internally, so it does not need to persist after `SDL_ShowOpenFileDialog` returns.

   - **`show_save_file_dialog()`** implementation — same pattern but uses `SDL_ShowSaveFileDialog` and passes `default_name`:

     ```cpp
     auto PlatformSDL3::show_save_file_dialog(FileDialogCallback cb,
                                               const char* filter_name,
                                               const char* filter_pattern,
                                               const char* default_name) -> void {
         SDL_DialogFileFilter filter{filter_name, filter_pattern};
         auto* cb_ptr = new FileDialogCallback(std::move(cb));
         SDL_ShowSaveFileDialog(
             [](void* userdata, const char* const* filelist, int /*filter_index*/) {
                 auto* cb = static_cast<FileDialogCallback*>(userdata);
                 if (filelist && filelist[0]) {
                     (*cb)(std::string(filelist[0]));
                 } else {
                     (*cb)(std::nullopt);
                 }
                 delete cb;
             },
             cb_ptr,
             get_sdl_window(),
             &filter, 1,
             default_name
         );
     }
     ```

   - **No changes to `poll_events()`** — the SDL dialog callback fires during `SDL_PollEvent` (which is already called in the existing `while (SDL_PollEvent(&event))` loop). No additional draining is needed.

### Step 3: PlatformHeadless implementation (`platform_headless.h` + `platform_headless.cpp`)

6. In `src/engine/platform/platform_headless.h`:
   - Add dialog method declarations:
     ```cpp
     auto show_open_file_dialog(FileDialogCallback callback,
                                const char* filter_name,
                                const char* filter_pattern) -> void override;

     auto show_save_file_dialog(FileDialogCallback callback,
                                const char* filter_name,
                                const char* filter_pattern,
                                const char* default_name) -> void override;
     ```

7. In `src/engine/platform/platform_headless.cpp`:
   - Both methods immediately invoke the callback with `std::nullopt` (no-op — no file dialog in headless mode):
     ```cpp
     auto PlatformHeadless::show_open_file_dialog(FileDialogCallback callback,
                                                   const char* /*filter_name*/,
                                                   const char* /*filter_pattern*/) -> void {
         callback(std::nullopt);
     }

     auto PlatformHeadless::show_save_file_dialog(FileDialogCallback callback,
                                                   const char* /*filter_name*/,
                                                   const char* /*filter_pattern*/,
                                                   const char* /*default_name*/) -> void {
         callback(std::nullopt);
     }
     ```

### Step 4: Remove ImGuiFileDialog from build system

8. In root `CMakeLists.txt`: **Remove** the entire ImGuiFileDialog FetchContent block (lines between `# ImGuiFileDialog — needs to be populated before src/editor CMakeLists.txt` and `add_subdirectory(src/editor)`). Keep all other content unchanged.

9. In `src/editor/CMakeLists.txt`:
   - **Remove** `ImGuiFileDialog.cpp` from the `add_library(buddd_editor STATIC ...)` source list.
   - **Remove** the `target_include_directories(buddd_editor SYSTEM PRIVATE ${ImGuiFileDialog_SOURCE_DIR})` block.
   - Result: `buddd_editor` no longer depends on ImGuiFileDialog at the build level.

### Step 5: Editor header changes (`src/editor/editor.h`)

10. **Remove** `#include <ImGuiFileDialog.h>` — it is not included in `editor.h` directly (it was in `editor.cpp`). No action needed in `editor.h` for this.

11. **Remove** the file dialog-related members and methods:
    - `bool show_file_dialog_ = false;`
    - `std::string file_dialog_action_;`
    - `auto draw_file_dialog() -> void;` (declaration)

12. **Remove** dead members (identified in code review):
    - `bool show_save_prompt_modal_ = false;`
    - `SavePromptResult save_prompt_result_ = SavePromptResult::Cancel;`
    - `auto handle_dirty_before_op(be::EngineContext const& ctx, PendingOp op) -> bool;` (declaration)

13. **No new members needed** — the simplified callback design eliminates the need for `pending_dialog_result_`, `pending_dialog_action_`, or any intermediate queue. Platform dialog callbacks directly invoke `open_scene()`, `save_scene_as()`, or `show_error_modal()`.

    Exception: Add a `bool request_exit_next_frame_ = false;` flag to handle the case where a Platform dialog callback (which fires during `poll_events()`, outside `draw_ui()`) needs to request exit after a Save As completes during a Quit save-prompt.

    Place this after `pending_file_path_` and before `error_modal_title_`.

    ```cpp
    // ── Exit-after-save flag (set by Platform dialog callbacks, checked in draw_ui) ──
    bool request_exit_next_frame_ = false;
    ```

### Step 6: Editor implementation changes (`src/editor/editor.cpp`)

14. **Remove** `#include <ImGuiFileDialog.h>`.

15. **Remove** `handle_dirty_before_op()` definition — it is dead code.

16. **Remove** `draw_file_dialog()` — delete the entire method definition.

17. **Update menu bar callbacks** in `Editor::setup()` — replace `show_file_dialog_` / `file_dialog_action_` assignments with direct Platform dialog calls:

    **`on_new_scene` callback** (no change — still uses `pending_op_` state machine):
    ```cpp
    menu_bar->set_on_new_scene([this]() {
        if (dirty_) {
            pending_op_ = PendingOp::NewScene;
        } else {
            new_scene();
        }
    });
    ```

    **`on_open_scene` callback** — direct Platform dialog call instead of flag:
    ```cpp
    menu_bar->set_on_open_scene([this]() {
        if (dirty_) {
            pending_op_ = PendingOp::OpenScene;
        } else {
            engine_->platform().show_open_file_dialog(
                [this](std::optional<std::string> path) {
                    if (!path) return;
                    if (auto result = open_scene(*path); !result) {
                        show_error_modal("Load Error", result.error().message);
                    }
                },
                "YAML Scene", "yaml");
        }
    });
    ```

    Note: The menu bar callbacks are `std::function<void()>` (no `ctx` parameter, except `on_quit`). They access the platform through the stored `engine_` pointer (set during `setup()`). The lambda captures `this` by raw pointer — the Editor outlives all dialogs.

    **`on_save_scene` callback** — changed:
    ```cpp
    menu_bar->set_on_save_scene([this]() {
        auto result = save_scene();
        if (!result) {
            // Untitled or error: open Save As dialog
            engine_->platform().show_save_file_dialog(
                [this](std::optional<std::string> path) {
                    if (!path) return;
                    if (auto r = save_scene_as(*path); !r) {
                        show_error_modal("Save Error", r.error().message);
                    }
                },
                "YAML Scene", "yaml", "Untitled.yaml");
        }
    });
    ```

    **`on_save_scene_as` callback** — direct Platform dialog call:
    ```cpp
    menu_bar->set_on_save_scene_as([this]() {
        engine_->platform().show_save_file_dialog(
            [this](std::optional<std::string> path) {
                if (!path) return;
                if (auto r = save_scene_as(*path); !r) {
                    show_error_modal("Save Error", r.error().message);
                }
            },
            "YAML Scene", "yaml", "Untitled.yaml");
    });
    ```

    **`on_quit` callback** — no change (still uses `pending_op_` state machine):
    ```cpp
    menu_bar->set_on_quit([this](be::EngineContext const& ctx) {
        if (dirty_) {
            pending_op_ = PendingOp::Quit;
        } else {
            ctx.request_exit();
        }
    });
    ```

18. **Update shortcut callbacks** similarly — replace `show_file_dialog_` with Platform dialog calls:

    **Ctrl+N** (no change):
    ```cpp
    shortcuts_.bind(be::KeyCode::N, {.ctrl = true}, [this](be::EngineContext const&) {
        if (dirty_) {
            pending_op_ = PendingOp::NewScene;
        } else {
            new_scene();
        }
    });
    ```

    **Ctrl+O** — direct Platform dialog:
    ```cpp
    shortcuts_.bind(be::KeyCode::O, {.ctrl = true}, [this](be::EngineContext const&) {
        if (dirty_) {
            pending_op_ = PendingOp::OpenScene;
        } else {
            engine_->platform().show_open_file_dialog(
                [this](std::optional<std::string> path) {
                    if (!path) return;
                    if (auto result = open_scene(*path); !result) {
                        show_error_modal("Load Error", result.error().message);
                    }
                },
                "YAML Scene", "yaml");
        }
    });
    ```

    **Ctrl+S** — direct Platform dialog for Save As redirect:
    ```cpp
    shortcuts_.bind(be::KeyCode::S, {.ctrl = true}, [this](be::EngineContext const&) {
        auto result = save_scene();
        if (!result) {
            engine_->platform().show_save_file_dialog(
                [this](std::optional<std::string> path) {
                    if (!path) return;
                    if (auto r = save_scene_as(*path); !r) {
                        show_error_modal("Save Error", r.error().message);
                    }
                },
                "YAML Scene", "yaml", "Untitled.yaml");
        }
    });
    ```

    **Ctrl+Shift+S** — direct Platform dialog:
    ```cpp
    shortcuts_.bind(be::KeyCode::S, {.ctrl = true, .shift = true}, [this](be::EngineContext const&) {
        engine_->platform().show_save_file_dialog(
            [this](std::optional<std::string> path) {
                if (!path) return;
                if (auto r = save_scene_as(*path); !r) {
                    show_error_modal("Save Error", r.error().message);
                }
            },
            "YAML Scene", "yaml", "Untitled.yaml");
    });
    ```

19. **Update `draw_ui()` phases** — replace Phase 6 (ImGuiFileDialog) with a new Phase 6 that checks `request_exit_next_frame_`:

    ```cpp
    // ═══════════════════════════════════════════════
    // Phase 6: Exit-on-next-frame flag (set by async dialog callbacks)
    // ═══════════════════════════════════════════════
    if (request_exit_next_frame_) {
        request_exit_next_frame_ = false;
        ctx.request_exit();
    }

    // ═══════════════════════════════════════════════
    // Phase 7: Error modals
    // ═══════════════════════════════════════════════
    draw_error_modals();
    ```

    The original Phase 6 (ImGuiFileDialog `draw_file_dialog()`) is removed entirely.

### Step 7: Save-prompt state machine updates (`draw_pending_op_modal()`)

20. In `draw_pending_op_modal()`, replace `show_file_dialog_` / `file_dialog_action_` flag sets with direct Platform dialog method calls:

    ```cpp
    auto Editor::draw_pending_op_modal(be::EngineContext const& ctx) -> void {
        if (pending_op_ == PendingOp::None) return;

        if (!dirty_) {
            execute_pending_op(ctx);
            pending_op_ = PendingOp::None;
            return;
        }

        auto result = draw_save_prompt_modal();
        switch (result) {
            case SavePromptResult::Save: {
                auto save_result = save_scene();
                if (save_result.has_value()) {
                    // Save succeeded — proceed with pending operation
                    if (pending_op_ == PendingOp::OpenScene) {
                        engine_->platform().show_open_file_dialog(
                            [this](std::optional<std::string> path) {
                                if (!path) return;
                                if (auto r = open_scene(*path); !r) {
                                    show_error_modal("Load Error", r.error().message);
                                }
                            },
                            "YAML Scene", "yaml");
                    } else {
                        execute_pending_op(ctx);
                    }
                    pending_op_ = PendingOp::None;
                } else if (!current_file_path_.has_value()) {
                    // Untitled: redirect to Save As dialog, then complete pending op
                    auto original_op = pending_op_;
                    pending_op_ = PendingOp::None;
                    engine_->platform().show_save_file_dialog(
                        [this, original_op](std::optional<std::string> save_path) {
                            if (!save_path) return; // cancelled — stay on current scene
                            auto r = save_scene_as(*save_path);
                            if (!r) {
                                show_error_modal("Save Error", r.error().message);
                                return;
                            }
                            // Save succeeded — complete the original operation
                            if (original_op == PendingOp::OpenScene) {
                                engine_->platform().show_open_file_dialog(
                                    [this](std::optional<std::string> path) {
                                        if (!path) return;
                                        if (auto r = open_scene(*path); !r)
                                            show_error_modal("Load Error", r.error().message);
                                    },
                                    "YAML Scene", "yaml");
                            } else if (original_op == PendingOp::NewScene) {
                                new_scene();
                            } else if (original_op == PendingOp::Quit) {
                                request_exit_next_frame_ = true;
                            }
                        },
                        "YAML Scene", "yaml", "Untitled.yaml");
                } else {
                    show_error_modal("Save Error", save_result.error().message);
                    pending_op_ = PendingOp::None;
                }
                break;
            }
            case SavePromptResult::Discard: {
                if (pending_op_ == PendingOp::OpenScene) {
                    engine_->platform().show_open_file_dialog(
                        [this](std::optional<std::string> path) {
                            if (!path) return;
                            if (auto r = open_scene(*path); !r) {
                                show_error_modal("Load Error", r.error().message);
                            }
                        },
                        "YAML Scene", "yaml");
                } else {
                    execute_pending_op(ctx);
                }
                pending_op_ = PendingOp::None;
                break;
            }
            case SavePromptResult::Cancel: {
                pending_op_ = PendingOp::None;
                BUDDD_LOG_INFO("Save prompt cancelled");
                break;
            }
        }
    }
    ```

    **Important**: Inside the callback lambdas (which fire during `poll_events()`, not during `draw_ui()`), use `engine_->platform()` instead of `ctx.services.platform()`. The `ctx` parameter of `draw_ui()` is not captured by these lambdas and would not be valid when the callback fires asynchronously. The `engine_` pointer is set during `setup()` and is always valid while the Editor exists.

    The `execute_pending_op()` method remains unchanged (handles `NewScene` → `new_scene()`, `OpenScene` → uses `pending_file_path_` which is set in `execute_pending_op`, `Quit` → `ctx.request_exit()`). The key change is that `OpenScene` case in the save-prompt now directly opens the Platform dialog instead of setting `show_file_dialog_ = true; file_dialog_action_ = "Open";`.

### Step 8: Add `request_exit_next_frame_` initialization

21. In `Editor::shutdown()`, no change needed. The `request_exit_next_frame_` flag is already initialized to `false` in the member initializer.

### Step 9: Clean up dead code

22. The removed members and methods (`show_file_dialog_`, `file_dialog_action_`, `draw_file_dialog()`, `show_save_prompt_modal_`, `save_prompt_result_`, `handle_dirty_before_op()`) have no remaining references after the above changes. Verify by building.

## Required tests

All tests must follow the existing conventions in `tests/editor_tests.cpp`:
- Catch2 v3 macros (`TEST_CASE`, `REQUIRE`, `SECTION`)
- `[editor]` tag
- Headless engine via `EngineService::create(Backend::Headless, ...)`
- Tests exercise logic, not modal rendering

### Unit tests

| ID | What to test | Verification | Spec AC |
|---|---|---|---|
| UT-01 | **Dirty state**: construct Editor, verify `is_dirty()` is false, call `mark_dirty()`, verify `is_dirty()` is true, call `clear_dirty()`, verify false. | `REQUIRE_FALSE(editor.is_dirty())` → `editor.mark_dirty()` → `REQUIRE(editor.is_dirty())` → `editor.clear_dirty()` → `REQUIRE_FALSE(editor.is_dirty())` | AC-04 |
| UT-02 | **Window title**: construct Editor, verify `build_title_string()` returns `"Untitled — Buddd Editor"`. Mark dirty → verify returns `"Untitled* — Buddd Editor"`. Set file path via `save_scene_as`, clear dirty → verify `"scene.yaml — Buddd Editor"`. Mark dirty → verify `"scene.yaml* — Buddd Editor"`. | `REQUIRE(editor.build_title_string() == "Untitled — Buddd Editor")` then mutate state and recheck. | AC-04 |
| UT-03 | **Untitled scene**: construct Editor, verify `current_file_path()` is nullopt, title is "Untitled". | `REQUIRE_FALSE(editor.current_file_path().has_value())` | AC-06 |
| UT-04 | **New scene**: create Editor with entities, call `new_scene()`, verify world entity count is 0, file path is nullopt, dirty is false. | `editor.world().add_entity()` → `REQUIRE(editor.world().entity_count() > 0)` → `editor.new_scene()` → `REQUIRE(editor.world().entity_count() == 0)` → `REQUIRE_FALSE(editor.current_file_path().has_value())` → `REQUIRE_FALSE(editor.is_dirty())` | AC-09 |
| UT-05 | **New scene with dirty**: mark dirty, call `new_scene()`, verify dirty cleared, world empty. | Same as UT-04 but with `editor.mark_dirty()` before `new_scene()`. | AC-09 |
| UT-06 | **Save on clean scene**: verify `save_scene()` returns success immediately when `!dirty_` and file path is set. | Set file path, save, then save again (clean) → verify second save returns success (no-op). | Edge case |
| UT-07 | **File path tracking**: save_scene_as("path/a.yaml") → verify `current_file_path()` returns "path/a.yaml". | `editor.save_scene_as("path/a.yaml")` → `REQUIRE(editor.current_file_path().value() == "path/a.yaml")` | AC-03 |
| UT-08 | **Quit with clean scene**: verify that when `!dirty_`, the quit handler calls `ctx.request_exit()`. | Use `is_exit_requested()` flag after `ctx.request_exit()`. | AC-07 |
| UT-09 | **Save redirects to Save As on untitled**: verify `save_scene()` returns error when untitled (both clean and dirty). | First test: clean and untitled → `REQUIRE_FALSE(editor.save_scene().has_value())`. Second test: mark dirty, untitled → `REQUIRE_FALSE(editor.save_scene().has_value())`. | AC-06 |
| UT-10 | **Dirty flag after failed save**: simulate save failure (e.g., write to invalid path), verify dirty is NOT cleared. | Requires headless engine with registry/assets, set file path to invalid location, verify error and dirty remains true. | Error case |
| UT-11 | **World replaced by new_scene**: verify `world()` returns a valid reference after `new_scene()`. | `auto& w1 = editor.world()` → `editor.new_scene()` → `auto& w2 = editor.world()` → both valid, entity count 0. | AC-09 |
| UT-12 | **Close-request callback — clean scene**: Register a close-request callback via `Platform::set_on_close_request()`. Verify that when `!dirty_`, the callback returns `true` (allow close). | Create a headless Platform, register callback that captures a `bool` flag, verify the flag indicates `true` (allow close). | AC-07 |
| UT-13 | **Close-request callback — dirty scene**: Mark dirty, register close-request callback, verify the callback returns `false` (cancel close) and sets `pending_op_ = PendingOp::Quit`. | Mark dirty, invoke close-request logic. Verify return is `false`. | Edge case |
| UT-14 | **PlatformHeadless dialog no-op**: Verify that `show_open_file_dialog()` and `show_save_file_dialog()` immediately invoke callback with `std::nullopt` in headless mode. | Create headless Platform, call both dialog methods with a test callback, verify callback is invoked synchronously with `std::nullopt`. | Edge case |

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

### Test changes from ImGuiFileDialog era

- Tests that checked `show_file_dialog_` or `file_dialog_action_` directly must be removed (these members no longer exist).
- Tests for the save-prompt state machine remain unchanged — the state machine logic is the same, only the file dialog invocation mechanism changed (flags → direct Platform calls). The state machine tests (UT-04, UT-05, UT-08, UT-12, UT-13) test the Editor's internal state (`pending_op_`, `dirty_`), not the actual file dialog invocation, so they remain valid.
- UT-14 is new: verifies PlatformHeadless dialog methods are no-ops.
- No tests depended on `ImGuiFileDialog` directly (the test suite uses headless mode which never opens file dialogs).

### Test YAML fixtures

For IT-01 (round-trip), the test should programmatically create entities in the Editor's World, save to a temp file, and reload. No pre-existing test `.yaml` scene file is needed — the round-trip is self-contained.

For IT-02 (error handling), create a temp file with invalid YAML content (e.g., just `corrupt: [unclosed`).

## Edge cases

The implementation must handle the following edge cases (from spec + additional):

| Edge case | Required behavior |
|---|---|
| **Save on clean scene with file path** | `save_scene()` is a no-op when `!dirty_` and `current_file_path_` has a value. Returns success. No dialog. |
| **Save on clean untitled scene** | `save_scene()` returns error (no file path set — same as dirty untitled). Caller opens Save As dialog. Not a silent no-op. |
| **Save As on scene with existing file path** | Opens OS file dialog regardless. After saving, updates `current_file_path_` to the new path. |
| **Open Scene with clean scene** | No save-prompt. Platform file dialog opens immediately (via direct callback). |
| **New Scene with clean scene** | No save-prompt. World cleared immediately. |
| **Quit with clean scene** | No save-prompt. `ctx.request_exit()` called immediately. |
| **Cancel file dialog (Open)** | No action (callback invoked with `std::nullopt`). Current scene unchanged. No error. |
| **Cancel file dialog (Save As)** | No action (callback invoked with `std::nullopt`). Current file path unchanged. Dirty state unchanged. |
| **SDL3 dialog callback returns empty path** | Treat as cancellation. No action. |
| **SDL3 dialog callback returns nullptr (error)** | Treat as cancellation. No action. Logged at debug level. |
| **PlatformHeadless dialog open** | Immediately invokes callback with `std::nullopt` — no dialog, no action. |
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
| **OS close button (X / Alt+F4)** | Triggers the same save-prompt as File > Quit. Close is intercepted via `Platform::set_on_close_request()`. Save → save then close. Don't Save → discard and close. Cancel → abort close, editor stays open. |
| **Save-prompt Save on untitled scene (during Open/Quit/New)** | `save_scene()` fails → Save As dialog is opened directly from the save-prompt handler. After Save As completes, the pending operation (Open/New/Quit) proceeds. For Quit, uses `request_exit_next_frame_` flag since the Save As callback fires during `poll_events()` (outside `draw_ui()`). |
| **SDL3 dialog callback fires during poll_events()** | The callback accesses Editor state directly (`open_scene()`, `save_scene_as()`, `show_error_modal()`). These modify state between frames, which is safe. No mutex needed. |
| **Callback chain (Save As → Open)** | The Save As callback (during OpenScene with untitled dirty) nests a Platform Open dialog call. The Open dialog callback fires in a subsequent `poll_events()` call. This is safe — both callbacks fire on the main thread during event processing. |

## Security impact

- File operations use OS-level file access via `SceneLoader`/`SceneSaver` (no elevated privileges).
- SDL3 native file dialogs use the OS-native dialog, inheriting the process's file permissions.
- No network access. No credentials, secrets, or sensitive data handled.
- File operations triggered by explicit user action through the File menu — no automated file I/O.
- Path traversal protection handled by OS file dialog (user selects file; arbitrary path input not accepted).
- Scene files trusted (developer/artist-generated content).
- The `FileDialogCallback` signature accepts `std::optional<std::string>` — the path from the SDL callback is trusted (from OS file dialog).

## Data and migration impact

None. No schema changes, no data migrations, no seed data, no data loss risks. Scene files are user-created `.yaml` files on disk. No cross-session state is stored by the Editor (file path is runtime-only). The change from ImGuiFileDialog to SDL3 native dialogs is a UI-only change with no data migration implications.

## API compatibility impact

| API | Change | Impact |
|---|---|---|
| `buddd::engine::Platform` | Added pure virtual `show_open_file_dialog()` and `show_save_file_dialog()`. | Breaking change for any existing `Platform` subclass not in this repo. All three existing subclasses (`PlatformSDL3`, `PlatformHeadless`) are updated. |
| `buddd::engine::Platform` | Added `FileDialogCallback` type alias and `#include <optional>`. | All consumers of `platform.h` now have access to `FileDialogCallback` and `<optional>`. Non-breaking. |
| `buddd::editor::Editor` | Removed `show_file_dialog_`, `file_dialog_action_`, `draw_file_dialog()`. | Private/implementation detail changes. No public API breakage. |
| `buddd::editor::Editor` | Removed `show_save_prompt_modal_`, `save_prompt_result_` (dead code), `handle_dirty_before_op()` (dead code). | Private cleanup. No public API breakage. |
| `buddd::editor::Editor` | Added `request_exit_next_frame_` flag. | Private. No public API breakage. |
| Root `CMakeLists.txt` | Removed ImGuiFileDialog FetchContent. | Build system change: ImGuiFileDialog no longer fetched. No consumer impact. |
| `src/editor/CMakeLists.txt` | Removed `ImGuiFileDialog.cpp` from source list. | `buddd_editor` no longer compiles ImGuiFileDialog source. |
| `PlatformSDL3` | Removed mutex/dialog queue members. Added `get_sdl_window()` helper. | Internal refactoring. No public API change. |

## Documentation impact

- **README**: no changes.
- **Wiki pages**: `docs/wiki/editor/scene-management.md` — update ImGuiFileDialog references to SDL3 native file dialogs via Platform abstraction. Document the simplified callback design (heap-allocated callback, no mutex, no result queue).
- **Wiki pages**: `docs/wiki/architecture/module-map.md` — replace ImGuiFileDialog dependency with Platform dialog methods. Add `Platform::show_open_file_dialog()` and `Platform::show_save_file_dialog()` to the Platform submodule rows. Remove ImGuiFileDialog from `buddd_editor` module dependencies.
- **Wiki pages**: `docs/wiki/editor/scene-management.md` — update the OS file dialog section from ImGuiFileDialog to SDL3 native dialogs with the simplified callback design.
- **Other specs**: `.specs/sprint-2026-06/editor-ux-design/spec.md` — add note that F-01 implements AC-015 through AC-018 and AC-025, and that AC-015/Story 1 need correction to match clean-by-default.
- **ADR**: `docs/adr/ADR-029-editor-ux-decisions.md` — update AC-015 reference to match clean-by-default decision.

## ADR impact

No new ADR needed. The decisions in this contract are consistent with existing ADRs:
- ADR-019 (Architecture Boundaries): The Platform defines `FileDialogCallback` so editor code never includes SDL3 headers. SDL3 dialog APIs are only used in `src/engine/platform/`.
- ADR-027 (Editor Architecture): The Editor uses `EngineContext::services.platform()` to access dialog methods.
- ADR-026 (Dear ImGui Integration): No ImGuiFileDialog dependency. ImGui remains the only UI library.
- ADR-029 (Editor UX Decisions): Save-prompt state machine unchanged. File dialogs are now native (not ImGui-based), which is transparent to the UX.

The only architectural impact is that `Platform` now has dialog responsibilities, which is a natural extension of its role as the OS abstraction layer.

## Done criteria

The implementation is complete when all of the following are verifiable:

1. [ ] **Platform dialog abstraction** — `platform.h` has `FileDialogCallback` type alias, `show_open_file_dialog()` and `show_save_file_dialog()` pure virtual methods, and `#include <optional>`. Verify by reading `platform.h`.

2. [ ] **PlatformSDL3 implements native dialogs with simplified callback** — `platform_sdl3.cpp` includes `<SDL3/SDL_dialog.h>`. `show_open_file_dialog()` calls `SDL_ShowOpenFileDialog()` with a heap-allocated callback (deleted by the SDL C-lambda after invocation) and stack-allocated `SDL_DialogFileFilter`. `show_save_file_dialog()` calls `SDL_ShowSaveFileDialog()` with the same pattern. No mutex, no intermediate queue, no thread-safety mechanism. `poll_events()` unchanged. Verify by reading `platform_sdl3.cpp`.

3. [ ] **PlatformSDL3 has `get_sdl_window()` helper** — Private method that iterates `window_map_` and returns `static_cast<SDL_Window*>(win->native_handle())` for the first window. Returns nullptr if empty. Verify by reading `platform_sdl3.h` and `platform_sdl3.cpp`.

4. [ ] **PlatformHeadless dialog no-op** — Both dialog methods immediately invoke callback with `std::nullopt`. Verify by reading `platform_headless.cpp`.

5. [ ] **ImGuiFileDialog removed from build** — Root `CMakeLists.txt` has no ImGuiFileDialog FetchContent block. `src/editor/CMakeLists.txt` has no `ImGuiFileDialog.cpp` source and no ImGuiFileDialog include path. Verify by reading CMake files.

6. [ ] **`#include <ImGuiFileDialog.h>` removed** — No `ImGuiFileDialog.h` include in `src/editor/editor.cpp` or `src/editor/editor.h`. Verify by `grep -rn "ImGuiFileDialog" src/editor/`.

7. [ ] **Editor class has no `show_file_dialog_`, `file_dialog_action_`, `draw_file_dialog()`** — All removed from `editor.h` and `editor.cpp`. Verify by reading source files.

8. [ ] **Editor class has no dead members** — `show_save_prompt_modal_`, `save_prompt_result_` removed from `editor.h`. `handle_dirty_before_op()` removed from `editor.h` and `editor.cpp`. Verify by reading source files.

9. [ ] **Editor class has `request_exit_next_frame_` flag** — Private `bool` member added to `editor.h`. Initialized to `false`. Checked in `draw_ui()` Phase 6. Verify by reading `editor.h` and `editor.cpp`.

10. [ ] **MenuBar callbacks updated** — `Editor::setup()` registers menu callbacks that call `platform().show_open_file_dialog()` / `platform().show_save_file_dialog()` directly (not via `show_file_dialog_` flags). Verify by reading `editor.cpp`.

11. [ ] **Shortcut callbacks updated** — All shortcuts (Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Shift+S, Ctrl+Q) call Platform dialog methods or scene ops directly. Verify by reading `editor.cpp`.

12. [ ] **`draw_ui()` has no ImGuiFileDialog phase** — Phase 6 is now the `request_exit_next_frame_` check. `draw_file_dialog()` call removed. Verify by reading `editor.cpp`.

13. [ ] **Save-prompt state machine uses Platform dialogs directly** — In `draw_pending_op_modal()`, when save-prompt resolves with Discard/Save for OpenScene, the code calls `platform().show_open_file_dialog()` directly (not setting flags). The untitled Save redirect chain (Save As → Open) uses nested callbacks. Verify by reading `editor.cpp`.

14. [ ] **All existing tests pass** — Run `ctest` (or build and run `buddd_tests`). All existing tests still pass with no regressions. Verify by running the test suite.

15. [ ] **All new/changed F-01 tests pass** — Run `ctest`. All F-01 tests (UT-01 through UT-14, IT-01, IT-02, HT-01) pass. Verify by running the test suite.

16. [ ] **Zero warnings from `src/` and `tests/`** — Build with `-Wall -Wextra -Wpedantic` (or project's default warning flags) produces zero warnings from changed or new files. Verify by building.

17. [ ] **Engine core changes are limited to Platform dialog methods** — `git diff` shows no modifications to `SceneLoader`, `SceneSaver`, `World`, `Entity`, `ComponentRegistry`, `AssetManager`, `EditorApp`, or `run_app()`. Changes to `Platform` are limited to the new dialog methods. Changes to `Window` are none (already has `set_title()`). Verify by `git diff --stat`.

18. [ ] **ADR-019 compliance** — No `<SDL3/` headers included in `src/editor/`. The editor only uses `Platform::FileDialogCallback` and `Platform::show_open_file_dialog()` / `Platform::show_save_file_dialog()`. Verify by `grep -rnE '#include.*SDL3' src/editor/`.

19. [ ] **Logging implemented** — Verify log calls exist for: scene saved, scene loaded, load/save failures, mark dirty, dirty cleared, save-prompt results.

20. [ ] **OS close button fully implemented** — OS close button (X / Alt+F4) triggers the same save-prompt as File > Quit. Verify by reading:
    - `src/engine/platform/platform.h` — contains `set_on_close_request()` and `close_request_callback_` member.
    - `src/engine/platform/platform_sdl3.cpp` — `SDL_EVENT_QUIT` handler checks `close_request_callback_` before returning `false`.
    - `src/editor/editor.cpp` — `Editor::setup()` registers close-request handler via `ctx.services.platform().set_on_close_request(...)`.
    - If callback returns `false`, the quit event is swallowed and the render loop continues.
    - If callback returns `true` or no callback registered, `poll_events()` returns `false` as before.

21. [ ] **Close-request callback behavior** — The Editor's close-request handler:
    - If scene is clean (`!dirty_`), returns `true` immediately (allow close, no prompt).
    - If scene is dirty, sets `pending_op_ = PendingOp::Quit` and returns `false` (cancel close temporarily). The save-prompt modal appears in the next `draw_ui()` frame.
    - On Save/Discard: calls `ctx.request_exit()` after save-prompt resolves.
    - On Cancel: clears `pending_op_`, no exit — editor stays open.
    - Verify by reading `editor.cpp` in `Editor::setup()`. UT-12 (clean scene → allow close) and UT-13 (dirty scene → cancel close, set pending_op_) must pass.

22. [ ] **No thread-safety mechanism in PlatformSDL3** — Verify `platform_sdl3.h` has no `#include <mutex>`, no `dialog_mutex_`, `dialog_result_`, `dialog_callback_`, `dialog_pending_`, `dialog_filters_`, `dialog_nfilters_`, or `sdl_dialog_callback()` static method. Verify by reading `platform_sdl3.h`.

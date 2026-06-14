# IMPL-2026-008 — Port Remaining Popups to Dialog Abstraction

## Source spec

`.specs/sprint-2026-06/port-popups-to-dialog/spec.md`

## Goal

Port three remaining ad-hoc popups (error modals, ScenePanel delete confirmation, and Editor save-prompt) to use the existing `Dialog`/`CustomDialog` abstraction from `editor_dialog.h`. Apply the `"title###id"` pattern in the Phase 4 dialog render loop to prevent ImGui popup ID collisions. All three popups are rendered in Phase 4 alongside the existing About dialog. The save-prompt is converted from a multi-frame polling state machine to a callback-driven pattern. All ad-hoc state members for these popups are removed from `Editor` and `ScenePanel`. A general `Editor::defer()` mechanism replaces the one-frame-delay pattern, allowing dialog button callbacks to defer command execution to the next `draw_ui()` frame with a fresh `EditorContext`.

## Non-goals

- No new base class features — only the `DialogButton` callback signature changes from `void()` to `bool()`.
- No changes to engine APIs or build system — no new CMake targets, no SDL3/OpenGL/GLM changes.
- No changes to menu/panel architecture — dialogs remain in the existing `dialogs_` vector.
- `PendingOp` enum and `pending_op_` member remain. `request_exit_next_frame_` is removed in favor of `Editor::defer()`.
- No new dialog types — only the three existing popup types are ported. No toasts, overlays, or non-blocking notifications.
- No focus-stealing / auto-focus behavior changes — existing modal behavior preserved.
- No changes to `CommandStack` or `EditorContext` — the `Editor::defer()` mechanism works within existing API constraints.

## Relevant ADRs

None of the existing ADRs are contradicted by this implementation:
- **ADR-019** (architecture boundaries): No SDL3, OpenGL, or GLM headers are added to `src/editor/`.
- **ADR-026** (ImGui integration): ImGui continues to be used via existing patterns; no new dependencies.
- **ADR-027** (editor architecture): The Editor class remains unchanged in its lifecycle and architecture boundary.

## Files to inspect

Before editing, the Code Agent must read these files to understand the existing code:

| File | Purpose |
|---|---|
| `src/editor/editor_dialog.h` | Dialog abstraction — `Dialog`, `CustomDialog`, `DialogButton` API. Only `DialogButton::callback` type changes from `std::function<void()>` to `std::function<bool()>`. Read to understand `draw_content()` ordering, `handle_escape()`, `on_close` callback, and the `request_close()` mechanism that now follows the callback's `bool` return. |
| `src/editor/editor.h` | Editor class declaration — state members, method declarations to be removed/updated. |
| `src/editor/editor.cpp` | Editor implementation — Phase 4 render loop, error modal functions, save-prompt state machine, `draw_pending_op_modal()`, `execute_pending_op()`, `show_error_modal()`. |
| `src/editor/panels/scene_panel.h` | ScenePanel declaration — delete confirmation state members, `draw_delete_confirmation_modal()` declaration. |
| `src/editor/panels/scene_panel.cpp` | ScenePanel implementation — `execute_delete_entity()`, `draw_delete_confirmation_modal()`, `draw_ui()`. |
| `tests/editor/editor_tests.cpp` | Existing dialog tests — understand test patterns (`HeadlessTestContext`, `EscapeTrackDialog`, test tags `[editor][dialog]`, dialog lifecycle tests at lines 1084-1260). |
| `src/editor/commands/delete_entity_command.h` | Verify `DeleteEntityCommand::execute()` uses only `ctx.editor.*` (no `ctx.engine`). Confirmed: uses `ctx.editor.world()`, `ctx.editor.selection()`, `ctx.editor.selection().snapshot()`/`.clear()`. |
| `src/editor/editor_context.h` | Understand `EditorContext` struct (uses `Editor& editor` and `EngineContext const& engine`). |

## Files allowed to change

- `src/editor/editor.h`
- `src/editor/editor.cpp`
- `src/editor/panels/scene_panel.h`
- `src/editor/panels/scene_panel.cpp`
- `tests/editor/editor_tests.cpp`
- `src/editor/editor_dialog.h` — only the `DialogButton::callback` type change from `std::function<void()>` to `std::function<bool()>`

## Files forbidden to change

- `src/editor/editor_dialog.h` — Dialog abstraction must not be modified **except for the `DialogButton::callback` type change from `std::function<void()>` to `std::function<bool()>`** (per NG-01 now scoped to "no new base class features"). No other changes to `editor_dialog.h` are permitted.
- `src/editor/editor_context.h` — No changes.
- `src/editor/editor_menu.h` / `src/editor/editor_panel.h` — No changes.
- Any file in `src/engine/` — No engine changes.
- `CMakeLists.txt` or any build file — No build system changes.
- `src/editor/commands/` — No command class changes.
- `src/editor/command.h` / `src/editor/command_stack.h` / `src/editor/command_stack.cpp` — No API changes.

## Existing conventions to follow

1. **Every dialog must have a unique `id()`** — dedup in `Editor::open_dialog()` checks by ID. Error modals use a generated unique ID (timestamp + counter). Delete confirmation uses fixed ID `"confirm-delete"`. Save-prompt uses fixed ID `"save-changes"`.

2. **Button callbacks return `bool`** — `true` closes the dialog (the framework calls `request_close()`), `false` keeps it open (the dialog stays visible). This is the new contract: `DialogButton::callback` changed from `std::function<void()>` to `std::function<bool()>`. See `CustomDialog::draw_content()` in `editor_dialog.h` — the button rendering now checks the return value before calling `request_close()`. Existing tests at line 1142 of `editor_tests.cpp` will need their button callbacks updated to return `true`.

3. **`on_close` callback** fires only for Escape/click-outside dismissal (via `handle_escape()`), NOT for button clicks. It is NOT used for the delete confirmation or save-prompt button flows.

4. **Logging** (spec section "Observability", lines 268-273):
   - Error dialog opened: `BUDDD_LOG_DEBUG("Error dialog: {} (id={})", title, id)`
   - Delete confirmation shown: `BUDDD_LOG_DEBUG("Delete confirmation: {} entities ({} with children)", ids.size(), with_children)`
   - Save prompt shown: `BUDDD_LOG_DEBUG("Save prompt: {} (dirty)", pending_op_name)`
   - Save prompt action: `BUDDD_LOG_INFO("Save prompt: Save (pending={})", ...)` / `BUDDD_LOG_INFO("Save prompt: Discard ...")` / `BUDDD_LOG_INFO("Save prompt cancelled: pending_op cleared")`

5. **Tests**: Tag conventions: `[editor][dialog]` for dialog tests, `[editor][f01]` for scene-management tests. Use `HeadlessTestContext` (line 308 of `editor_tests.cpp`) when an engine context is needed. Use bare `Editor` for pure unit tests without engine.

6. **`auto` return type style**: Use trailing return type `auto foo() -> void` consistently.

7. **String formatting for `ImGui::Text`**: Always use `"%s", string.c_str()` format, never pass `std::string` directly (format-string warning risk).

8. **`#include` ordering**: `scene_panel.cpp` already includes `"editor.h"` which includes `"editor_dialog.h"`. No new includes needed in scene_panel.cpp for `CustomDialog`, `DialogButton`, `Dialog`.

9. **Include `<ctime>` in editor.cpp** for `std::time(nullptr)` used by the error modal ID generator.

10. **Use convenience helpers for common dialog patterns** — prefer the public convenience methods on `Editor` (`open_message_dialog()`, `open_error_dialog()`, `open_confirm_dialog()`, `open_ok_cancel_dialog()`) over manually constructing a `CustomDialog`. These reduce boilerplate and ensure consistent unique ID generation. Use `open_message_dialog()` for simple info OK-only dialogs, `open_error_dialog()` for error modals (semantic alias for `open_message_dialog`), `open_confirm_dialog()` for OK with a callback, and `open_ok_cancel_dialog()` for OK + Cancel with callbacks.

## Required implementation behavior

### Change 1: `"title###id"` fix in Phase 4 dialog render loop (`editor.cpp`)

In `Editor::draw_ui()`, Phase 4 (lines 429-440), make the following changes:

**a)** Replace the `ImGui::OpenPopup` call (line 431):
```cpp
// BEFORE:
ImGui::OpenPopup(dialog->title().c_str());
// AFTER:
auto popup_id = dialog->title() + "###" + dialog->id();
ImGui::OpenPopup(popup_id.c_str());
```

**b)** Remove the `opened_dialog_ids_.erase()` call (line 434) entirely:
```cpp
// DELETE this line:
opened_dialog_ids_.erase(dialog->id());
```

**c)** Replace the `ImGui::BeginPopupModal` call (line 436):
```cpp
// BEFORE:
if (ImGui::BeginPopupModal(dialog->title().c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
// AFTER:
if (ImGui::BeginPopupModal(popup_id.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
```

**Rules:**
- The `popup_id` variable is a `std::string` declared inside the for-loop body, used once for both `OpenPopup` and `BeginPopupModal`.
- The `opened_dialog_ids_` member in `editor.h` is **removed** entirely since `ImGui::OpenPopup` is called every frame unconditionally — the tracking set is no longer needed.
- Escape handling (lines 442-446) and closed-dialog cleanup (lines 449-455) remain unchanged.

### Change 2: Error modals (`editor.h` + `editor.cpp`)

**In `editor.h`:**

- **Remove** `show_error_modal()` private method declaration entirely (line 127). No replacement needed — the public `open_error_dialog()` helper (added in Change 7) serves the same purpose.
- **Remove** `draw_error_modals()` private method declaration (line 128).
- **Remove** state members:
  - `error_modal_title_` (line 169)
  - `error_modal_message_` (line 170)
  - `show_error_modal_` (line 171)

**In `editor.cpp`:**

- **Add** `#include <ctime>` at the top (for `std::time`).
- **Remove** `show_error_modal()` function entirely (lines 673-677). No replacement — `open_error_dialog(title, message)` in Change 7 replaces it.
- **Remove** `draw_error_modals()` function entirely (lines 679-694).
- **Remove** the Phase 7 call to `draw_error_modals()` in `draw_ui()` (line 473). Remove the entire Phase 7 section (the `// ═══════════` lines 470-473). After removal, the phases are 1-6 (Phase 6 is the last one).
- **Replace all `show_error_modal(` call sites with `open_error_dialog(`** throughout `editor.cpp`. The Code Agent must `grep` for every occurrence of `show_error_modal` in `editor.cpp` and replace with `open_error_dialog`. The known call sites include the save-prompt code in Change 4, but there may be others in scene load/save error paths. Every `show_error_modal(...)` call becomes `open_error_dialog(...)` with the same arguments.

### Editor::defer() mechanism (`editor.h` + `editor.cpp`)

A general deferred-action mechanism is added to `Editor` to allow dialog button callbacks
to defer execution of commands that require an `EditorContext` until the next `draw_ui()`
frame, where a fresh, valid context is available. This eliminates the need for per-panel
state members (like `pending_confirm_delete_`) to bridge the one-frame gap.

**In `editor.h`:**

- **Add** to the private section of `Editor` (after the dialog state members, before scene management state):

```cpp
// Deferred actions executed at the start of the next draw_ui() call
// with a fresh, valid EditorContext. Used by dialog button callbacks
// that need a context for CommandStack::execute().
std::vector<std::function<void(EditorContext const&)>> deferred_actions_;

auto defer(std::function<void(EditorContext const&)> action) -> void {
    deferred_actions_.push_back(std::move(action));
}
```

**In `editor.cpp`:**

- **Add** at the very beginning of `Editor::draw_ui()`, **before** the existing Phase 1 comment and code:

```cpp
// ── Flush deferred actions with the fresh context ──
for (auto& action : deferred_actions_) action(ctx);
deferred_actions_.clear();
```

This flush runs before any UI rendering, so deferred commands are already on the undo stack
before any UI elements are processed on that frame. The `EditorContext` `ctx` is valid because
it is the stack-local variable created at the top of `draw_ui()`.

### Change 3: Delete confirmation (`scene_panel.h` + `scene_panel.cpp`)

**Note — deferred command execution via `Editor::defer()`:**

The delete confirmation "Delete" button callback cannot execute `DeleteEntityCommand` directly because the callback fires during Phase 4 of `draw_ui()`, while the `EditorContext` reference needed by `CommandStack::execute()` is a stack-local variable that is not available from the button callback. The callback captures `Editor*` and entity IDs, then calls `editor->defer()` to enqueue the command for execution at the top of the next `draw_ui()` frame, where a fresh, valid `EditorContext` is available. This avoids adding any pending state members to `ScenePanel`.

**In `scene_panel.h`:**

- **Remove** state members:
  - `bool show_delete_confirmation_` (line 33)
  - `std::vector<buddd::engine::EntityId> pending_deletion_ids_` (line 34)
  - `size_t pending_deletion_with_children_` (line 35)
  - `std::string pending_deletion_first_name_` (line 36)
- **Remove** method declaration:
  - `auto draw_delete_confirmation_modal(EditorContext const& ctx) -> void` (line 45)
- **No new members added.** The one-frame-delay pattern (`pending_confirm_delete_`) is
  eliminated entirely — delete confirmation state now lives in the deferred-action
  lambda captured by the button callback via `Editor::defer()`.

**In `scene_panel.cpp`:**

**a) Rewrite `execute_delete_entity()`** (line 231):

Replace the entire `if (with_children > 0) { ... }` block (lines 261-289) with:

```cpp
if (with_children > 0) {
    // Build the dynamic confirmation message text (same format as before)
    std::string message;
    if (ids.size() == 1) {
        // Find the first entity's name for the singular form
        auto find_name = [&](auto& self, buddd::engine::Entity e) -> std::string {
            if (e.id() == ids[0]) return e.name();
            for (size_t i = 0; i < e.child_count(); ++i) {
                auto name = self(self, e.get_child(i));
                if (!name.empty()) return name;
            }
            return "";
        };
        std::string first_name;
        for (size_t i = 0; i < world.root_entity_count(); ++i) {
            auto root = world.get_root_entity(i);
            if (root.id() == ids[0]) { first_name = root.name(); break; }
            first_name = find_name(find_name, root);
            if (!first_name.empty()) break;
        }
        if (first_name.empty()) first_name = "(unnamed)";
        message = "Delete " + first_name + " and its " + std::to_string(with_children) + " children?";
    } else {
        message = "Delete " + std::to_string(ids.size()) + " entities? ("
                + std::to_string(with_children) + " have children that will also be deleted.)";
    }

    BUDDD_LOG_DEBUG("Delete confirmation: {} entities ({} with children)", ids.size(), with_children);

    ctx.editor.open_dialog(std::make_unique<CustomDialog>(
        "confirm-delete",
        "Confirm Delete",
        [message]() { ImGui::Text("%s", message.c_str()); },
        std::vector<DialogButton>{
            {"Delete", "del_btn", [editor = &ctx.editor, ids = std::move(ids)]() {
                // Defer command execution to the next draw_ui() frame,
                // where a fresh, valid EditorContext is available.
                editor->defer([ids = std::move(ids)](EditorContext const& fresh_ctx) {
                    auto cmd = std::make_unique<DeleteEntityCommand>(std::move(ids));
                    fresh_ctx.editor.command_stack().execute(std::move(cmd), fresh_ctx);
                });
                return true;  // Close dialog after callback.
            }},
            {"Cancel", "cancel_btn", []() {
                return true;  // Close dialog, no action needed.
            }}
        }
    ));
} else {
    // No entities with children — execute deletion immediately
    auto cmd = std::make_unique<DeleteEntityCommand>(std::move(ids));
    ctx.editor.command_stack().execute(std::move(cmd), ctx);
}
```

**b) Modify `draw_ui()`** (line 36):

- **Remove** the call to `draw_delete_confirmation_modal(ctx)` at line 218.
- **Remove** the preceding comment `// ── Delete confirmation modal ──`.
- No replacement needed — the deferred delete command is executed by the `Editor::defer()` flush at the top of `draw_ui()` before any UI processing.

**c) Remove `draw_delete_confirmation_modal()`** function entirely (lines 360-394).

### Change 4: Save-prompt (`editor.h` + `editor.cpp`)

**In `editor.h`:**

- **Remove** `SavePromptResult` enum declaration (lines 25-26).
- **Remove** private method declaration for `draw_save_prompt_modal()` (line 126).
- **Remove** state members:
  - `bool save_prompt_requested_` (line 165)
  - `bool save_prompt_seen_` (line 166)
  - `std::optional<std::string> pending_file_path_` (line 159)
  - `bool request_exit_next_frame_` (line 162)
- **Keep**:
  - `PendingOp` enum (line 29) and `pending_op_` member (line 158).
  - `auto draw_pending_op_modal(be::EngineContext const& ctx) -> void` declaration (line 129).
  - `auto execute_pending_op(be::EngineContext const& ctx) -> void` declaration (line 130).

**In `editor.cpp`:**

**a) Remove `draw_save_prompt_modal()`** function entirely (lines 698-748).

**b) Rewrite `draw_pending_op_modal()`** (lines 752-846):

The new implementation opens a `CustomDialog` via `open_dialog()` with button callbacks that capture `this` (Editor*) and the `pending_op_` value by value. The dialog uses the fixed ID `"save-changes"` for dedup.

```cpp
auto Editor::draw_pending_op_modal(be::EngineContext const& ctx) -> void {
    if (pending_op_ == PendingOp::None) return;

    if (!dirty_) {
        execute_pending_op(ctx);
        pending_op_ = PendingOp::None;
        return;
    }

    // Build scene name for the prompt text.
    std::string scene_name = "Untitled";
    if (current_file_path_.has_value()) {
        scene_name = std::filesystem::path(*current_file_path_).filename().string();
    }

    // Capture the pending_op value for the callbacks (by value, since the
    // callbacks may fire on a different frame's draw_ui() call).
    auto captured_op = pending_op_;
    auto op_name = [captured_op]() -> std::string_view {
        switch (captured_op) {
            case PendingOp::NewScene:  return "NewScene";
            case PendingOp::OpenScene: return "OpenScene";
            case PendingOp::Quit:      return "Quit";
            default:                   return "";
        }
    };

    BUDDD_LOG_DEBUG("Save prompt: {} (dirty)", op_name());

    // Try to open the save-prompt dialog. If it's already open,
    // open_dialog() returns false (dedup by "save-changes" ID).
    open_dialog(std::make_unique<CustomDialog>(
        "save-changes",
        "Save Changes",
        [scene_name]() {
            ImGui::Text("Save changes to %s?", scene_name.c_str());
        },
        std::vector<DialogButton>{
            // ── Save button ──
            {"Save", "save_btn", [this, captured_op]() {
                auto save_result = save_scene();
                if (save_result.has_value()) {
                    // Scene has a file path and save succeeded.
                    // Chain the pending operation.
                    BUDDD_LOG_INFO("Save prompt: Save (pending={})",
                        captured_op == PendingOp::NewScene ? "NewScene" :
                        captured_op == PendingOp::OpenScene ? "OpenScene" : "Quit");
                    if (captured_op == PendingOp::OpenScene) {
                        engine_->platform().show_open_file_dialog(
                            [this](std::optional<std::string> path) {
                                if (!path) return;
                                if (auto r = open_scene(*path); !r)
                                    open_error_dialog("Load Error", r.error().message);
                            },
                            "YAML Scene", "yaml");
                    } else if (captured_op == PendingOp::NewScene) {
                        new_scene();
                    } else if (captured_op == PendingOp::Quit) {
                        defer([](EditorContext const& fresh_ctx) {
                            fresh_ctx.request_exit();
                        });
                    }
                    pending_op_ = PendingOp::None;
                } else if (!current_file_path_.has_value()) {
                    // Untitled: redirect to Save As, then complete pending op.
                    auto original_op = captured_op;
                    pending_op_ = PendingOp::None;
                    engine_->platform().show_save_file_dialog(
                        [this, original_op](std::optional<std::string> save_path) {
                            if (!save_path) return;  // cancelled — stay on current scene.
                                    if (auto r = save_scene_as(*save_path); !r) {
                                open_error_dialog("Save Error", r.error().message);
                                return;
                            }
                            // Save succeeded — complete the original operation.
                            if (original_op == PendingOp::OpenScene) {
                                engine_->platform().show_open_file_dialog(
                                    [this](std::optional<std::string> path) {
                                        if (!path) return;
                                        if (auto r = open_scene(*path); !r)
                                            open_error_dialog("Load Error", r.error().message);
                                    },
                                    "YAML Scene", "yaml");
                            } else if (original_op == PendingOp::NewScene) {
                                new_scene();
                            } else if (original_op == PendingOp::Quit) {
                                defer([](EditorContext const& fresh_ctx) {
                                    fresh_ctx.request_exit();
                                });
                            }
                        },
                        "YAML Scene", "yaml", dialog_default_path().c_str());
                } else {
                    // Path exists but save failed.
                    open_error_dialog("Save Error", save_result.error().message);
                    pending_op_ = PendingOp::None;
                }
                return true;  // Close dialog after Save attempt.
            }},
            // ── Don't Save button ──
            {"Don't Save", "discard_btn", [this, captured_op]() {
                BUDDD_LOG_INFO("Save prompt: Discard (pending={})",
                    captured_op == PendingOp::NewScene ? "NewScene" :
                    captured_op == PendingOp::OpenScene ? "OpenScene" : "Quit");
                if (captured_op == PendingOp::OpenScene) {
                    engine_->platform().show_open_file_dialog(
                        [this](std::optional<std::string> path) {
                            if (!path) return;
                                if (auto r = open_scene(*path); !r)
                                    open_error_dialog("Load Error", r.error().message);
                            },
                            "YAML Scene", "yaml");
                    } else if (captured_op == PendingOp::NewScene) {
                    new_scene();
                } else if (captured_op == PendingOp::Quit) {
                    defer([](EditorContext const& fresh_ctx) {
                        fresh_ctx.request_exit();
                    });
                }
                pending_op_ = PendingOp::None;
                return true;  // Close dialog after Discard.
            }},
            // ── Cancel button ──
            {"Cancel", "cancel_btn", [this]() {
                BUDDD_LOG_INFO("Save prompt cancelled: pending_op cleared");
                pending_op_ = PendingOp::None;
                return true;  // Close dialog after Cancel.
            }}
        },
        // on_close fires on Escape/click-outside dismissal (NOT on button clicks).
        // Without this, Escape dismisses the dialog but leaves pending_op_ set,
        // causing draw_pending_op_modal() to reopen the dialog next frame.
        [this]() {
            BUDDD_LOG_INFO("Save prompt cancelled: pending_op cleared");
            pending_op_ = PendingOp::None;
        }
    ));
}
```

**c) Simplify `execute_pending_op()`** (lines 848-867):

```cpp
auto Editor::execute_pending_op(be::EngineContext const& ctx) -> void {
    switch (pending_op_) {
        case PendingOp::NewScene:
            new_scene();
            break;
        case PendingOp::OpenScene:
            // No-op: the file dialog is opened by save-prompt button callbacks,
            // not by execute_pending_op(). pending_file_path_ was always nullopt
            // (dead code) and has been removed.
            break;
        case PendingOp::Quit:
            ctx.request_exit();
            break;
        case PendingOp::None:
            break;
    }
}
```

**d) Remove the old `// (handle_dirty_before_op removed . . .)`** comment lines (869-870) — they refer to long-dead code from earlier sprints.

### Change 5: Remove unused "#include" (`editor.cpp`)

No `#include` changes needed beyond adding `<ctime>`. All existing includes remain. The `"editor_dialog.h"` is already included via `"editor.h"` (line 19 of `editor.h` includes `"editor_dialog.h"`).

### Change 6: Update existing About dialog Close button callback (`editor.cpp`)

The About dialog was ported in a previous feature and its Close button callback is `[](){}` (a void no-op). Since `DialogButton::callback` now returns `bool`, this must be updated to:

```cpp
{"Close", "close_btn", []() { return true; }}
```

Find the existing About dialog construction in `editor.cpp` (look for the `std::vector<DialogButton>` containing the Close button) and update the callback. This is the only change needed for the About dialog — no other About dialog behavior changes.

### Change 7: New Editor convenience dialog helpers

Four public convenience methods are added to `Editor` to reduce boilerplate for common dialog patterns. All auto-generate unique IDs using `std::time(nullptr)` + a static counter, ensuring uniqueness even within the same timestamp tick. The counter is a `uint64_t` static local variable to avoid overflow concerns.

**In `editor.h`:** Add to the public section of `Editor`:

```cpp
auto open_message_dialog(const std::string& title, const std::string& message) -> void;
auto open_error_dialog(const std::string& title, const std::string& message) -> void;
auto open_confirm_dialog(const std::string& title, const std::string& message,
    std::function<bool()> on_ok = []() { return true; }) -> void;
auto open_ok_cancel_dialog(const std::string& title, const std::string& message,
    std::function<bool()> on_ok = []() { return true; },
    std::function<bool()> on_cancel = []() { return true; }) -> void;
```

**In `editor.cpp`:** Add the implementations:

**`open_message_dialog`** — simple OK dialog, no callback. Closes when OK is clicked:
```cpp
auto Editor::open_message_dialog(const std::string& title, const std::string& message) -> void {
    static uint64_t counter = 0;
    auto id = "msgbox_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(counter++);
    open_dialog(std::make_unique<CustomDialog>(
        std::move(id), title,
        [message]() { ImGui::Text("%s", message.c_str()); },
        std::vector<DialogButton>{{"OK", "ok_btn", []() { return true; }}}
    ));
}
```

**`open_error_dialog`** — alias for message box, used at all error-modal call sites:
```cpp
auto Editor::open_error_dialog(const std::string& title, const std::string& message) -> void {
    open_message_dialog(title, message);
}
```

**`open_confirm_dialog`** — OK button with callback. Callback returns `bool` (true = close, false = keep open):
```cpp
auto Editor::open_confirm_dialog(const std::string& title, const std::string& message,
    std::function<bool()> on_ok) -> void {
    static uint64_t counter = 0;
    auto id = "confirm_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(counter++);
    open_dialog(std::make_unique<CustomDialog>(
        std::move(id), title,
        [message]() { ImGui::Text("%s", message.c_str()); },
        std::vector<DialogButton>{{"OK", "ok_btn", std::move(on_ok)}}
    ));
}
```

**`open_ok_cancel_dialog`** — OK + Cancel with callbacks. Both return `bool`:
```cpp
auto Editor::open_ok_cancel_dialog(const std::string& title, const std::string& message,
    std::function<bool()> on_ok, std::function<bool()> on_cancel) -> void {
    static uint64_t counter = 0;
    auto id = "okcancel_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(counter++);
    open_dialog(std::make_unique<CustomDialog>(
        std::move(id), title,
        [message]() { ImGui::Text("%s", message.c_str()); },
        std::vector<DialogButton>{
            {"OK", "ok_btn", std::move(on_ok)},
            {"Cancel", "cancel_btn", std::move(on_cancel)}
        }
    ));
}
```

**Call site migration**: All `show_error_modal(...)` call sites in `editor.cpp` are replaced with `open_error_dialog(...)`. The `show_error_modal` function is removed entirely (see Change 2). Error modal call sites now use the public helper:

```cpp
// Before:
show_error_modal("Load Error", result.error().message);

// After:
open_error_dialog("Load Error", result.error().message);
```

### Change 8: Remove `request_exit_next_frame_` from Phase 6 of `draw_ui()`

**In `editor.h`:**
- `request_exit_next_frame_` is removed as a state member. It has been moved from "Keep" to "Remove" in Change 4 above.

**In `editor.cpp`:**
- Remove the Phase 6 block in `draw_ui()` that checked `request_exit_next_frame_`. This block was the only remaining user of the member after the save-prompt button callbacks were updated in Change 4 to use `Editor::defer()` instead.
- All three save-prompt Quit paths (Save button for file-pathed scenes, Save button for untitled→Save As, and Don't Save button) now use:
  ```cpp
  defer([](EditorContext const& fresh_ctx) {
      fresh_ctx.request_exit();
  });
  ```
  instead of `request_exit_next_frame_ = true;`.

### Change 9: Remove `opened_dialog_ids_` tracking set

Since `ImGui::OpenPopup` is now called every frame unconditionally in Phase 4, the `opened_dialog_ids_` tracking set is unused.

**In `editor.h`:**
- Remove `std::unordered_set<std::string> opened_dialog_ids_` from the Editor class member declarations.
- Remove `#include <unordered_set>` if it was only included for this member. Verify by checking whether any other code in `editor.h` uses `std::unordered_set`.

**In `editor.cpp`:**
- Remove `opened_dialog_ids_.insert(id)` from the `open_dialog()` method. The `dialogs_` vector's existing ID-based dedup check (searching for a dialog with matching ID) handles dedup without the separate set.
- Remove `opened_dialog_ids_.erase(dialog->id())` from the Phase 4 render loop (previously described in Change 1b — this line is already removed as part of the `"title###id"` change).

**In `tests/editor/editor_tests.cpp`:**
- Remove or update any test that references `opened_dialog_ids_`. If a test checks the member directly (e.g., for size or contents), remove that assertion. If a test uses the member to verify dialog behavior, restructure the test to observe behavior from the `dialogs_` vector (via `open_dialog()` return values or other observable side-effects).

## Required tests

### Unit tests (in `tests/editor/editor_tests.cpp`)

All new tests use tag `[editor][dialog]`. Tests that require an engine context use `HeadlessTestContext`. Pure unit tests use bare `Editor`. Follow the existing test structure at the end of the file (after line 1260).

| Test | Description | Verification method | Links to spec AC |
|---|---|---|---|
| **T1: Error dialog via `open_error_dialog()` creates a CustomDialog** | Call `editor.open_error_dialog("Test Title", "Test Message")` directly (it is public), then verify the dialog is opened by observing that a second call to `open_dialog` with a different ID succeeds (or via other observable side effects). Since `open_error_dialog` calls `open_message_dialog` which calls `open_dialog`, the dialog should appear in the editor. | Create a bare `Editor`, call `open_error_dialog()` twice with different titles, verify both open successfully (no dedup collision since each gets its own ID). Or: capture side effects by checking that the `open_dialog` return for a subsequent different-ID dialog is `true`. | AC-001, AC-002 |
| **T2: Multiple error dialogs stack independently** | Open two error dialogs via `open_error_dialog()` (public), verify both are opened (different unique IDs, no dedup collision). Call `request_close()` on one, verify the other remains open. Since `open_error_dialog()` is public, this can be tested directly: call `open_error_dialog()` twice on a bare `Editor`, then call `open_dialog` with the same IDs to verify dedup (second call returns false). For `request_close` isolation, create two `CustomDialog` instances directly, call `request_close()` on one, and verify the other's `should_close()` is false. | Use bare `Editor`. Call `open_error_dialog("Title", "Msg")` twice → both succeed (no dedup since IDs differ). For close isolation: create two `CustomDialog` instances, call `request_close()` on one, verify the other's `should_close()` is false. | AC-003 |
| **T3: `open_message_dialog()` / `open_error_dialog()` produces correct dialog structure** | Call `open_message_dialog()` on a bare `Editor`. Verify that the dialog opened has title "Test Title", an OK button, and that clicking OK closes it. Since `dialogs_` is private, this can be verified indirectly: call `open_message_dialog("T", "M")`, then call `open_message_dialog("T", "M")` again — both should return successfully (no dedup collision). To verify the OK button closes the dialog, construct a `CustomDialog` with the same pattern and verify the button callback returns `true`. | Use bare `Editor`. Call `open_message_dialog("Test Title", "Test")` twice → verify both open (no dedup). For callback verification: create a `CustomDialog` matching the pattern, invoke the button callback, verify it returns `true`. | AC-002 |
| **T4: Delete confirmation via `open_dialog`** | Verify that `execute_delete_entity` opens a dialog with title "Confirm Delete" when entities have children. | Create entity with children via `World::add_entity()`/`create_child()`, select it, call `execute_delete_entity()` via ScenePanel (or simulate the selection + deletion trigger). Verify that a dialog with ID "confirm-delete" is opened (observe via dedup: second `open_dialog` with same ID returns false). | AC-004, AC-005 |
| **T5: Delete confirmation shows correct text** | Verify the content function produces the right text for both single-entity-with-children and multi-entity scenarios. | Create a `CustomDialog` with the same content function pattern used in the new `execute_delete_entity()`. Call `draw_content()` in a controlled ImGui context... or verify at the string-composition level by capturing the message in a test lambda. Since `draw_content()` uses ImGui, test the string-building logic separately: verify the message composition matches expected format for both cases. | AC-005 |
| **T6: Delete confirmation "Delete" button executes command via `Editor::defer()`** | Simulate clicking the Delete button: the button callback calls `editor->defer()` with a lambda. On the next frame (simulated by calling `Editor::draw_ui()` or manually flushing the deferred actions), the command executes. Verify `DeleteEntityCommand` is on the command stack (via undo availability). | Create entities with children, select them, call `execute_delete_entity()`, then call `editor.defer(...)` flush (either by running `draw_ui()` with a valid context or by directly invoking the deferred actions). Verify `command_stack().can_undo()` returns true and `undo_name()` == "Delete Entity". The test should verify that the defer callback captures entity IDs and `Editor*` pointer, and that the defer flush at the next frame executes the command with a valid context. | AC-006 |
| **T7: Delete confirmation "Cancel" button does nothing** | Click Cancel, verify no command is executed and no deferred action is queued. | The Cancel button callback returns `true` (close dialog, no action). No deferred action is created. Verify no command is on the undo stack and `Editor::deferred_actions_` is empty. | AC-006 |
| **T8: Save-prompt uses CustomDialog** | Verify that `draw_pending_op_modal()` opens a dialog with ID "save-changes" when `dirty_` and `pending_op_` are set. | Create Editor + HeadlessTestContext, setup. Set `dirty_` and `pending_op_` to a non-None value. Call `draw_pending_op_modal()`. Verify via dedup that a dialog with ID "save-changes" is in `dialogs_`. Since `dialogs_` is private, call `open_dialog` with "save-changes" again and expect `false`. | AC-007, AC-018 |
| **T9: Save-prompt Cancel and Escape both clear `pending_op_`** | Test both Cancel button and Escape: (1) invoke the Cancel button callback and verify `pending_op_` becomes `None`; (2) invoke the `on_close` callback (as triggered by `handle_escape()`) and verify `pending_op_` becomes `None`. | Create a `CustomDialog` with the same patterns as `draw_pending_op_modal()`. For the button path: invoke the Cancel button's callback and check `pending_op_`. For the Escape path: call `dialog.handle_escape()` (which fires `on_close_` then `request_close()`), then verify `pending_op_` is `None`. | AC-010, AC-011 |
| **T10: Save-prompt text displays scene name** | Verify the content function renders `"Save changes to [name]?"`. | Create the content function with a known scene name, capture output via a test that renders the content function in a non-ImGui way (e.g., string builder). Since ImGui::Text is called, verify at the string-composition level that the message matches the expected format. | AC-007 |
| **T11: Dialog open/close with "title###id" pattern** | Verify that two dialogs with the same title but different IDs can both be opened and dedup works by ID, not title. | Open two dialogs with title "Same Title" and IDs "a" and "b". Both should return true from `open_dialog`. Open again with ID "a" → false (dedup). | AC-016, AC-017 |
| **T12: Dead code removed** | Verify `SavePromptResult` enum, `save_prompt_requested_`, `save_prompt_seen_`, `pending_file_path_`, `error_modal_title_`, `error_modal_message_`, `show_error_modal_`, `show_delete_confirmation_`, `pending_deletion_ids_`, `pending_deletion_with_children_`, `pending_deletion_first_name_`, `request_exit_next_frame_`, `opened_dialog_ids_` are removed from headers. Also verify `#include <unordered_set>` is removed from `editor.h` if it was only needed for `opened_dialog_ids_`. | Compile check: static_assert or simply verify via grep that these names no longer exist in the headers. | AC-012, AC-013, AC-014, AC-015 |
| **T14a: All four convenience helpers create dialogs with unique IDs** | Call each helper (`open_message_dialog`, `open_error_dialog`, `open_confirm_dialog`, `open_ok_cancel_dialog`) on a bare `Editor`. Verify each opens a dialog. Since all generate unique IDs, calling the same helper twice with the same arguments should produce two separate dialogs (no dedup collision). | Invoke each helper on a bare `Editor` and verify no crash. Call each helper twice in succession — verify both calls open independent dialogs by checking that calling `open_dialog(...)` with a fresh ID after each succeeds. For `open_confirm_dialog` and `open_ok_cancel_dialog`, verify the callback is invokable and returns `bool`. | AC-002, AC-003 |
| **T13: All existing tests still pass** | Verify no regressions. | Run `buddd_tests` — all previously passing tests continue to pass. | AC-019 |
| **T14: Zero new warnings** | Build `src/editor/` and `tests/` with zero new warnings. | `cmake --build --preset debug` — verify zero new warnings from `src/editor/` and `tests/`. | AC-020 |

### E2E / Integration verification

| Test | Description |
|---|---|
| **Manual smoke test** (requires display) | Run `buddd edit`. 1) Open a corrupt .yaml file → error modal with "Load Error" appears with OK button. Dismiss. 2) Trigger two load errors in sequence → both stack, dismiss independently. 3) Select entity with children, press Delete → confirmation modal with correct text. Click Delete → entity deleted. 4) Make scene dirty, File > New → save-prompt appears. Test Save (untitled→Save As), Don't Save, Cancel. 5) File > Quit with dirty scene → save-prompt → Cancel → editor stays open. |
| **Headless test suite** (CI) | Run `buddd_tests` — all dialog, scene-management, and command tests pass. |
| **Clean build** (CI) | `cmake --build --preset debug` with zero new warnings from `src/editor/` and `tests/`. |

## Edge cases

From the spec (section lines 232-247), all edge cases must be handled:

1. **Error modal triggered while another error is showing**: Both dialogs appear stacked (different unique IDs). Second opens on top. User dismisses each independently. Verified by T2.

2. **Error modal triggered rapidly (10 errors in one frame)**: Each gets its own unique ID. 10 dialogs stack. No dedup. The `error_counter` static ensures unique IDs even within the same `std::time(nullptr)` tick.

3. **Error modal with empty title or message**: ImGui handles empty strings. Title bar shows empty. Body shows empty text. Still works — no special handling needed.

4. **Delete confirmation triggered while already showing**: `open_dialog("confirm-delete")` returns false due to ID-based dedup (already tested in existing dialog dedup tests). No duplicate modals.

5. **Save-prompt triggered while already showing**: Same dedup: `open_dialog("save-changes")` returns false. No duplicate save-prompts. Verified by T8.

6. **Save-prompt: Save clicked on untitled, error on Save As**: Save As file dialog opens → user picks path → `save_scene_as()` fails → error modal shown. `pending_op_` is already `PendingOp::None` (cleared before file dialog).

7. **Save-prompt: Save clicked on untitled, user cancels Save As**: User cancels the Save As file dialog → no save occurs → `pending_op_` is already `PendingOp::None` (operation aborted). Matches spec.

8. **Save-prompt: Save clicked on scene with file path, save fails**: `save_scene()` returns error → error modal shown with "Save Error". `pending_op_` cleared.

9. **Delete confirmation: empty selection**: `execute_delete_entity()` is only called when selection is non-empty. If it does happen, `ids` would be empty and `with_children` would be 0, so the `else` branch would create `DeleteEntityCommand` with empty `ids`, which is harmless (no-op).

10. **Delete confirmation: entity deleted between confirmation and Delete click**: The `DeleteEntityCommand` captures IDs at dialog creation time. If the entity is deleted externally (unlikely given modal is blocking), the command may operate on stale IDs — existing behavior preserved, no change.

11. **Headless mode (no ImGui init)**: `draw_ui()` returns early due to `initialized_` guard. `open_error_dialog()` / `open_message_dialog()` creates a dialog via `open_dialog()` but it's never rendered. `open_dialog()` makes no ImGui calls — safe. Verified by AC-021.

## Security impact

None. Dialog content is provided by editor code, not user input — no injection risk. File dialog callbacks use captured lambdas — same pattern as existing Platform integration. No changes to permissions or security posture.

## Data and migration impact

None. No schema changes, data migrations, seed data, or data loss risks. The behavior of all three ported popups is identical from the user's perspective.

## API compatibility impact

- **Private API changes**: `Editor` class removes several private state members and private methods. This has no external API impact. The `open_dialog()` public method remains unchanged.
- **Public API additions**: Four new public convenience methods on `Editor`:
  - `open_message_dialog(title, message)` — simple OK-only info dialog, no callback.
  - `open_error_dialog(title, message)` — alias for `open_message_dialog`, replaces all `show_error_modal(...)` call sites.
  - `open_confirm_dialog(title, message, on_ok)` — OK dialog with a `bool`-returning callback.
  - `open_ok_cancel_dialog(title, message, on_ok, on_cancel)` — OK + Cancel dialog with `bool`-returning callbacks.
  All generate unique IDs internally. No existing caller is affected.
- **Private API removed**: `show_error_modal()` private method is removed. All 15+ call sites are migrated to the public `open_error_dialog()`.
- **Public API changes**: `DialogButton::callback` changes from `std::function<void()>` to `std::function<bool()>`. Any external code that constructs `DialogButton` instances with a callable must update their callbacks to return `bool` (typically `true`).
- `SavePromptResult` enum is removed (was public). It was not used by any external code — only within `Editor` itself. Verify no external references exist. **Check**: grep for `SavePromptResult` in the entire codebase. If no references outside `editor.h`/`editor.cpp`, removal is safe.
- `opened_dialog_ids_` is removed — no longer needed since `ImGui::OpenPopup` is called every frame unconditionally. The `dialogs_` vector's ID check in `open_dialog()` handles dedup.
- `request_exit_next_frame_` is removed — replaced by `Editor::defer()` mechanism.

## Documentation impact

The following documents will be updated by the **wiki-agent** after implementation (described here for reference; the Code Agent does NOT update wiki pages):

- **`docs/wiki/editor/scene-management.md`**: Update lines 3, 60, 81, 130, 131, 186 — remove statements saying save-prompt and error modals are "not ported". Update Phase 4/5/7 documentation to reflect that all popups now use the Dialog abstraction. Update save-prompt "State machine" section (line 62) to describe the callback-driven approach.
- **`docs/wiki/architecture/module-map.md`**: Update Editor class entry (line 365): remove references to removed members (`error_modal_title_`, `show_error_modal_`, `save_prompt_requested_`, `save_prompt_seen_`, `draw_error_modals`, `draw_save_prompt_modal`, `draw_pending_op_modal` (replaced with new impl), `pending_file_path_`). Update ScenePanel entry to remove delete-confirmation members. Update dialog render loop description to reflect `"title###id"` pattern and removal of `opened_dialog_ids_` usage from render loop.
- **`docs/wiki/editor/editor-panels.md`**: Update lines 15 and 492 — remove mentions that save-prompt, error modals, and delete-confirmation "remain separate" or "not yet ported".
- **`docs/wiki/architecture/overview.md`**: Update lines 209 and 272 — remove descriptions of save-prompt and error modals as ad-hoc.

## ADR impact

No ADR impact. Existing ADRs (ADR-027, ADR-029, ADR-014, ADR-001, ADR-019, ADR-026) remain unchanged. The spec confirms no ADRs reference the specific error-modal or save-prompt implementation details in a way that would require updating.

## Done criteria

The following checklist must be fully satisfied for the implementation to be considered complete:

- [ ] **DC-01**: Phase 4 of `Editor::draw_ui()` uses `dialog->title() + "###" + dialog->id()` for both `ImGui::OpenPopup` and `ImGui::BeginPopupModal`. The `opened_dialog_ids_` member is entirely removed from `editor.h`, and all `.insert()`/`.erase()` calls are removed from `editor.cpp`. Verified by inspecting both files.

- [ ] **DC-02**: `show_error_modal()` is **removed** entirely — no declaration in `editor.h`, no implementation in `editor.cpp`. All call sites use `open_error_dialog(title, message)` instead. `open_error_dialog()` is a public helper on `Editor` that delegates to `open_message_dialog()`. Verified by inspecting `editor.h` and `editor.cpp`.

- [ ] **DC-03**: `draw_error_modals()` removed entirely. Phase 7 removed from `draw_ui()`. Verified by inspecting `editor.cpp`.

- [ ] **DC-04**: `error_modal_title_`, `error_modal_message_`, `show_error_modal_` removed from `editor.h`. Verified by inspecting `editor.h`.

- [ ] **DC-05**: `SavePromptResult` enum removed from `editor.h`. `draw_save_prompt_modal()` removed from `editor.h` and `editor.cpp`. `save_prompt_requested_`, `save_prompt_seen_`, `pending_file_path_` removed from `editor.h`. Verified by inspecting both files.

- [ ] **DC-06**: `DialogButton::callback` changed from `std::function<void()>` to `std::function<bool()>` in `editor_dialog.h`. The `CustomDialog::draw_content()` button rendering checks the return value: calls `request_close()` only if the callback returns `true`. Verified by inspecting `editor_dialog.h`.

- [ ] **DC-06a**: `draw_pending_op_modal()` rewritten to use `open_dialog()` with a `CustomDialog("save-changes", ...)` when `dirty_ && pending_op_ != None`. Button callbacks capture `this` and `pending_op_` by value and return `true` to close. Verified by inspecting `editor.cpp`.

- [ ] **DC-07**: `show_delete_confirmation_`, `pending_deletion_ids_`, `pending_deletion_with_children_`, `pending_deletion_first_name_` removed from `scene_panel.h`. Verified by inspecting `scene_panel.h`.

- [ ] **DC-08**: `draw_delete_confirmation_modal()` declaration and implementation removed from `scene_panel.h` and `scene_panel.cpp`. Verified by inspecting both files.

- [ ] **DC-08a**: Existing About dialog Close button callback updated from `[](){}` to `[](){ return true; }` in `editor.cpp`. Verified by inspecting `editor.cpp`.

- [ ] **DC-09**: `execute_delete_entity()` uses `ctx.editor.open_dialog()` with a `CustomDialog("confirm-delete", ...)` when entities have children. The Delete button callback captures `Editor*` and calls `editor->defer()` to enqueue command execution. Verified by inspecting `scene_panel.cpp`.

- [ ] **DC-09a**: `Editor::defer()` mechanism implemented: `deferred_actions_` vector and `defer()` method added to `editor.h`. The flush loop (`for (auto& action : deferred_actions_) action(ctx)`) runs at the top of `Editor::draw_ui()` before any Phase 1 code. Verified by inspecting `editor.h` and `editor.cpp`.

- [ ] **DC-10**: No `pending_confirm_delete_` member exists in `scene_panel.h`. The delete confirmation execution is entirely handled by `Editor::defer()` — no per-panel pending state. Verified by inspecting `scene_panel.h`.

- [ ] **DC-11**: `execute_pending_op()` simplified: OpenScene case is a no-op (dead code removed). Verified by inspecting `editor.cpp`.

- [ ] **DC-12**: `#include <ctime>` added to `editor.cpp` if not already present. Verified by inspecting `editor.cpp`.

- [ ] **DC-13**: All tests pass (compile and run). Run `buddd_tests` — zero failures. No regressions in previously passing tests. Verified by test results.

- [ ] **DC-14**: Zero new warnings from `src/editor/` and `tests/`. Build with `cmake --build --preset debug` — verify compiler output. Verified by build results.

- [ ] **DC-15**: New tests exist for the ported popups:
  - Error dialog via `open_error_dialog()` creates a standard dialog.
  - Multiple error dialogs can stack (different IDs both open).
  - Delete confirmation dialog opens and executes command via Editor::defer() mechanism.
  - Save-prompt dialog opens via `draw_pending_op_modal()`.
  - Save-prompt Cancel and Escape both clear `pending_op_` (button + `on_close` callback).
  - All four convenience helpers (`open_message_dialog`, `open_error_dialog`, `open_confirm_dialog`, `open_ok_cancel_dialog`) create dialogs with unique IDs.
  - Dead code members removed (compile-time check).
  - `"title###id"` pattern works (two dialogs with same title, different IDs).
  - Verified by test source and test results.

- [ ] **DC-16**: Manual smoke test (display-required) for all three ported popups: error modal, delete confirmation, save-prompt (Save/Don't Save/Cancel with dirty scene). Verified by manual execution description in coordination.md or a test log.

- [ ] **DC-17**: All four convenience helpers are declared in `editor.h` (public section) and implemented in `editor.cpp`:
  - `open_message_dialog(title, message)` — OK-only dialog with auto-generated `"msgbox_"` ID. Verified by inspection.
  - `open_error_dialog(title, message)` — delegates to `open_message_dialog`. Verified by inspection.
  - `open_confirm_dialog(title, message, on_ok)` — OK dialog with bool-returning callback, auto-generated `"confirm_"` ID. Verified by inspection.
  - `open_ok_cancel_dialog(title, message, on_ok, on_cancel)` — OK + Cancel with bool-returning callbacks, auto-generated `"okcancel_"` ID. Verified by inspection.
  - All four generate unique IDs using `std::time(nullptr)` + static counter. The static counter is a `uint64_t` to avoid overflow. Verified by inspecting both files.

- [ ] **DC-18**: `request_exit_next_frame_` is entirely removed from `editor.h`. All three save-prompt Quit paths use `defer([](EditorContext const& ctx) { ctx.request_exit(); })` instead of `request_exit_next_frame_ = true`. The Phase 6 block in `draw_ui()` that checked the member is removed. Verified by inspecting `editor.h` and `editor.cpp`.

- [ ] **DC-19**: `opened_dialog_ids_` is entirely removed: `std::unordered_set<std::string> opened_dialog_ids_` removed from `editor.h`; `.insert()` removed from `open_dialog()` in `editor.cpp`; `.erase()` removed from Phase 4 render loop. `#include <unordered_set>` removed from `editor.h` if it was only needed for this member. Tests referencing `opened_dialog_ids_` are removed or updated. Verified by inspecting `editor.h`, `editor.cpp`, and `tests/editor/editor_tests.cpp`.

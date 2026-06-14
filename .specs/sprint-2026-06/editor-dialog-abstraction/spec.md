# SPEC-2026-007 — Editor Dialog Abstraction

## Problem

The Buddd Editor currently has no reusable dialog abstraction. Every modal or popup (About popup, error modals, save-prompt, delete confirmation) uses ad-hoc booleans (`show_about_`, `show_error_modal_`, `save_prompt_requested_/save_prompt_seen_`) and inline `ImGui::OpenPopup` + `BeginPopupModal` / `EndPopupModal` calls scattered across the `Editor` class and panel implementations. This violates DRY, makes it hard to add new dialogs (each requires new booleans, state management, render-phase placement, and cleanup logic), and makes the modal lifecycle inconsistent across dialog types.

Without this abstraction:

- Every new dialog requires adding multiple state members and manual OpenPopup lifecycle management.
- Escape/close-dismissal handling is ad-hoc and varies per dialog (About sets `show_about_ = false`, save-prompt returns `Cancel` from a state machine, error modals set `show_error_modal_ = false`).
- There is no mechanism to prevent duplicate dialogs (nothing prevents open "About" being triggered twice).
- Dialog rendering is fixed at specific phases in `draw_ui()` — adding a new dialog means choosing a phase, inserting code there, and managing ordering relative to other modals.

## Goals

| ID | Goal |
|---|---|
| G-01 | **Reusable Dialog base class**: An abstract `Dialog` class in `namespace buddd::editor` with virtual methods `id()`, `title()`, `draw_content()`, `request_close()`, `should_close()`, and `handle_escape()`. |
| G-02 | **CustomDialog for simple dialogs**: A concrete `CustomDialog` class that accepts constructor parameters (`id`, `title`, `content_fn`, `buttons`, `on_close` callback) for one-liner dialogs without writing a subclass. |
| G-03 | **ID-based deduplication**: `Editor::open_dialog()` checks if a dialog with the same `id()` is already open. If so, the new dialog is rejected (returns `false`). Prevents double-opening the same dialog type. |
| G-04 | **Standardized modal lifecycle**: Editor renders all dialogs in a single `draw_ui()` phase. The framework calls `ImGui::OpenPopup` (once per dialog), `BeginPopupModal`, `draw_content()`, renders button bars (for CustomDialog), and `EndPopupModal`. After rendering, closed dialogs are removed. |
| G-05 | **Escape/close handling**: The topmost (last in vector) dialog receives Escape key events. `handle_escape()` defaults to `request_close()`. `CustomDialog::handle_escape()` fires the `on_close` callback then calls `request_close()`. |
| G-06 | **Port About popup**: Replace the ad-hoc `show_about_` boolean and `draw_about_popup()` method with a `CustomDialog` instance. Remove the old code. |
| G-07 | **Unit test coverage**: Tests for dialog lifecycle (open, render, close), ID-based dedup, stacking of multiple dialogs, Escape handling on topmost only, CustomDialog button callbacks, and headless safety. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No porting of save-prompt** — the existing save-prompt state machine (`draw_save_prompt_modal`, `draw_pending_op_modal`, `PendingOp`, `SavePromptResult`) is left unchanged. It may be migrated to a Dialog subclass in a future feature. |
| NG-02 | **No porting of error modals** — `show_error_modal_`, `draw_error_modals()`, and inline OpenPopup/BeginPopupModal for error dialogs are left unchanged. |
| NG-03 | **No porting of delete confirmation** — the "Confirm Delete" popup in `ScenePanel` is left unchanged. |
| NG-04 | **No changes to `menus_` or `panels_` architecture** — dialogs are stored in a separate `dialogs_` vector, not merged into `menus_` or `panels_`. |
| NG-05 | **No changes to engine APIs** — no SDL3, OpenGL, GLM, or any engine header changes. |
| NG-06 | **No changes to build system** — no new CMake targets or dependencies. The existing `buddd_editor` library target is modified in-place. |
| NG-07 | **No non-blocking overlays / toasts** — all dialogs are ImGui modal popups (blocking). Non-modal overlays are out of scope. |
| NG-08 | **No explicit result type** — the framework auto-closes after any button click; button callbacks execute actions without calling `request_close()`. No polling pattern for dialog results. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens dialogs via menu actions (Help > About) or other triggers. Interacts with dialog content and buttons (OK, Close, etc.). Can dismiss the topmost dialog with Escape. Cannot open duplicate dialogs. |
| **Editor developer** | Uses `Editor::open_dialog()` to show dialogs. Uses `CustomDialog` for simple dialogs (About, confirmations). Creates `Dialog` subclasses for complex dialogs (save-prompt, multi-step workflows). Adds buttons with callbacks. |
| **Future feature developer** | Builds on this abstraction for any new modal dialog. Relies on `open_dialog()` returning `false` on dedup. Does not need to manage booleans, OpenPopup lifecycle, or render-phase placement. |

## User-visible behavior

### Dialog API (`src/editor/editor_dialog.h` — new file)

An abstract base class `Dialog` in `namespace buddd::editor`:

```cpp
namespace buddd::editor {

struct DialogButton {
    std::string label;
    std::string label_id;        ///< ImGui ID suffix for the button (unique within the dialog)
    std::function<void()> callback;
};

class Dialog {
public:
    virtual ~Dialog() = default;
    virtual auto id() const -> std::string = 0;
    virtual auto title() const -> std::string = 0;
    virtual auto draw_content() -> void = 0;

    auto request_close() -> void { should_close_ = true; }
    [[nodiscard]] auto should_close() const -> bool { return should_close_; }

    virtual auto handle_escape() -> void { request_close(); }

protected:
    Dialog() = default;

private:
    bool should_close_ = false;
};

} // namespace buddd::editor
```

A concrete `CustomDialog` class for simple cases:

```cpp
class CustomDialog final : public Dialog {
public:
    CustomDialog(
        std::string id,
        std::string title,
        std::function<void()> content_fn,
        std::vector<DialogButton> buttons,
        std::function<void()> on_close = nullptr
    );

    auto id() const -> std::string override { return id_; }
    auto title() const -> std::string override { return title_; }
    auto draw_content() -> void override;
    auto handle_escape() -> void override;

private:
    std::string id_;
    std::string title_;
    std::function<void()> content_fn_;
    std::vector<DialogButton> buttons_;
    std::function<void()> on_close_;
};
```

**CustomDialog rendering order:**
1. `draw_content()` — renders the body content (text, widgets, etc.)
2. Button bar below content — each `DialogButton` rendered as an ImGui button in a single line. Each button has an `ImGui::SameLine()` separator (except the first). `label_id` provides unique ImGui IDs for the buttons.
3. Button callback fires on click; after the callback returns, the framework automatically calls `request_close()` on the dialog. Button callbacks do NOT need to call `request_close()` themselves.
4. Buttons may optionally specify a shortcut key (stored but not auto-bound — shortcuts are handled by the Editor's shortcut system for external shortcuts; dialog-internal shortcuts are future work).

**CustomDialog::handle_escape():**
1. Fires `on_close` callback (if set)
2. Calls `request_close()`

### Editor integration

The `Editor` class gains:

```cpp
// New private member:
std::vector<std::unique_ptr<Dialog>> dialogs_;

// New public method:
auto open_dialog(std::unique_ptr<Dialog> dialog) -> bool;
```

**`open_dialog()` behavior:**
1. Iterates `dialogs_` and checks if any existing dialog has `.id()` equal to the incoming dialog's `id()`.
2. If a match is found: return `false` (no insertion, dedup).
3. If no match: `push_back` the dialog, return `true`.

**`draw_ui()` integration (new phase, replacing Phase 4):**

The existing Phase 4 (About popup) is replaced with a general dialog rendering phase inserted at the same position (after panels, before save-prompt):

```
// Phase 4: Dialog rendering (replaces old About popup)
for (auto& dialog : dialogs_) {
    // Call OpenPopup once per dialog
    ImGui::OpenPopup(dialog->id().c_str());

    if (ImGui::BeginPopupModal(dialog->title().c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        dialog->draw_content();

        // Buttons are rendered inside CustomDialog::draw_content() — no dynamic_cast needed.

        ImGui::EndPopup();
    }
}

// After rendering loop, remove closed dialogs
std::erase_if(dialogs_, [](auto& d) { return d->should_close(); });
```

**OpenPopup tracking:** To avoid calling `ImGui::OpenPopup` every frame after the first open, the Editor tracks first-frame OpenPopup via a `std::unordered_set<std::string> opened_dialog_ids_`. On the first frame after `open_dialog()`, `OpenPopup` is called. On subsequent frames, `OpenPopup` is skipped (the popup remains open from the first call). This is managed by the Editor, not the Dialog.

**Escape handling:** After the rendering loop and before the cleanup, the Editor checks:
```cpp
if (!dialogs_.empty() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    dialogs_.back()->handle_escape();
}
```
Only the topmost (last in vector) dialog receives the Escape event. If there are stacked dialogs, the topmost handles Escape first.

**Per-dialog `OpenPopup` tracking in detail:**
- Editor maintains a `std::unordered_set<std::string> opened_dialog_ids_` alongside `dialogs_`.
- On `open_dialog()`: the ID is added to `opened_dialog_ids_`.
- In the render loop: if the dialog's ID is in `opened_dialog_ids_`, call `ImGui::OpenPopup(id)` and remove from `opened_dialog_ids_`. Otherwise, skip `OpenPopup`.
- When a dialog closes: nothing special — the next time `open_dialog()` is called for the same ID, it will be added to `opened_dialog_ids_` again.

### About popup migration

- **Before**: `bool show_about_` member, `draw_about_popup(ctx)` method, called in `draw_ui()` Phase 4.
- **After**: In `Editor::setup()`, the `MenuBar` on_about callback changes from `show_about_ = true` to:
  ```cpp
  menu_bar->set_on_about([this]() {
      open_dialog(std::make_unique<CustomDialog>(
          "about",
          "About Buddd Editor",
          [this]() {
              ImGui::Text("Buddd Engine v%s", buddd::engine::version().data());
          },
          std::vector<DialogButton>{
              {"Close", "close_btn", []() {
                  // No-op is fine: the framework auto-closes the dialog
                  // after any button click. No explicit request_close() needed.
              }}
          }
      ));
  });
  ```
  Button callbacks do NOT need to call `request_close()` — the framework automatically calls it after any button click. The callback only performs the action (if any). For a simple Close button, a no-op is sufficient.

- Remove `bool show_about_` from `Editor` class.
- Remove `draw_about_popup()` method declaration and implementation.
- Phase 4 in `draw_ui()` is replaced with the general dialog rendering loop.

### Headless / initialization safety

- `draw_ui()` is guarded by `if (!initialized_) return;` — dialogs are never rendered in headless or uninitialized states.
- `open_dialog()` has no such guard — dialogs can be added to the vector even before initialization (though they won't render). This is safe because `draw_ui()` simply iterates an empty or populated vector.
- No ImGui calls are made from `open_dialog()` or the Dialog constructor — they are deferred to the render phase.

## Key entities

### `Dialog` (abstract base, `src/editor/editor_dialog.h`)

| Method | Description |
|---|---|
| `id()` | Unique string identifier for dedup. Must be stable across calls. |
| `title()` | Title displayed in the modal window's title bar. |
| `draw_content()` | Renders the dialog body (text, widgets, custom UI). No button layout. |
| `request_close()` | Sets a flag indicating the dialog should be removed after the current frame. |
| `should_close()` | Returns the close flag. Editor checks this after render to decide removal. |
| `handle_escape()` | Called when Escape is pressed while this dialog is topmost. Default: `request_close()`. |

### `CustomDialog` (concrete, `src/editor/editor_dialog.h`)

| Feature | Description |
|---|---|
| **Constructor** | Takes `id`, `title`, `content_fn`, `buttons`, optional `on_close`. |
| **`draw_content()`** | Calls `content_fn_()`. |
| **Button rendering** | After `content_fn_()`, renders buttons in a row with separators. |
| **`handle_escape()`** | Fires `on_close_` (if set), then `request_close()`. |

### `DialogButton` (struct, `src/editor/editor_dialog.h`)

| Field | Type | Description |
|---|---|---|
| `label` | `std::string` | Display text on the button |
| `label_id` | `std::string` | ImGui ID suffix (unique within dialog) for the button |
| `callback` | `std::function<void()>` | Action to execute when button is clicked (framework auto-closes after callback returns) |

### Editor integration

| Member | Type | Description |
|---|---|---|
| `dialogs_` | `std::vector<std::unique_ptr<Dialog>>` | Owned list of currently-open dialogs. |
| `opened_dialog_ids_` | `std::unordered_set<std::string>` | Set of dialog IDs that need `OpenPopup` called on the next render. |
| `open_dialog()` | `auto(std::unique_ptr<Dialog>) -> bool` | Adds dialog if ID not already present. Returns `false` on dedup. |

## User stories

### Story 1 — Opening a simple dialog (Priority: P1)

As an editor developer, I want to open a simple informational dialog (like About) with a single call, so that I don't need to manage booleans, OpenPopup, or modal lifecycle manually.

**Given** the editor has a `CustomDialog` with id "about" and title "About Buddd Editor"
**When** I call `editor.open_dialog(std::make_unique<CustomDialog>(...))`
**Then** the dialog is added to `dialogs_`
**And** on the next `draw_ui()` call, `ImGui::OpenPopup` is called once for "about"
**And** the dialog renders as a modal with the title "About Buddd Editor"
**And** clicking a "Close" button fires its callback; the framework then auto-closes the dialog
**And** after rendering, the dialog is removed from `dialogs_`

### Story 2 — ID-based dedup prevents duplicate dialogs (Priority: P1)

As an editor developer, I want to prevent the same dialog from being opened twice, so that users never see duplicate modals.

**Given** a dialog with id "about" is already open in the editor
**When** I call `editor.open_dialog(std::make_unique<CustomDialog>("about", ...))` a second time
**Then** `open_dialog()` returns `false`
**And** `dialogs_.size()` remains unchanged
**And** no duplicate modal appears

### Story 3 — Stacking multiple dialogs (Priority: P2)

As an editor developer, I want to open multiple dialogs that stack on top of each other, so that the user can interact with nested modals in order.

**Given** the editor has two dialogs open: "about" and "confirm-delete" (in that order)
**When** both dialogs render in Phase 4
**Then** both modals appear, with "confirm-delete" on top (last opened = topmost)
**And** pressing Escape only closes "confirm-delete" (topmost)
**And** pressing Escape again closes "about" (now topmost)
**And** when "confirm-delete" is closed, "about" remains visible

### Story 4 — CustomDialog button callbacks (Priority: P1)

As an editor developer, I want buttons on a CustomDialog to execute callbacks and close the dialog, so that I can wire up actions (close, confirm, cancel) without subclassing.

**Given** a CustomDialog with a "Close" button (callback is a no-op or performs an action)
**When** the user clicks the "Close" button
**Then** the button callback fires
**And** after the callback returns, the framework auto-closes the dialog (calls `request_close()`)
**And** the dialog's `should_close()` returns `true` after rendering
**And** the dialog is removed from `dialogs_` at the end of the render phase

### Story 5 — Escape closes the topmost dialog (Priority: P1)

As an editor user, I want pressing Escape to close the topmost dialog, so that I can dismiss dialogs with a keyboard shortcut.

**Given** a single dialog is open and visible
**When** the user presses Escape
**Then** `handle_escape()` is called on the dialog (default: `request_close()`)
**And** the dialog closes on the next frame

**Given** two dialogs are stacked
**When** the user presses Escape
**Then** only the topmost dialog's `handle_escape()` is called
**And** the lower dialog remains open

### Story 6 — CustomDialog::handle_escape fires on_close callback (Priority: P2)

As an editor developer, I want the `on_close` callback to fire when the user dismisses a CustomDialog with Escape, so that I can clean up state or prevent close.

**Given** a CustomDialog with an `on_close` callback that sets a flag in the editor
**When** the user presses Escape
**Then** `handle_escape()` calls the `on_close` callback
**And** then calls `request_close()`
**And** the dialog closes

### Story 7 — Port About popup to CustomDialog (Priority: P1)

As an editor developer, I want the About popup to use CustomDialog instead of ad-hoc booleans, so that the old `show_about_` and `draw_about_popup()` code is removed.

**Given** the editor codebase before migration
**When** the migration is applied
**Then** `show_about_` is removed from `editor.h`
**And** `draw_about_popup()` is removed from `editor.h` and `editor.cpp`
**And** The `MenuBar::on_about` callback calls `open_dialog(std::make_unique<CustomDialog>(...))`
**And** the About modal renders identically to before (same text, same title, same Close button behavior)
**And** clicking Help > About opens the About dialog
**And** clicking the Close button or pressing Escape closes it

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-01 | `Dialog` abstract base class exists in `src/editor/editor_dialog.h` with pure virtual methods `id()`, `title()`, `draw_content()`, and non-virtual methods `request_close()`, `should_close()`, `handle_escape()`. | Inspect header — verify all methods declared with correct signatures. |
| AC-02 | `CustomDialog` class exists in `src/editor/editor_dialog.h`, inherits `Dialog`, and is constructable with `id`, `title`, `content_fn`, `buttons`, `on_close`. | Inspect header — verify constructor signature. |
| AC-03 | `DialogButton` struct exists with `label`, `label_id`, and `callback` fields. | Inspect header — verify struct definition. |
| AC-04 | `Editor::open_dialog(std::unique_ptr<Dialog>) -> bool` is declared in `editor.h` and implemented. | Compile check. Unit test: open a dialog, verify `dialogs_` size is 1, return value is `true`. |
| AC-05 | `open_dialog()` returns `false` when a dialog with the same `id()` is already open. No insertion occurs. | Unit test: open dialog with id "test" (returns `true`), open another with id "test" (returns `false`), verify `dialogs_` size is 1. |
| AC-06 | `open_dialog()` inserts the dialog when its `id()` is unique among open dialogs. | Unit test: open dialog with id "a" (true), dialog with id "b" (true), verify `dialogs_` size is 2. |
| AC-07 | Dialogs are rendered in `draw_ui()` Phase 4 (after panels, before save-prompt). Each dialog gets `ImGui::OpenPopup` called on the first frame after `open_dialog()`. | Unit test (with mocked ImGui or via integration test): open dialog, call `draw_ui()`, verify `OpenPopup` was called with the dialog's ID. Manual: run editor with display, open About, verify modal appears. |
| AC-08 | `ImGui::OpenPopup` is called only once per dialog open (not every frame). | Unit test: track calls to `OpenPopup`. Open dialog, call `draw_ui()` twice. Verify `OpenPopup` called only on first `draw_ui()` call. |
| AC-09 | After `draw_ui()`, dialogs where `should_close() == true` are removed from `dialogs_`. | Unit test: open dialog, call `request_close()`, call `draw_ui()`, verify `dialogs_` is empty. |
| AC-10 | CustomDialog renders `content_fn` body above the button bar in `draw_content()`. | Manual: open About dialog, verify engine version text appears above the Close button. |
| AC-11 | CustomDialog button callbacks execute when the button is clicked, and the framework auto-closes the dialog after the callback returns. | Unit test: open CustomDialog with a button that sets a test flag, simulate button click. Verify flag is set AND `should_close()` is `true` after the render phase. |
| AC-12 | Pressing Escape calls `handle_escape()` on the **topmost** dialog only. | Unit test: stack two dialogs (D1, D2). Simulate Escape press. Verify `handle_escape()` called on D2 only. Verify `should_close()` is `true` for D2, `false` for D1. |
| AC-13 | CustomDialog::handle_escape() fires `on_close` callback (if set) then calls `request_close()`. | Unit test: create CustomDialog with `on_close` that sets a flag. Call `handle_escape()`. Verify flag is set and `should_close()` is true. |
| AC-14 | Dialog::handle_escape() default calls `request_close()`. | Unit test: create a minimal Dialog subclass with no override. Call `handle_escape()`. Verify `should_close()` is true. |
| AC-15 | `show_about_` member is removed from `Editor` class. | Inspect `editor.h` — verify no `show_about_` field. |
| AC-16 | `draw_about_popup()` method declaration and implementation are removed. | Inspect `editor.h` and `editor.cpp` — verify no `draw_about_popup` declaration or definition. |
| AC-17 | Help > About opens the About dialog via `open_dialog()` with a CustomDialog. | Manual: run editor, click Help > About, verify About modal appears with engine version and Close button. Manual: click Close, verify modal closes. Manual: press Escape, verify modal closes. |
| AC-18 | Help > About opened twice does NOT show duplicate modals. | Manual: open About, click Help > About again while modal is open — no duplicate appears. |
| AC-19 | All existing tests still pass. | Run `buddd_tests` — all previously passing tests continue to pass. |
| AC-20 | Zero new warnings from `src/editor/` and `tests/`. | Build with `cmake --build --preset debug` — verify zero new warnings. |
| AC-21 | Dialogs are headless-safe: `draw_ui()` is guarded by `initialized_`, and `open_dialog()` makes no ImGui calls. | Unit test: create Editor without setup, call `open_dialog()`, call `draw_ui()` — no crash. Verify dialog is added but not rendered (no ImGui calls). |
| AC-22 | `Editor` has a `std::unordered_set<std::string> opened_dialog_ids_` member for tracking first-frame OpenPopup. | Inspect header — verify member exists. |

## E2E Verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Build with `BUDDD_HAS_DISPLAY=OFF`. Run `buddd_tests`. Verify `[editor][dialog]` tagged tests pass — lifecycle, dedup, stacking, Escape, headless safety, CustomDialog buttons. |
| **Manual smoke test (display)** | Run `buddd edit`. Click Help > About. Verify About modal appears with engine version and Close button. Click Close → modal closes. Open About again, click Help > About again while open → no duplicate. Press Escape → modal closes. |
| **Clean build verification (CI)** | Run `cmake --build --preset debug` and verify zero new warnings from `src/editor/` and `tests/`. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | An editor developer can open a simple dialog with a single `open_dialog()` call, without adding booleans or state members to the Editor class. | Demonstrate by adding a new test dialog (e.g., "Hello World") to `editor_dialog.h` and calling `open_dialog()` from a menu callback — no Editor member changes needed. |
| SC-002 | The About popup behaves identically to before after migration — same content, same title, same close behavior (Close button + Escape). | Manual smoke test compares old and new behavior. All F-01 scene management tests pass. |
| SC-003 | Attempting to open a dialog with a duplicate ID is silently rejected — no duplicate modals, no crash, no log error. | Unit test AC-05 passes. |
| SC-004 | Stacked dialogs are rendered correctly, and Escape only closes the topmost. | Unit test AC-12 passes. Manual test: open two dialogs (About + another test dialog), press Escape, verify only the topmost closes. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **Empty `dialogs_` vector** | Phase 4 loop body is skipped. No `OpenPopup` or `BeginPopupModal` calls. No-op. |
| **Dialog removed before first render** | If `request_close()` is called between `open_dialog()` and the next `draw_ui()` call, the dialog is removed in the cleanup phase after the next `draw_ui()` (or before the next draw). Since no `OpenPopup` was ever called, no orphan popup state. |
| **Dialog ID with special characters (spaces, colons)** | Used as ImGui popup ID via `dialog->id().c_str()`. ImGui IDs are string-based and handle any characters. No encoding issues. |
| **Two dialogs with same title but different IDs** | They are distinct modals (different ImGui popup IDs via `dialog->id()`). The title is only for display in the title bar. Both render simultaneously. |
| **CustomDialog with zero buttons** | Only `content_fn` is rendered. No button bar. Still closable via Escape (`on_close` fires, then `request_close()`). |
| **CustomDialog button callback throws** | The exception propagates — no special handling. Consistent with rest of editor (ImGui callbacks are noexcept-unknown). |
| **Escape pressed when dialogs_ is empty** | No dialog receives `handle_escape()`. Normal ImGui Escape handling applies (may close current ImGui popup context). Existing shortcut-only behavior unaffected. |
| **Dialog subclass with custom handle_escape() that does NOT call request_close()** | Dialog remains open. The Editor respects `should_close()` — if the dialog does not set it, it stays open. The Escape key is consumed by the dialog. |
| **open_dialog() called outside draw_ui() during update()** | Dialog is added to `dialogs_` vector. It will be rendered on the next `draw_ui()` call. `OpenPopup` ID is added to `opened_dialog_ids_`. Safe. |
| **open_dialog() called with nullptr** | Caller responsibility — `Editor` does not null-check. `std::unique_ptr` cannot hold null by convention. |
| **Headless mode (no ImGui init)** | `draw_ui()` returns early due to `initialized_` guard. Dialogs remain in vector but are never rendered. No OpenPopup or BeginPopupModal calls. |
| **Dialog::draw_content() calls request_close()** | Legal — dialog can self-close during rendering. Close flag is checked after the rendering loop. Safe, no double-free. |
| **Very long dialog title (> 256 chars)** | ImGui truncates title in modal title bar. No crash. |

## Error cases

| Case | Expected behavior |
|---|---|
| **`BeginPopupModal` returns false (popup not yet visible)** | Standard ImGui pattern: skip content rendering, call `EndPopup` only if `BeginPopupModal` returned true. If false, the dialog is not visible this frame — wait for next frame. The `should_close()` flag is not affected. |
| **`open_dialog()` called after Editor shutdown** | The Editor is not destroyed yet. Dialogs are rendered only if `draw_ui()` is called (it won't be after shutdown). No crash. |
| **Button callback calls `request_close()` during `draw_content()` (before button bar)** | `request_close()` just sets a flag. The flag is checked after the entire rendering loop. Safe — no mid-frame removal. |
| **Memory allocation failure in `push_back`** | `std::bad_alloc` propagates. Consistent with rest of codebase (no special OOM handling). |
| **Dialog ID is empty string (`""`)** | ImGui popup ID will be empty. This could cause ImGui ID collisions with other unnamed popups. Not explicitly prevented — documented as developer responsibility. |

## Permissions and security

- No changes to permissions or security posture.
- Dialogs render UI content provided by the Editor — no file I/O, no network access, no user-supplied code execution.
- No authentication or authorization boundaries are crossed.
- `Dialog` callbacks are developer-written, not user-supplied — no injection risk.

## Observability

| Signal | Source |
|---|---|
| **Dialog opened** | `BUDDD_LOG_DEBUG("Dialog opened: {}", dialog->id())` in `open_dialog()` when a new dialog is added. Also in dedup case: `BUDDD_LOG_DEBUG("Dialog dedup: {} already open", id)` — helpful for debugging unexpected dedup behavior. |
| **Dialog closed** | `BUDDD_LOG_DEBUG("Dialog closed: {}", dialog->id())` when a dialog is removed from `dialogs_` after rendering. |
| **Dialog count** | No per-frame logging. Could be added as debug log if dialog leak is suspected. |
| **OpenPopup tracking** | No logging — low-level ImGui detail. Debuggable via ImGui metrics. |
| **Escape handling** | `BUDDD_LOG_DEBUG("Escape on topmost dialog: {}", dialog->id())` — helpful for debugging stacked-dialog Escape behavior. |

## Documentation impact

The following existing wiki pages must be updated when this spec is implemented:

| Document | Reason for update |
|---|---|
| `docs/wiki/architecture/module-map.md` (buddd_editor section) | Add `editor_dialog.h` to the Editor class file listing. Document `Dialog`, `CustomDialog`, `DialogButton`, `open_dialog()`, and `dialogs_`. Remove reference to `show_about_` and `draw_about_popup()`. |
| `docs/wiki/editor/editor-panels.md` | Document the dialog abstraction as a mechanism for rendering transient modals alongside panels and menus. |
| `docs/wiki/editor/scene-management.md` | Document that Phase 4 is now the general dialog rendering phase. Note that the save-prompt and error modals are still separate (out of scope for this pass). |

These updates are the responsibility of the wiki-agent and will be tracked in the wiki-agent section of coordination.md.

## Out of scope

- Porting save-prompt modals to Dialog subclasses.
- Porting error modals to Dialog subclasses.
- Porting delete-confirmation popup (ScenePanel) to Dialog subclass.
- Non-blocking overlays / toasts.
- Focus-stealing or auto-focus behavior on dialog open.
- Dialog z-ordering beyond insertion order (last opened = topmost).
- Dialog resize/minimize support.
- Dialog animation (fade-in, slide-in).
- Dialog return value / result type pattern.
- `on_close` callback on non-CustomDialog (abstract) dialogs — each subclass defines its own close behavior.
- ImGui `OpenPopup` level tracking beyond the `has_opened_once_` flag.
- Any changes to `menus_` or `panels_` iteration or interface.
- Any changes to engine APIs or build system.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `ImGui::OpenPopup`, `ImGui::BeginPopupModal`, `ImGui::EndPopup`, `ImGui::CloseCurrentPopup`, `ImGui::IsKeyPressed(ImGuiKey_Escape)`, and `ImGui::SameLine` are all available from the ImGui docking branch (v1.91.8-docking). |
| A-02 | `std::erase_if` (C++20) is available in the project's C++ standard. The project already uses C++20 features elsewhere. |
| A-03 | The `Editor` class uses `std::vector` for `dialogs_` — iteration order is insertion order. Last inserted dialog is the topmost. |
| A-04 | Dialogs are transient — they are not persisted, serialized, or restored across sessions. |
| A-05 | The `opened_dialog_ids_` unordered_set is managed entirely inside `Editor::draw_ui()`. No external code needs to know about OpenPopup timing. |
| A-06 | Button callbacks do NOT need to call `request_close()` — the framework auto-closes after any button click. Callbacks are safe because they are invoked during `draw_ui()` while the Dialog is alive in `dialogs_`, and the Dialog is not destroyed mid-frame. The `on_close` callback (for Escape/X dismissal) captures raw pointers safely for the same reason. |
| A-07 | `draw_ui()` is called once per frame. The `OpenPopup` tracking relies on this — "first frame" means the first `draw_ui()` call after `open_dialog()`. |
| A-08 | The About popup is the only dialog being ported. The save-prompt, error modals, and delete-confirmation remain as-is and are not affected by this change. |
| A-09 | `std::unordered_set<std::string>` is sufficient for tracking opened dialog IDs. The performance impact is negligible (dialog count is typically < 10). |
| A-10 | All dialogs use `ImGuiWindowFlags_AlwaysAutoResize` for their modal windows (consistent with the About popup and error modals). CustomDialog uses this flag. Subclasses may override by not passing the flag in their own `BeginPopupModal` call (but the framework always passes this flag — subclasses that want different flags should manage their own `BeginPopupModal` call within `draw_content()`). |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | **How does the Editor determine which dialog to call `handle_escape()` on when dialogs implement their own `BeginPopupModal` inside `draw_content()`?** | **Only the framework's own `BeginPopupModal` is tracked.** If a Dialog subclass opens its own modal inside `draw_content()` (bypassing the framework), the framework still calls `handle_escape()` on the topmost dialog in `dialogs_`. The subclass is responsible for its own Escape handling in that case. This is acceptable because complex subclasses (like save-prompt) that use multi-frame state machines will override `handle_escape()`. |
| Q-02 | **Should `CustomDialog` buttons support `ImGuiKey` shortcuts (as specified in the API design)?** | **Store the shortcut field but do NOT auto-bind in this pass.** The `DialogButton` struct includes an `std::optional<ImGuiKey> shortcut` field. Editor shortcut binding is deferred — the framework does not handle dialog-internal shortcuts in this spec. The shortcut field exists for future use and documentation. Buttons with shortcuts will still work via click — the shortcut is not enforced. |
| Q-03 | **Should `CustomDialog` buttons have a default "Close" behavior (auto `request_close()`) if the callback is empty?** | **Not needed — the framework already auto-closes after any button click.** If the callback is empty, the button still closes the dialog. The `on_close` callback on the CustomDialog handles Escape/click-outside dismissal only. |
| Q-04 | **How does the Close button callback in the About dialog close the dialog?** | **Not needed — the framework auto-closes after any button click.** Button callbacks only perform the action (if any). A simple Close button uses a no-op callback; the framework calls `request_close()` automatically. |

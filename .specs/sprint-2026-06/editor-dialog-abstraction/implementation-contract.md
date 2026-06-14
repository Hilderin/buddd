# IMPL-2026-007 — Editor Dialog Abstraction

## Source spec

- `.specs/sprint-2026-06/editor-dialog-abstraction/spec.md`

## Goal

Introduce a reusable `Dialog` abstraction for the Buddd Editor — an abstract base class (`Dialog`) and a concrete `CustomDialog` — that replaces ad-hoc boolean-based modal popups. The `Editor` gains `dialogs_` storage, `open_dialog()` with ID-based deduplication, and a general dialog rendering phase in `draw_ui()`. The About popup (`show_about_` / `draw_about_popup()`) is migrated to a `CustomDialog` instance. All code changes are confined to `src/editor/` and `tests/editor/`.

## Non-goals

- NG-01 through NG-08 from the source spec: no porting of save-prompt, error modals, delete confirmation; no changes to `menus_`/`panels_` architecture, engine APIs, or build system.
- No `ImGuiKey` shortcut auto-binding — the `shortcut` field on `DialogButton` is stored for future use but never read by the framework.
- No support for non-blocking overlays / toasts — all dialogs are modal.
- No focus-stealing, z-ordering beyond insertion order, resize, animation, or result-type pattern.
- No changes to `src/engine/`, `src/cmd/`, or `tests/CMakeLists.txt`.

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-027 (Editor Architecture) | All new code goes in `src/editor/`. Editor is a static library. `namespace buddd::editor`. Direct member variables (no PIMPL). |
| ADR-026 (ImGui Integration) | `<imgui.h>` available in editor code. OpenPopup, BeginPopupModal, SameLine, IsKeyPressed(ImGuiKey_Escape) all usable. |
| ADR-029 (Editor UX Decisions) | Phase numbering in `draw_ui()` is established. The new dialog phase replaces the old About popup phase. |

## Files to inspect

| File | Reason |
|---|---|
| `src/editor/editor.h` | Current Editor class declaration — `show_about_` and `draw_about_popup()` to be removed. |
| `src/editor/editor.cpp` | Current Editor implementation — `draw_ui()` phases, About popup code, menu callback wiring. |
| `src/editor/panels/menu_bar.h` | `set_on_about()` callback signature — must be changed from `show_about_ = true` to `open_dialog(...)`. |
| `tests/editor/editor_tests.cpp` | Existing test patterns (HeadlessTestContext, CmdTestCtx) and tag conventions ([editor], [f01], etc.). |

## Files allowed to change

- `src/editor/editor.h` — Add `dialogs_`, `opened_dialog_ids_`, `open_dialog()`. Remove `show_about_`, `draw_about_popup()`. Add `#include <unordered_set>`.
- `src/editor/editor.cpp` — Replace Phase 4 About popup with general dialog rendering. Migrate About popup to `CustomDialog`. Implement `open_dialog()`.
- `tests/editor/editor_tests.cpp` — Add `[editor][dialog]`-tagged test cases for lifecycle, dedup, stacking, Escape, CustomDialog buttons, headless safety.

## Files forbidden to change

- Any file under `src/engine/` (including `src/engine/imgui/`, `src/engine/platform/`, etc.)
- Any file under `src/cmd/`
- `src/editor/panels/menu_bar.h` — The `set_on_about` callback signature (`std::function<void()>`) must remain unchanged. Only the *caller* (editor.cpp) changes what callback is passed.
- `tests/CMakeLists.txt` — GLOB picks up new test code automatically.
- `src/editor/CMakeLists.txt` — GLOB picks up `editor_dialog.h` automatically (it is a header-only file).
- Any other `.specs/` files.

## Existing conventions to follow

1. **Namespace**: All new types go in `namespace buddd::editor` (per ADR-027).
2. **C++ standard**: C++20 (`std::erase_if`, `std::unordered_set`, `std::make_unique`).
3. **Error handling**: `buddd::engine::Result<void>` for fallible operations (per ADR-001). `open_dialog()` returns `bool` (specifically returning `false` for dedup, which is not an error — not a `Result`).
4. **Logging**: `BUDDD_LOG_DEBUG()` for dialog lifecycle events, `BUDDD_LOG_TAG("Editor")` already at top of `editor.cpp`.
5. **ImGui**: Direct use of `<imgui.h>` APIs (`OpenPopup`, `BeginPopupModal`, `EndPopup`, `SameLine`, `Button`, `IsKeyPressed(ImGuiKey_Escape)`, `CloseCurrentPopup`).
6. **ImGui Modal pattern**: `if (ImGui::BeginPopupModal(...)) { ... ImGui::EndPopup(); }` — the `else` branch handles the case where the popup is not yet visible.
7. **Test patterns**: `HeadlessTestContext` struct provides headless engine + context. Tags like `[editor][dialog]`, `[editor][f01]`. Use `REQUIRE`/`REQUIRE_FALSE` macros. Display-dependent tests guarded by `#ifdef BUDDD_HAS_DISPLAY`.
8. **Header guards**: `#pragma once` (consistent with existing `editor.h`).
9. **Private member naming**: Trailing underscore for private members (`dialogs_`, `opened_dialog_ids_`).
10. **Move semantics**: `std::unique_ptr` for ownership transfer. `auto` return types with trailing return type syntax.

## Required implementation behavior

### 1. New file: `src/editor/editor_dialog.h`

Create a header-only file with the following exact declarations in `namespace buddd::editor`:

```cpp
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <imgui.h>

namespace buddd::editor {

struct DialogButton {
    std::string label;
    std::string label_id;        // ImGui ID suffix for the button (unique within the dialog)
    std::function<void()> callback;
    std::optional<ImGuiKey> shortcut = std::nullopt;  // stored, not auto-bound
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

} // namespace buddd::editor
```

**CustomDialog implementation rules (inline in .cpp or in .h):**

- **`draw_content()` behavior**: Call `content_fn_()`. Then, if `buttons_` is non-empty, render each button using `ImGui::Button()`. Use `label_id` as the button label via `PushID`/`PopID` scoping. After each button, call `ImGui::SameLine()` except after the last button. On button click: invoke `callback()`, then call `request_close()` (auto-close). Use the following pattern:
  ```cpp
  for (size_t i = 0; i < buttons_.size(); ++i) {
      if (i > 0) ImGui::SameLine();
      ImGui::PushID(buttons_[i].label_id.c_str());
      if (ImGui::Button(buttons_[i].label.c_str())) {
          if (buttons_[i].callback) buttons_[i].callback();
          request_close();
      }
      ImGui::PopID();
  }
  ```

- **`handle_escape()` implementation**: If `on_close_` is set (`if (on_close_)`), call it. Then call `request_close()`.

### 2. Modify `src/editor/editor.h`

- **Remove**:
  - `#include <unordered_set>` (add — not currently there)
- `bool show_about_ = false;` (in the Panel state flags section)
- `auto draw_about_popup(buddd::engine::EngineContext const& ctx) -> void;` (in the private method declarations section)
  - The `// ── About popup ──` comment block

- **Add includes**:
  - `#include <memory>` (already present)
  - `#include <unordered_set>`
  - `#include "editor_dialog.h"`

- **Add public method**:
  ```cpp
  /// Open a dialog. Returns false if a dialog with the same id() is already open.
  auto open_dialog(std::unique_ptr<Dialog> dialog) -> bool;
  ```

- **Add private members** (place after existing members, before `// ── Window geometry tracking ──` section or at end of private section):
  ```cpp
  // ── Dialog state ──
  std::vector<std::unique_ptr<Dialog>> dialogs_;
  std::unordered_set<std::string> opened_dialog_ids_;
  ```

### 3. Modify `src/editor/editor.cpp`

#### 3a. Implement `open_dialog()`

Place after `add_menu()` / `add_panel()` implementations, before `draw_ui()`:

```cpp
auto Editor::open_dialog(std::unique_ptr<Dialog> dialog) -> bool {
    auto const& incoming_id = dialog->id();
    for (auto const& existing : dialogs_) {
        if (existing->id() == incoming_id) {
            BUDDD_LOG_DEBUG("Dialog dedup: {} already open", incoming_id);
            return false;
        }
    }
    BUDDD_LOG_DEBUG("Dialog opened: {}", incoming_id);
    opened_dialog_ids_.insert(incoming_id);
    dialogs_.push_back(std::move(dialog));
    return true;
}
```

#### 3b. Update `setup()` — change on_about callback

In `Editor::setup()`, replace:
```cpp
menu_bar->set_on_about([this]() {
    show_about_ = true;
});
```
with:
```cpp
menu_bar->set_on_about([this]() {
    open_dialog(std::make_unique<CustomDialog>(
        "about",
        "About Buddd Editor",
        [this]() {
            ImGui::Text("Buddd Engine v%s", be::version().data());
        },
        std::vector<DialogButton>{
            {"Close", "close_btn", []() {
                // No-op: the framework auto-closes after any button click.
            }}
        }
    ));
});
```

#### 3c. Replace Phase 4 in `draw_ui()`

Replace:
```cpp
// ═══════════════════════════════════════════════
// Phase 4: About popup (rendered every frame if show_about_ is true)
// ═══════════════════════════════════════════════
draw_about_popup(ctx);
```
with:
```cpp
// ═══════════════════════════════════════════════
// Phase 4: Dialog rendering
// ═══════════════════════════════════════════════
for (auto& dialog : dialogs_) {
    // OpenPopup only once per dialog open (first frame after open_dialog)
    auto it = opened_dialog_ids_.find(dialog->id());
    if (it != opened_dialog_ids_.end()) {
        ImGui::OpenPopup(dialog->id().c_str());
        opened_dialog_ids_.erase(it);
    }

    if (ImGui::BeginPopupModal(dialog->title().c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        dialog->draw_content();
        ImGui::EndPopup();
    }
}

// Escape handling: only for the topmost dialog
if (!dialogs_.empty() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    BUDDD_LOG_DEBUG("Escape on topmost dialog: {}", dialogs_.back()->id());
    dialogs_.back()->handle_escape();
}

// Remove closed dialogs (logging is inline in the erase_if predicate)
std::erase_if(dialogs_, [](auto& d) {
    if (d->should_close()) {
        BUDDD_LOG_DEBUG("Dialog closed: {}", d->id());
        return true;
    }
    return false;
});
```

**Important**: The "After rendering loop, remove closed dialogs" cleanup must come AFTER the Escape handling, so that Escape can mark a dialog as closing and the erase_if removes it in the same frame.

#### 3d. Remove `draw_about_popup()` method

Delete the entire `Editor::draw_about_popup()` function body in `editor.cpp` (find and remove the function definition).

#### 3e. Logging for dialog removal

Not a separate block — logging is performed inline within the `std::erase_if` predicate in Section 3c. No additional code is required.

### 4. Button auto-close contract

The framework (CustomDialog::draw_content) MUST call `request_close()` AFTER the button callback returns. The button callback MUST NOT call `request_close()` itself. This is the single authoritative pattern — no exceptions.

### 5. Escape dispatch contract

Only the topmost (last in `dialogs_`) dialog receives `ImGui::IsKeyPressed(ImGuiKey_Escape)` events. The check is:
```cpp
if (!dialogs_.empty() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    dialogs_.back()->handle_escape();
}
```
This runs AFTER all dialogs have been rendered but BEFORE the erase_if cleanup.

### 6. Headless safety

- `draw_ui()` is already guarded by `if (!initialized_) return;` — this covers the dialog rendering loop.
- `open_dialog()` makes no ImGui calls — it only modifies `dialogs_` and `opened_dialog_ids_`. Safe to call even before `setup()`.
- `Dialog` constructor makes no ImGui calls.

## Required tests

All tests must be in `tests/editor/editor_tests.cpp` with tag `[editor][dialog]`.

### Unit tests

| # | Test | Source spec AC | Description |
|---|---|---|---|
| UT-01 | **Dialog lifecycle (open, render, close)** | AC-04, AC-09 | Open a minimal Dialog subclass. Call `draw_ui()`. Call `request_close()`. Call `draw_ui()`. Verify dialog is removed. |
| UT-02 | **ID-based dedup — same ID returns false** | AC-05 | Open dialog "test" (returns `true`). Open another with id "test" (returns `false`). Verify `dialogs_` size is 1 (cannot verify `dialogs_` directly because it's private; instead verify behavior: the return value). |
| UT-03 | **ID-based dedup — different IDs both open** | AC-06 | Open dialog "a" (true), dialog "b" (true). Both accepted. |
| UT-04 | **OpenPopup called once per dialog** | AC-07, AC-08 | Open dialog, call `draw_ui()` twice. Use a Dialog subclass where `draw_content()` sets an externally-observable counter. After two `draw_ui()` calls, verify the counter is 2 — the dialog's content is rendered every frame, confirming OpenPopup gating happens only on the first frame. Use `opened_dialog_ids_` emptiness as a secondary proxy: verify the private set is empty after the first `draw_ui()` (observable via the dialog not being re-opened — a pragmatically equivalent check). |
| UT-05 | **Escape calls handle_escape on topmost only** | AC-12 | Stack two dialogs (D1, D2). Simulate Escape press. Verify D2's close flag is set, D1's is not. Requires a way to observe dialog state; easiest with a custom Dialog subclass that tracks whether `handle_escape()` was called. |
| UT-06 | **CustomDialog button callback fires and auto-closes** | AC-11 | Create CustomDialog with a button whose callback sets a test flag. Simulate button click (call `draw_content()`, then the internal button logic). Verify flag is set AND `should_close()` is true. (This can only be fully tested by rendering the dialog and simulating ImGui click — see integration tests below.) |
| UT-07 | **CustomDialog::handle_escape fires on_close** | AC-13 | Create CustomDialog with `on_close` that sets a flag. Call `handle_escape()`. Verify flag is set and `should_close()` is true. |
| UT-08 | **Dialog::handle_escape default calls request_close** | AC-14 | Create a minimal Dialog subclass (no override). Call `handle_escape()`. Verify `should_close()` is true. |
| UT-09 | **Headless safety** | AC-21 | Create Editor without `setup()`. Call `open_dialog()`. Call `draw_ui()` (no-op due to `initialized_` guard). No crash. Verify dialog is not processed. |
| UT-10 | **Empty dialogs_ is no-op** | Edge case | Call `draw_ui()` with empty `dialogs_`. No OpenPopup or BeginPopupModal calls. No crash. |
| UT-11 | **CustomDialog with zero buttons** | Edge case | Create CustomDialog with empty `buttons_`. Call `draw_content()`. Only `content_fn` is rendered. No button bar. |
| UT-12 | **CustomDialog button does NOT need request_close** | AC-11 | Call `draw_content()` with a button. After button click callback, verify `should_close()` became true (the framework calls `request_close()` after the callback). |

### Integration / E2E verification

| # | Test | Description |
|---|---|---|
| IT-01 | **About popup migration — Help > About opens dialog** | `#ifdef BUDDD_HAS_DISPLAY` test: configure offscreen SDL3, create Editor, call `setup()`, call `draw_ui()`. Programmatically trigger the about callback (via `on_about_`). Call `draw_ui()` again. Verify dialog renders (uses ImGui popup). Manually verify no crash. |
| IT-02 | **About popup Close button closes dialog** | Display test: open About, simulate button click. Verify dialog closes. |
| IT-03 | **Dedup of About dialog** | Display test: open About via `open_dialog()`, then call `open_dialog()` again with about — returns `false`. No duplicate. |
| IT-04 | **Stacked dialogs — Escape only closes topmost** | Display test: open two dialogs, press Escape, verify only topmost closes. |

### Manual smoke tests (not automated)

- Run `buddd edit`. Click Help > About. Verify About modal appears with engine version and Close button.
- Click Close → modal closes.
- Open About again, click Help > About again while open → no duplicate.
- Press Escape → modal closes.

## Edge cases

All edge cases from the spec (lines 376-392) must be handled as described. Key ones:

| Edge case | Expected behavior |
|---|---|
| **Empty `dialogs_` vector** | Phase 4 loop body is skipped. No OpenPopup or BeginPopupModal calls. No-op. |
| **Dialog closed before first render** | If `request_close()` called between `open_dialog()` and next `draw_ui()`, erased after next `draw_ui()`. No orphan popup. |
| **Dialog ID with special characters** | Used as ImGui popup ID via `dialog->id().c_str()`. ImGui handles any characters. |
| **Two dialogs with same title, different IDs** | Distinct modals (different ImGui popup IDs via `dialog->id()`). Both render. |
| **CustomDialog with zero buttons** | Only `content_fn` rendered. No button bar. Closable via Escape (fires `on_close`, then `request_close()`). |
| **Button callback calls `request_close()` during `draw_content()`** | Legal — just sets a flag. Flag checked after render loop. Safe, no double-free. |
| **Escape pressed when `dialogs_` empty** | No-op. Normal ImGui Escape handling applies. |
| **`draw_ui()` returns early (headless/uninitialized)** | Dialogs remain in vector. No OpenPopup/BeginPopupModal. |
| **Dialog ID is empty string** | Not explicitly prevented. Documented as developer responsibility. |
| **`BeginPopupModal` returns false (not yet visible)** | Standard ImGui pattern: skip rendering, call `EndPopup` only if `BeginPopupModal` returned true. |

## Security impact

None. Dialog callbacks are developer-written, not user-supplied. No injection risk. No new file I/O, network access, or authentication boundaries.

## Data and migration impact

None. Dialogs are transient — no persistence, schema changes, or migrations.

## API compatibility impact

- **Breaking**: `Editor` class removes `show_about_` member (private) and `draw_about_popup()` method (private). No external callers access these — they are only used internally within `editor.cpp`. Safe removal.
- **Additive**: `Editor` gains public method `open_dialog(std::unique_ptr<Dialog>) -> bool`. All existing code continues to compile.
- **Additive**: New public header `editor_dialog.h` with `Dialog`, `CustomDialog`, `DialogButton`.
- **Backward-compatible**: The `MenuBar::set_on_about()` callback signature (`std::function<void()>`) is unchanged. Only the callback body changes.

## Documentation impact

- **Wiki pages**: The following wiki pages must be updated (responsibility of wiki-agent, documented here for traceability):
  - `docs/wiki/architecture/module-map.md` — Add `editor_dialog.h` to the `buddd_editor` section. Document `Dialog`, `CustomDialog`, `DialogButton`, `open_dialog()`, and `dialogs_`. Remove references to `show_about_` and `draw_about_popup()`.
  - `docs/wiki/editor/editor-panels.md` — Document the dialog abstraction as a mechanism for rendering transient modals alongside panels and menus.
  - `docs/wiki/editor/scene-management.md` — Document that Phase 4 is now the general dialog rendering phase. Note that save-prompt and error modals remain separate.
- **README**: None.
- **Other specs**: None.

## ADR impact

No new ADR needed. This implementation follows the existing architectural decisions (ADR-027, ADR-026, ADR-029). No ADR is deprecated or amended.

## Done criteria

- [ ] `src/editor/editor_dialog.h` exists with `Dialog` (abstract), `CustomDialog` (concrete), and `DialogButton` (struct) — all signatures matching the spec.
- [ ] `DialogButton` struct includes `std::optional<ImGuiKey> shortcut = std::nullopt`.
- [ ] `Dialog` has pure virtual `id()`, `title()`, `draw_content()` and non-virtual `request_close()`, `should_close()`, `handle_escape()`.
- [ ] `Dialog::handle_escape()` default implementation calls `request_close()`.
- [ ] `CustomDialog::handle_escape()` fires `on_close` callback (if set) then calls `request_close()`.
- [ ] `CustomDialog::draw_content()` renders `content_fn_()` then buttons with `ImGui::SameLine()` separators, calls `request_close()` after each button callback.
- [ ] `CustomDialog::draw_content()` with empty `buttons_` renders only the content — no button bar.
- [ ] `Editor::open_dialog(std::unique_ptr<Dialog>) -> bool` declared in `editor.h` and implemented in `editor.cpp`.
- [ ] `Editor::open_dialog()` checks for duplicate ID via iteration, returns `false` on dedup, `true` on insertion.
- [ ] `Editor::open_dialog()` adds dialog ID to `opened_dialog_ids_`.
- [ ] `Editor` has private `std::vector<std::unique_ptr<Dialog>> dialogs_` member.
- [ ] `Editor` has private `std::unordered_set<std::string> opened_dialog_ids_` member (verify `#include <unordered_set>` added).
- [ ] `bool show_about_` removed from `Editor` class.
- [ ] `draw_about_popup()` declaration removed from `editor.h`.
- [ ] `draw_about_popup()` implementation removed from `editor.cpp`.
- [ ] Phase 4 in `draw_ui()` replaced with dialog rendering loop using `opened_dialog_ids_` for conditional `OpenPopup`.
- [ ] Escape handling present after dialog render loop (before cleanup): calls `handle_escape()` on `dialogs_.back()`.
- [ ] Closed dialog cleanup via `std::erase_if` after render + escape handling.
- [ ] `BUDDD_LOG_DEBUG("Dialog opened: ...")` in `open_dialog()`.
- [ ] `BUDDD_LOG_DEBUG("Dialog dedup: ...")` in `open_dialog()` dedup path.
- [ ] `BUDDD_LOG_DEBUG("Dialog closed: ...")` when dialog is removed.
- [ ] `BUDDD_LOG_DEBUG("Escape on topmost dialog: ...")` in Escape handling.
- [ ] Menu bar on_about callback uses `open_dialog(std::make_unique<CustomDialog>(...))` with Close button (no-op callback).
- [ ] All `[editor][dialog]` tests pass:
  - UT-01: Dialog lifecycle (open, render, request_close, remove)
  - UT-02: ID-based dedup (same ID returns false)
  - UT-03: Different IDs both open
  - UT-04: OpenPopup called once per dialog
  - UT-05: Escape on topmost only (stacked dialogs)
  - UT-06: CustomDialog button callback fires and auto-closes
  - UT-07: CustomDialog::handle_escape fires on_close
  - UT-08: Dialog::handle_escape default calls request_close
  - UT-09: Headless safety (no crash, no ImGui calls)
  - UT-10: Empty dialogs_ is no-op
  - UT-11: CustomDialog with zero buttons
  - UT-12: Button callback does NOT need request_close (framework does it)
- [ ] All existing tests pass (run `buddd_tests` — no regressions).
- [ ] Zero new compiler warnings from `src/editor/` and `tests/editor/`.

# Implementation Contract Review — IMPL-F-01 Editor Scene Load/Save Integration

**Contract file:** `.specs/sprint-2026-06/editor-scene-load-save/implementation-contract.md`
**Reviewer:** implementation-contract-critic
**Date:** 2026-06-12

## Summary

The IMPL-F-01 contract is thorough, well-structured, and covers nearly every aspect required for implementation. It includes precise file-level instructions, a detailed state machine for the save-prompt workflow, comprehensive test tables (13 UT + 2 IT + 2 DT + 1 HT), explicit edge cases with required behavior, and full API compatibility documentation. The architecture boundary (ADR-019) and editor architecture (ADR-027) are respected.

**Both previously blocking issues (B-01 and B-02) have been resolved:**

1. **B-01 (on_quit_ callback type mismatch)**: `on_quit_` type changed to `std::function<void(be::EngineContext const&)>`, setter signature matches, `draw_ui(ctx)` passes the current frame's context. No dangling reference — ctx is a function parameter, not a captured reference.

2. **B-02 (Save on clean untitled scene)**: Guard changed to `if (!dirty_ && current_file_path_.has_value()) return {};`. Clean untitled scenes now correctly fall through to the file-path null check and return an error (triggering Save As redirect), matching AC-06 and UT-09.

Additionally, **UT-12 and UT-13** were added for close-request callback testing, and Done criteria items 11 and 17 now require these tests. No new issues were introduced.

### Re-review #3 (2026-06-12 — SDL3 Native File Dialog Replacement)

Re-reviewed the updated contract replacing ImGuiFileDialog with SDL3 native file dialogs via Platform abstraction. Key findings:

- **ADR-019 boundary fully respected**: `FileDialogCallback` type alias (`std::function<void(std::optional<std::string>)>`) and pure virtual dialog methods in `platform.h` expose no SDL3 types to editor code. SDL3 types (`<SDL3/SDL_dialog.h>`, `SDL_DialogFileFilter`) only appear in `src/engine/platform/`. ✓
- **Thread safety approach is sound**: Mutex-protected `dialog_result_` written by SDL callback (off-thread), drained in `poll_events()` (main thread). Callback invokes `FileDialogCallback` on main thread after mutex unlock. ✓
- **Editor async flow well-specified**: `pending_dialog_result_` / `pending_dialog_action_` pattern correctly decouples Platform dialog callback from `draw_ui()` Phase 6 processing. The save-prompt state machine correctly dispatches to Platform dialog methods instead of `show_file_dialog_` flags. ✓
- **All ImGuiFileDialog references are removal instructions**: Every mention of ImGuiFileDialog in the contract describes what to remove or references the current state that must change. No instructions tell the Code Agent to USE or ADD ImGuiFileDialog. ✓
- **Test changes adequate**: UT-14 (PlatformHeadless dialog no-op) added. No pre-existing tests depend on ImGuiFileDialog directly. ✓

**Several non-blocking concerns identified** (see Warnings below): the `dialog_callback_` overwrite without guard, `dialog_filters_` lifetime bound to a single-concurrent-dialog assumption, and `pending_dialog_action_` as raw string (fragile). None are blocking — the contract's async flow and thread-safety are correct for the designed usage.

### Re-review #4 (2026-06-12 — Simplified Callback Design, No Mutex/Queue)

Re-reviewed the **updated** contract which replaces the mutex-based/queue-based design with a simplified direct-callback design:

- **Callback-based design is sound**: SDL3 dialog callbacks fire on the main thread during `SDL_PumpEvents`/`SDL_PollEvent` (inside `poll_events()`). The heap-allocated `std::function` is deleted by the SDL C-lambda after invocation. No mutex, no intermediate result queue, no thread-safety mechanism. This matches the spec's design (A-05, A-12).
- **No ImGuiFileDialog remains in the contract**: All 39 matches for ImGuiFileDialog/mutex/pending_dialog_result_/pending_dialog_action_/dialog_callback_ in the contract are removal instructions, negative statements ("no mutex"), or files-to-inspect descriptions of the current (pre-change) state. The Done criteria explicitly verify removal (items 5, 6, 22).
- **ADR-019 fully respected**: `FileDialogCallback` is `std::function<void(std::optional<std::string>)>` — no SDL3 types exposed to editor code. SDL3 includes (`<SDL3/SDL_dialog.h>`) are confined to `src/engine/platform/`. Editor code receives only `std::optional<std::string>` paths.
- **Editor async callback usage is safe**: Callbacks capture `this` (Editor pointer). `engine_->platform()` is used instead of `ctx.services.platform()` because callbacks fire during `poll_events()`, outside `draw_ui()` context. Editor methods (`open_scene()`, `save_scene_as()`, `show_error_modal()`) modify state between frames — safe because everything is single-threaded, main-thread.
- **`request_exit_next_frame_` correctly bridges the async→draw_ui gap**: Dialog callbacks that need to request exit (Save→Quit for untitled scenes) set `request_exit_next_frame_` instead of calling `ctx.request_exit()` (which requires a valid `EngineContext` not available during `poll_events()`). Phase 6 of `draw_ui()` checks and consumes this flag.
- **No architectural decisions left to the Code Agent**: Every code change is fully specified — exact snippets for Platform methods, exact member additions/removals for Editor, exact state machine code for `draw_pending_op_modal()`, exact callback implementations.
- **Nested callback chain is correct**: The save-prompt Save→Save As→Open chain (for untitled dirty scenes during Open) uses nested Platform dialog calls. The inner `show_open_file_dialog()` is called from the outer Save As callback, both firing during `poll_events()` — this is safe as both run on the main thread.
- **Done criteria item 22 explicitly verifies absence of thread-safety mechanisms**: `platform_sdl3.h` must have no `#include <mutex>`, no `dialog_mutex_`, `dialog_result_`, `dialog_callback_`, `dialog_pending_`, `dialog_filters_`, or `sdl_dialog_callback()`.

**No new blocking issues. All previously resolved blocking issues (B-01, B-02) remain resolved.**

**Verdict: ACCEPTED** — no remaining blocking issues.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **B-01 — Internal contradiction: `on_quit_` callback type vs. registration signature; dangling reference bug**
  - Item 32 declares `on_quit_` as `std::function<void()>` (no parameters) in `MenuBar`.
  - Item 33 registers the callback with signature `[this](be::EngineContext const& ctx) { ... }`, which does not match `std::function<void()>`.
  - The lambda captures `ctx` — a local `const&` parameter of `Editor::setup()`. If captured by reference, `ctx` dangles after `setup()` returns (lifetime ends). If captured by value, the ctx snapshot from setup is stale by the time the callback fires during `draw_ui()`.
  - **Required fix**: Either (a) change `on_quit_` type to `std::function<void(be::EngineContext const&)>` and call it from `MenuBar::draw_ui(ctx)` with the current frame's context (consistent with the shortcut handler pattern), or (b) have the Editor store an `EngineContext const*` and use that in a `void()` callback. The first approach is preferred as it matches the existing pattern (shortcut callbacks receive `EngineContext const&`).

- [x] **B-02 — Save on clean untitled scene: silent no-op instead of Save As redirect**
  - Item 48 adds `if (!dirty_) return {};` as the FIRST line of `save_scene()`, bypassing the subsequent file-path null check.
  - **Spec AC-06**: "Save triggers Save As (no-op redirect)" for untitled scenes — user must get a Save As dialog.
  - **UT-09**: expects `save_scene()` to return error when untitled (triggers the dialog in the callback).
  - **Scenario**: Editor launches (clean, untitled), user presses Ctrl+S. With item 48's early return, `save_scene()` returns success immediately — no Save As dialog. The user expects to be asked where to save.
  - **Required fix**: Move the early-return guard AFTER the untitled check, or change it to `if (!dirty_ && current_file_path_.has_value()) return {};`. The clean-skip should only apply when a file path already exists.

## Warnings

Non-blocking concerns for awareness:

### Pre-existing warnings (from ImGuiFileDialog-era reviews)

- **Missing automated test for OS close-request callback**: Done criteria item 17 says "Run UT-XX for close-request callback (if added) or verify via manual test." The close-request mechanism (Platform::set_on_close_request + SDL_EVENT_QUIT interception) is a significant behavioral change with non-trivial state interactions. A dedicated unit test (e.g., UT-12) should verify that (a) the callback returns `true` when `!dirty_`, (b) sets `pending_op_ = PendingOp::Quit` and returns `false` when dirty, (c) the subsequent flow handles Save/Discard/Cancel correctly. "Manual verification" is insufficient for CI-gated quality.

- **Missing null safety for `engine_` in scene operations**: Edge case table states "If `engine_` is null, methods should return an error." But items 21-23 (`open_scene`, `save_scene`, `save_scene_as`) access `engine_->registry()` and `engine_->assets()` without null guards. While `engine_` is non-null after `setup()`, defensive null checks would prevent crashes if these methods are called before setup. Add `if (!engine_) return make_error(Error::Category::InvalidArgument, "Editor not initialized");` to each public scene operation method.

- **Log channel tag mismatch with wiki north-star**: The wiki north-star section (`docs/wiki/editor/scene-management.md` line 60) states "All editor file operations are logged to the Console via the `Editor:Scene` log channel." The contract resolves the log tag to `"Editor"` (the existing tag). The contract's documentation impact section should note this mismatch for wiki update, or the implementation should use a `"Editor:Scene"` sub-tag (via `BUDDD_LOG_TAG` scoping or tag override).

- **Test UT-08 lacks concrete testing methodology**: UT-08 says "Use a mock context or check the exit_requested_ flag pattern" without specifying how to invoke the quit handler from a test. Tests can't easily trigger menu callbacks or shortcuts. The test should either (a) directly call the `pending_op_` mechanism and verify state, or (b) call `ctx.request_exit()` and check `ctx.is_exit_requested()`. The contract should provide more specific guidance.

- **UT-02 window title test incomplete**: Only 2 of 4 spec-defined scenarios are tested (untitled/clean, untitled/dirty). Titled/clean and titled/dirty assertions are missing. The contract specifies all 4 cases in the test table but the implementation may skip them. Flagged by governance review — remains unaddressed.

### New warnings (simplified direct-callback design — re-review #4)

- **`engine_` null safety not explicitly enforced for dialog callbacks**: The callback lambdas in `draw_pending_op_modal()` and menu/shortcut handlers access `engine_->platform()` without null checks. The pre-existing warning about null `engine_` in `open_scene`/`save_scene`/`save_scene_as` covers this, but the callbacks themselves would crash if `engine_` were null. In practice, `engine_` is always non-null after `setup()` and dialogs are only triggered after setup, but an `if (!engine_) return;` guard at the top of each callback would provide defense-in-depth.

- **`pending_op_` cleared before async dialog invocation may swallow cancellation context**: In the untitled dirty save-prompt flow (Step 7, `Save` case), `pending_op_` is set to `None` before `show_save_file_dialog()` is called. If the user cancels the Save As dialog, the original pending operation (e.g., OpenScene) is already cleared — the user stays on the current scene with no pending operation, which is the correct behavior (Cancel means abort everything). However, this means the original operation information is lost if the dialog is cancelled; a hypothetical "retry from last state" feature would need to preserve it. Not a blocking concern — the current behavior matches spec edge cases.

- **`this` capture in lambdas: Editor must outlive dialog callbacks**: All dialog callbacks capture `this` (raw pointer to Editor). The Editor is destroyed after `shutdown()` (managed by `EditorApp`). SDL3 dialogs close when the parent `SDL_Window` is destroyed — the Editor's `engine_` pointer provides platform access, and the platform is destroyed with `EngineService` after the Editor. The lifespan ordering must be: Editor outlives any outstanding SDL3 dialogs. This is guaranteed by the modal nature of SDL3 dialogs (dialog completes before the next frame's event processing) and the `EditorApp` lifecycle (Editor is destroyed before `EngineService`). Implicit but safe — documented in edge case table.

- **No test coverage for nested callback chain (Save As → Open)**: The save-prompt state machine for untitled dirty scenes during OpenScene involves two chained async callbacks (Save As completion → Open dialog). This is a non-trivial async flow that is not covered by any unit test (UT-14 only tests the no-op PlatformHeadless case). Testing this would require mocking the Platform dialog methods or using a real headless Platform with callbacks. The contract does not require such a test, which is acceptable given headless mode immediately calls the callback with nullopt (making the chain trivially testable at the platform level).

- **Wiki still describes ImGuiFileDialog as current implementation**: `docs/wiki/editor/scene-management.md` (lines 83-92) still references ImGuiFileDialog as the OS file dialog implementation. The contract's documentation impact section correctly identifies this as a required wiki update. Not a contract defect — the wiki will be updated by a subsequent agent when implementation proceeds.

## Checklist coverage

| Spec AC | Covered by contract steps | Covered by tests | Notes |
|---------|--------------------------|------------------|-------|
| AC-01 | ✅ Steps 5, 6, 9 | ⚠️ DT-01 (manual) | OS file dialog in menu/file dialog steps |
| AC-02 | ✅ Steps 4, 6 | ⚠️ DT-02 | Menu→Save through Editor callbacks |
| AC-03 | ✅ Steps 4, 6, 9 | ✅ UT-07 | save_scene_as + file path tracking |
| AC-04 | ✅ Steps 1, 3, 4 | ✅ UT-01, UT-02 | Dirty state + window title |
| AC-05 | ✅ Steps 4, 6 | ✅ UT-04, UT-05 | Dirty prompt via pending_op_ state machine |
| AC-06 | ✅ Steps 4, 6 | ✅ UT-03, UT-09 | ✅ B-02 resolved |
| AC-07 | ✅ Steps 4, 6, 7, 8 | ✅ UT-08 | Close-request + pending_op_ |
| AC-08 | ✅ Steps 4, 5 | ✅ IT-02 | Error modal + world preservation |
| AC-09 | ✅ Steps 4, 6 | ✅ UT-04, UT-05, UT-11 | new_scene behavior |
| AC-10 | ✅ Steps 4, 5, 9 | ✅ IT-01 | Round-trip save/load |

## Verdict

**ACCEPTED** — both blocking issues resolved. No new issues introduced.

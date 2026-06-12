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

- **Missing automated test for OS close-request callback**: Done criteria item 17 says "Run UT-XX for close-request callback (if added) or verify via manual test." The close-request mechanism (Platform::set_on_close_request + SDL_EVENT_QUIT interception) is a significant behavioral change with non-trivial state interactions. A dedicated unit test (e.g., UT-12) should verify that (a) the callback returns `true` when `!dirty_`, (b) sets `pending_op_ = PendingOp::Quit` and returns `false` when dirty, (c) the subsequent flow handles Save/Discard/Cancel correctly. "Manual verification" is insufficient for CI-gated quality.

- **Missing null safety for `engine_` in scene operations**: Edge case table states "If `engine_` is null, methods should return an error." But items 21-23 (`open_scene`, `save_scene`, `save_scene_as`) access `engine_->registry()` and `engine_->assets()` without null guards. While `engine_` is non-null after `setup()`, defensive null checks would prevent crashes if these methods are called before setup. Add `if (!engine_) return make_error(Error::Category::InvalidArgument, "Editor not initialized");` to each public scene operation method.

- **Log channel tag mismatch with wiki north-star**: The wiki north-star section (`docs/wiki/editor/scene-management.md` line 60) states "All editor file operations are logged to the Console via the `Editor:Scene` log channel." The contract resolves the log tag to `"Editor"` (the existing tag). The contract's documentation impact section should note this mismatch for wiki update, or the implementation should use a `"Editor:Scene"` sub-tag (via `BUDDD_LOG_TAG` scoping or tag override).

- **[RESOLVED by B-01 fix] Step 6 item 33: ctx captured by reference from setup() — potential dangling**: The B-01 fix changed the callback type to `std::function<void(be::EngineContext const&)>`, so ctx is now a parameter passed by MenuBar::draw_ui(ctx) — not a captured reference. No dangling risk. The Code Agent must ensure MenuBar::draw_ui() accepts ctx to pass to the callback.

- **Step 2: Duplicate `target_include_directories`**: Item 7 adds the same `${ImGuiFileDialog_SOURCE_DIR}` include directory twice (lines 153-157 and 167-172). While not harmful, the two blocks should be consolidated into one.

- **Test UT-08 lacks concrete testing methodology**: UT-08 says "Use a mock context or check the exit_requested_ flag pattern" without specifying how to invoke the quit handler from a test. Tests can't easily trigger menu callbacks or shortcuts. The test should either (a) directly call the `pending_op_` mechanism and verify state, or (b) call `ctx.request_exit()` and check `ctx.is_exit_requested()`. The contract should provide more specific guidance.

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

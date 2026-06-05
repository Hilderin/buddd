# SPEC-016 Review — Architecture Refactor: Navigable Object Graph (RenderDevice → Window → Platform → InputSystem)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **BLOCKING: `make_headless_device(w, h)` has a dangling-reference lifetime bug (Assumption 3, Q2 resolution, AC-013).** The spec proposes a helper that "constructs a full PlatformHeadless + WindowHeadless + RenderDeviceHeadless chain and returns a std::unique_ptr<RenderDeviceHeadless> (or similar owning type)." If the Platform and Window are local variables inside the helper, they are destroyed when the function returns, and the RenderDeviceHeadless would hold a dangling `Window&` reference. The "or similar owning type" is ambiguous and does not resolve the lifetime problem. The helper must either: (a) return a struct/container that owns all three objects, (b) accept output parameters, or (c) heap-allocate all three. Without this, the helper is unusable — every test that calls it would have undefined behavior.
    - **Resolution**: The `make_headless_device` helper has been replaced entirely by a proper `EngineService` class (`src/engine/engine_service.h`) that owns the full Platform→Window→RenderDevice chain via `unique_ptr` members. Tests use `EngineService::create(Backend::Headless, config)` which correctly manages lifetimes: Platform, Window, and RenderDevice are all heap-allocated and destroyed in the correct order. No dangling-reference bug is possible. ✓ Resolved.

- [x] **BLOCKING: AC-023 golden-string baseline is not anchored.** AC-023 says "verify outputs match pre-refactor golden strings" but does not specify where these golden strings are stored, how they are captured, or what the expected exact output is (stdout, stderr, exit code, window title, frame count). Without a reference artifact or embedded assertions, this criterion cannot be tested or verified. The spec must either: (a) define the expected output inline, (b) reference a committed golden file, or (c) replace with measurable assertions (e.g., "stderr contains 'Demo complete: triangle (120 frames rendered)'" — which already exists in the test suite).
    - **Resolution**: AC-023 has been replaced with a compilation-only check ("All four demo functions compile with `(RenderDevice&, int, const char* const*)` signatures"). The `[cli][demo]` subprocess tests in `demo_tests.cpp` that ran `buddd demo <name>` have been removed entirely (they pop up windows and interfere with the user's work). New AC-044 verifies that `demo_tests.cpp` no longer runs demos as subprocesses. Demo correctness is verified via compilation and the EngineService creation tests. ✓ Resolved.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- **WindowSDL3 cached `captured_` may desync from SDL state on focus loss (EC-012)**: EC-012 documents that SDL auto-releases relative mouse mode on focus loss. The cached `captured_` bool would still read `true`, temporarily inconsistent with SDL's actual state. The spec's resolution assumes the next right-click cycle corrects this (which it does for the demo use case), but `is_mouse_captured()` callers outside the demo context could get stale data. Not a blocker, but worth adding a comment in `WindowSDL3::is_mouse_captured()` or handling focus-gain events in the future.

- **`engine->device()` returns `RenderDevice&` (base class), not `RenderDeviceHeadless&` — diagnostic methods inaccessible**: Tests that use `RenderDeviceHeadless` diagnostic methods (`frame_begin_count()`, `frame_end_count()`, `draw_call_count()`) currently access them on a local `RenderDeviceHeadless device(800, 600)` variable. The spec says to replace these with `EngineService::create(Backend::Headless, config)` and use `engine->device()`. However, `EngineService::device()` returns `RenderDevice&` (the abstract base), not `RenderDeviceHeadless&`. Tests relying on Headless-specific diagnostics will need an alternative approach: adding virtual diagnostic methods to `RenderDevice`, using `dynamic_cast`, or maintaining some direct constructions. The spec does not address this transition. Acceptable as an implementation detail, but should be noted for the implementer.

- **AC-038 "invalid config" scope is vague**: AC-038 says "create with negative dimensions or other invalid config, verify error returned." The spec references `WindowConfig` validation in `Platform::create_window` (which checks `width>0 && height>0`). The "other invalid config" is open-ended; negative dimensions are the only clearly defined invalid case. Minor — the intent is clear from context.

- **`demo_command.cpp` forward declaration for `Platform`**: Assumption 4 says demo headers can remove the `class Platform;` forward declaration. However, some demo `.cpp` files may still use `Platform` types indirectly (e.g., through `input_system()` return type). The spec notes this ("unless it is still needed for internal includes"), which is correct but requires per-file verification during implementation.

## Required changes

None. Both blocking issues have been resolved to the reviewer's satisfaction.

## Suggested improvements

Optional ideas (not required):

- Consider documenting the WindowSDL3 cached-state-vs-SDL desync window in a comment or EC-012 note: `captured_` is set immediately on `set_mouse_capture(true/false)` but SDL may release asynchronously on focus loss. A future improvement could listen for `SDL_EVENT_WINDOW_FOCUS_LOST` to reset the cache.
- The `RenderDeviceOpenGL` constructor stores both `Window&` and `SDL_Window*`. Consider renaming the old `SDL_Window* window_` to `sdl_window_` in the "Detailed design" to avoid the naming collision the spec already flags.
- Consider whether `RenderDevice` base class should expose virtual diagnostic accessors (`frame_begin_count()`, `frame_end_count()`, `draw_call_count()`) to enable seamless EngineService migration without requiring `dynamic_cast` in tests.

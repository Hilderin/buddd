# Spec Review — SPEC-037: Editor Window Geometry Persistence

## Blocking issues

Items that must be resolved before the artifact can be accepted.

No blocking issues found.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- **Save algorithm pseudocode vs execution order mismatch**: The Save algorithm section (line 301) includes `settings_manager_->save_all()` inside the `if` block alongside the `set()` calls. However, the Execution order section (line 323) lists `save_all()` as a separate step 3 ("Existing code: settings_manager_->save_all()"). If the implementer follows the pseudocode literally, `save_all()` would be called twice. The execution order section is authoritative, but the two code blocks should be reconciled to avoid confusion.

- **`resize()` vs `on_resize()` naming**: The new `resize(w, h)` method (a request to change window size) sits alongside the existing `on_resize(w, h)` method (a notification callback when the window size actually changes). The semantic difference is clear from context but not explicitly documented, which could cause confusion during implementation. Consider adding a doc comment to `resize()` noting that it triggers a platform resize request (potentially generating a subsequent `on_resize()` event).

- **AC-016 verification approach**: AC-016 requires verifying that `set_position()` is "not invoked" when position validation fails. Testing a negative (a method was NOT called) typically requires a mock/spy pattern. The verification note "or invoked with a flag indicating no-op" suggests an alternative approach but doesn't specify it. The integration test approach (verifying final window position equals the default SDL3-centred position rather than checking call counts) would be simpler and more robust. Consider updating the verification description.

- **`resize()` returns `void` — SDL3 errors not propagated**: The spec documents that window resize failure (e.g., `SDL_SetWindowSize` returns false) should log a warning, but the new `resize()` method signature is `void` — errors cannot be propagated to the caller. This is a reasonable design choice for a void-returning method, but it should be called out explicitly in the method documentation or the error cases table.

- **`noexcept` not specified on new methods**: Existing `Window` methods like `width()` and `height()` are marked `noexcept`. The new methods (`position()`, `set_position()`, etc.) in the spec do not specify `noexcept`. The implementer should add `noexcept` for consistency where appropriate.

## Required changes

Concrete, actionable changes requested:

None — no blocking issues found. All warnings above are non-blocking suggestions.

## Suggested improvements

Optional ideas (not required):

- Consider adding an explicit assumption: "SDL3 returns the restored (un-maximized) position and size for a maximized window via `SDL_GetWindowPosition` and `SDL_GetWindowSize`." This documents why saving position/size when maximized is useful (they are recovered on un-maximize).
- The `DEFAULT_WIDTH` and `DEFAULT_HEIGHT` constants (1280, 800) should be defined in a single location (e.g., `editor.h` or a shared header) rather than repeated inline. The spec could suggest a named constant.

## Definition of Ready checklist

| Criterion | Status |
|---|---|
| **Clarity & Completeness** | |
| Scope is clearly defined (what is included and what is explicitly excluded) | ✅ |
| Dependencies on other features, modules, or external systems are identified | ✅ |
| Edge cases and error conditions are described | ✅ |
| The expected behavior is unambiguous and testable | ✅ |
| **Verification** | |
| The spec defines how the feature will be verified end-to-end | ✅ |
| Acceptance criteria are specific, measurable, and verifiable | ✅ |
| Success and failure states are described | ✅ |
| **Documentation** | |
| Interface changes (CLI flags, API signatures, config keys) are documented | ✅ |
| Existing documentation that must be updated is listed | ✅ |
| **Technical** | |
| Technical constraints are identified (system APIs, libraries, build changes) | ✅ |
| Risks or unknowns are surfaced | ✅ |
| Performance or resource implications, if any, are noted | ✅ |

**Overall assessment**: All Definition of Ready criteria satisfied. Spec is accepted.

## Review summary

SPEC-037 defines a clear, thorough, and well-structured feature for persisting editor window geometry. The validation algorithm is correctly specified, edge cases are comprehensive, and the 23 acceptance criteria are specific and testable. Types (`int32_t`, `std::string`) are compatible with the existing `SettingsStore` explicit template instantiations. The architecture respects ADR-019 (no SDL3 in editor code) through proper abstract interfaces. `PlatformHeadless` and `WindowHeadless` already exist — no new backend classes needed. The bootstrap ordering (window created by `EngineService::create()` before `Editor::setup()` applies settings) is correct. No blocking issues were found; the warnings above are non-blocking suggestions for implementation clarity.

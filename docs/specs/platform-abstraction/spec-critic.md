# Spec Review — SPEC-002: Platform Abstraction Layer (v3)

## Summary

Re-review of SPEC-002 after a batch of naming and API consistency changes:
- Class names migrated from underscore-separated to PascalCase-backend-suffixed
  (`Platform_SDL3` → `PlatformSDL3`, `Window_SDL3` → `WindowSDL3`,
   `RenderDevice_GL45` → `RenderDeviceOpenGL`).
- File names aligned: `render_device_gl45` → `render_device_opengl`.
- Added `make_error()` helper function as a project-wide standard for concise
  error construction.
- `Error` struct uses `int code = 0` default member initializer (consistent
  with the `make_error` default parameter).

The review checked internal consistency of all naming changes, proper
integration of the new `make_error()` helper, absence of contradictions
across sections, continued correctness of all Acceptance Criteria, and
alignment with the constitution and SPEC-001 conventions.

**Verdict: Accepted with warnings** — no new blocking issues introduced by
the changes. The two pre-existing warnings remain unaddressed but are
non-blocking.

## Status

`Accepted with warnings`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

*None. All checks passed:*

- **Naming consistency** ✅ — Every occurrence of old names
  (`Platform_SDL3`, `Window_SDL3`, `RenderDevice_GL45`,
  `render_device_gl45`) has been replaced with the new names
  (`PlatformSDL3`, `WindowSDL3`, `RenderDeviceOpenGL`,
  `render_device_opengl`). Confirmed via grep — zero stale references.
  The conventions section (line 59) uses the new names as examples.

- **`make_error()` integration** ✅ — The helper is:
  - Defined with the correct signature in the Goals section
    (`make_error(Category, message, code = 0)` returning
    `std::unexpected<Error>`).
  - Declared identically in AC-005.
  - Used consistently in Story 2 (lines 120, 124) and in all Error cases
    (lines 199–206).
  - The return type `std::unexpected<Error>` is correct for the
    `Result<T>` / `std::expected<T, Error>` pattern.

- **`Error` struct `code = 0` default** ✅ — The default member initializer
  `int code = 0` appears consistently in the Goals section (line 34),
  AC-005 (line 162), and A-09 (line 252). No contradictions.

- **All ACs remain correct** ✅ — AC-001 through AC-015 use the new class
  and file names. Every AC still has a feasible verification method. No AC
  relies on removed or renamed symbols.

- **No contradictions with constitution or SPEC-001** ✅ — The naming
  conventions (PascalCase classes, `snake_case` files) match SPEC-001.
  The testing requirements (CONST-002) are satisfied by the AC verification
  methods. The architecture boundary rule (CONST-001 is still TODO, but the
  spec's AC-015 pre-figures it appropriately).

## Warnings

- [ ] **W-01 — Headless backend observability logs are implementation-defined.**
  The headless backend's `begin_frame()` / `end_frame()` are no-ops. The
  observability table (line 221) mentions logging for backend selection and
  init success/failure but does not specify whether headless `begin_frame` /
  `end_frame` should log or remain silent. Non-blocking — the implementation
  may choose as appropriate.

- [ ] **W-02 — `WindowConfig` has no default member initializers.**
  The struct (`title: std::string`, `width: int`, `height: int`) requires all
  three fields at every construction site. Adding sensible defaults
  (e.g., `width = 800`, `height = 600`, `title = "Buddd Engine"`) would
  improve ergonomics. Non-blocking — the spec explicitly states (A-08) that
  "no builder pattern is needed at this stage", and the implementation
  contract stays aligned.

## Required changes

*None.*

## Suggested improvements

- The file naming convention table (line 54) specifies individual source
  files (`platform_sdl3.cpp`, `render_device_opengl.h`) but does not
  enumerate the headless backend file names (`platform_headless.cpp`,
  `window_headless.cpp`, `render_device_headless.cpp`). Consider adding
  them for completeness, though the convention is clear enough by
  extrapolation.

- The `make_error()` helper's default parameter `code = 0` is redundant
  with the `Error` struct's `int code = 0` default member initializer.
  This is harmless, but a spec purist might choose to keep only one default
  to avoid confusion about which "wins". (In practice, the function default
  takes precedence, so the behavior is well-defined either way.)

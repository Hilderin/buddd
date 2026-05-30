# ADR Assessment — Framebuffer Capture (SPEC-010)

**Author:** ADR agent
**Date:** 2026-05-30
**Status:** Assessment complete — no new ADR required.

---

## Summary

The Framebuffer Capture feature (SPEC-010) introduces a new `src/engine/image/` module,
a `read_pixels()` pure virtual on `RenderDevice`, new `Error::Category` values (`ReadbackFailed`,
`IoFailed`), a stb_image/stb_image_write dependency, a `buddd capture` CLI command, and
a "cube" capture scenario. Every architectural decision in this feature follows established
patterns documented in existing ADRs. No new ADR is warranted.

---

## Question-by-question analysis

### 1. Does adding `src/engine/image/` as a new module warrant an ADR?

**No.** The module follows existing conventions without introducing new architectural rules:

- Lives under `src/engine/` — respects CONST-001 architecture boundary.
- Namespace `buddd::engine` — consistent with all engine code.
- Uses `Result<T>` for all fallible APIs — per ADR-001.
- Uses static factory methods (`create`, `load`) — same pattern as `Platform::create()`,
  `RenderDevice::create()`.
- `ImageBuffer` is a pure aggregate struct — no new type pattern.
- `Image` is non-copyable, movable — same as `Platform`, `RenderDevice`, `Shader`, etc.

The module is small (2 headers, 1 implementation file, ~185 lines total) and functionally
scoped. It does not establish new module-structure conventions; it follows the existing
one-directory-per-subsystem pattern already established by `src/engine/math/`,
`src/engine/render/`, `src/engine/scene/`, `src/engine/platform/`, etc.

**Precedent:** ADR-002 documented the *math wrapper pattern* because it introduced a novel
wrapper design (`reinterpret_cast` + triple `static_assert`). The image module does not
introduce any novel pattern — it uses plain value types with standard factory methods.

---

### 2. Does `read_pixels()` contradict ADR-003?

**No.**

ADR-003 establishes a narrow exception: `draw()` and `draw_indexed()` return `void` instead
of `Result<T>` because they are on a **performance-sensitive hot path** where per-call error
checking is impractical.

`read_pixels()` returns `Result<ImageBuffer>` — it follows ADR-001 (the standard `Result<T>`
pattern), not the ADR-003 exception. This is explicitly correct because:

- `read_pixels()` is called at most **once per frame**, not thousands of times.
- Failure of `read_pixels()` is a meaningful runtime error (GL error, I/O error) that the
  caller must handle, not a precondition violation.
- The implementation contract explicitly notes this (line 64):
  > *"Draw methods return void. `read_pixels()` returns `Result<ImageBuffer>` (it is not on
  > a hot path and its failure is not a precondition violation)."*

The spec does note one consistency with ADR-003: calling `read_pixels()` outside a
`begin_frame()`/`end_frame()` pair is **undefined behaviour** in the OpenGL backend, which
is consistent with ADR-003's precondition-UB contract for `draw()`. This is an application
of the same principle (precondition UB for graphics state violations), not an extension of
the ADR-003 exception.

**Verdict:** No contradiction. The ADR-003 carveout remains strictly scoped to
`draw()`/`draw_indexed()`.

---

### 3. Does adding stb as a dependency warrant an ADR?

**No.** Adding a new third-party dependency via the existing FetchContent mechanism is an
incremental change, not an architectural decision, for the following reasons:

- **Same mechanism**: Uses `FetchContent_Declare` + `FetchContent_MakeAvailable` — identical
  to SDL3, GLM, and Catch2.
- **Same location**: Declared in `src/engine/CMakeLists.txt`, consistent with all other
  dependencies.
- **Header-only**: Like GLM, stb is header-only — no compiled library, no build-mode concern
  (ADR-007 is not relevant).
- **Public-domain license**: Permissive, consistent with the project's existing dependency
  licenses (MIT for SDL3/GLM, BSL for Catch2).
- **Pinned version**: Uses a specific git commit hash — consistent with the existing pattern
  (SDL3 uses a tag, GLM uses a tag, stb uses a commit hash).

The project already established the pattern for adding FetchContent dependencies. Adding
one more is not architecturally notable.

---

### 4. Is there any contradiction with existing ADRs?

| ADR | Contradiction? | Explanation |
|-----|---------------|-------------|
| ADR-001 (`Result<T>`) | ✅ None | `read_pixels()`, `Image::create()`, `load()`, `save()` all return `Result<T>`. New `Error::Category` values (`ReadbackFailed`, `IoFailed`) are additive — ADR-001 explicitly says the enum is extensible. |
| ADR-002 (GLM wrapper) | ✅ None | Image module does not use GLM. No impact on the wrapper pattern. |
| ADR-003 (Render pipeline) | ✅ None | `read_pixels()` returns `Result<T>` (not void). Precondition UB for outside-begin_frame/end_frame is consistent with ADR-003's approach but does not extend the exception. |
| ADR-004 (Demo system) | ✅ None | Capture lives in `src/cmd/capture/` as a separate CLI command (`buddd capture`), not as a demo under `src/cmd/demo/`. This is consistent — capture is a distinct operation mode, not a visual demonstration. |
| ADR-005 (`optional<T&>`) | ✅ None | Not relevant. |
| ADR-006 (RTTI dispatch) | ✅ None | Not relevant. |
| ADR-007 (Release deps) | ✅ None | stb is header-only — no build-mode decision needed. |
| ADR-008 (Docker CI) | ✅ None | No CI changes needed. |
| ADR-009 (Test naming) | ✅ None | Image tests use `image_tests.cpp` — plural suffix, correct per ADR-009. |

**Verdict:** Zero contradictions.

---

### 5. Is keeping stb as PRIVATE and not exposing it in public headers architecturally notable?

**No — it follows existing precedent.** This decision is important for correct encapsulation,
but it is not a *new* architectural decision. It is an application of the principle already
established by ADR-002:

> ADR-002, lines 123-128:
> *"This wrapper pattern establishes a template for any future dependency wrapping in the project:
> - Expose project-namespaced types in the public API.
> - Hide the external dependency inside implementation files or wrapper headers."*

The image module follows this template precisely:

- `image.h` (public header) contains **no stb includes** — zero stb types or macros leak out.
- `image.cpp` is the sole translation unit with `#include "stb_image.h"` and
  `#include "stb_image_write.h"`, each preceded by its `IMPLEMENTATION` macro.
- CMake adds `stb_SOURCE_DIR` as a **PRIVATE** include directory (line 42 of
  `src/engine/CMakeLists.txt`), so stb headers are invisible to downstream targets.
- The code review explicitly confirms this: `"image.h has NO stb includes"`.

This is a correct application of an existing pattern, not a new pattern. It would be
redundant to create an ADR that says "we should continue doing what we already decided
to do."

---

## Conclusion

**No new ADR is required.** The Framebuffer Capture feature (SPEC-010):

1. ✅ Follows established module conventions — no new structural pattern.
2. ✅ Does not contradict ADR-003 — `read_pixels()` correctly returns `Result<T>`.
3. ✅ Adds a dependency via the existing FetchContent mechanism — incremental, not architectural.
4. ✅ Does not contradict any existing ADR — verified across all nine ADRs.
5. ✅ Encapsulates stb behind project types — follows ADR-002's established wrapper pattern.

All architecturally significant decisions were already made in:
- **ADR-001** (error handling via `Result<T>`)
- **ADR-002** (wrapping external dependencies behind project types)
- **ADR-003** (draw-call precondition UB, which `read_pixels()` partially references
  but does not extend)
- **ADR-004** (CLI command structure, which `CaptureCommand` follows)

The implementation contract (`docs/specs/capture/implementation-contract.md`) and spec
(`docs/specs/capture/spec.md`) already document the design rationale, architecture boundary
compliance, and relationship to existing ADRs (see spec sections A-01, A-02, Q-05;
implementation contract references to ADR-001, ADR-003).

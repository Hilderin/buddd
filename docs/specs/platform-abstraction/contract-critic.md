# Implementation Contract Review — IMPL-002: Platform Abstraction Layer

## Status

`Accepted`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

---

## Review cycle history

| Cycle | Date | Verdict | Key changes |
|-------|------|---------|-------------|
| 1 | 2026-05-29 | `Rejected` | Initial review. Found 1 blocking issue (B-01: missing SDL3 include in `platform.cpp` code block) and 3 warnings. |
| 2 | 2026-05-29 | `Accepted` | All 4 issues from Cycle 1 resolved. No new issues found. |
| 3 | 2026-05-29 | `Rejected` | Re-review after naming changes (`Platform_SDL3` → `PlatformSDL3`, `RenderDevice_GL45` → `RenderDeviceOpenGL`, `render_device_gl45.*` → `render_device_opengl.*`), `make_error()` helper added, all `std::unexpected(Error{...})` → `make_error(...)`, CMake changed to `GLOB_RECURSE ... CONFIGURE_DEPENDS`, tests T-09/T-10/T-11 added. Found 1 new blocking issue (B-02: SDL_CreateWindow error message contradicts spec). |
| 4 | 2026-05-29 | `Accepted` | Fresh re-review after fixes for B-02, W-04, W-05, W-06. All fixes verified correct. No new issues found. |

---

## Summary

The IMPL-002 implementation contract has received a fresh comprehensive review from scratch. All four previously-flagged items (B-02, W-04, W-05, W-06) have been verified as correctly fixed. No new blocking issues or warnings have been identified.

The contract is now internally consistent, consistent with the accepted spec SPEC-002 (within the spec's own margins of ambiguity), and consistent with the constitution. The level of detail is prescriptive enough to prevent the Code Agent from making uncontrolled architectural decisions.

**Verdict change reason (Cycle 3 → 4):** B-02 is confirmed fixed (line 365 now correctly uses `SDL_GetError()`). W-04, W-05, W-06 are all confirmed fixed. No new issues found in fresh review.

---

## Blocking issues

Items that must be resolved before the artifact can be accepted.

### Cycle 1 (resolved)

- [x] **B-01 — Missing `#include <SDL3/SDL.h>` in `platform.cpp` code block.**
  **Resolution:** Added `#include <SDL3/SDL.h>` to the `platform.cpp` code block. Verified present. ✅

### Cycle 2 (resolved)

- [x] No new blocking issues found in Cycle 2. ✅

### Cycle 3 (resolved)

- [x] **B-02 — `platform_sdl3.cpp` SDL_CreateWindow failure returns wrong error message.**
  **Location:** `implementation-contract.md` §5 `platform_sdl3.cpp`, line 365.
  **Original problem:** The message `"Invalid window dimensions"` was a copy-paste from the dimension-validation error above. It should contain the actual SDL error via `SDL_GetError()`.
  **Fix verified (line 365–366):**
  ```cpp
  return make_error(Error::Category::WindowCreationFailed,
      "SDL_CreateWindow failed: " + std::string(SDL_GetError()));
  ```
  This now correctly matches:
  - Spec SPEC-002 Error cases table (line 200): `"SDL_CreateWindow failed: <details>"`
  - Contract's own §5 requirements text (line 379): "return `make_error(Error::Category::WindowCreationFailed, ...)` with the SDL error message"
  - Contract's own edge-case table (line 937): `"SDL_CreateWindow failed: ..."`
  ✅ **Resolved.**

### Cycle 4 (this review)

- [x] No new blocking issues found. ✅

---

## Warnings

Non-blocking concerns for awareness.

### Cycle 1 (resolved)

- [x] **W-01 — Unused `#include <string_view>` in `error.h`.**
  **Resolution:** Removed from `error.h` code block. ✅

- [x] **W-02 — Headless backend window-creation observability missing.**
  **Resolution:** Added `std::cerr << "Window created (Headless): " << config.width << "x" << config.height << "\n";` to `platform_headless.cpp` code block. ✅

- [x] **W-03 — Edge-case table over-promised `SDL_GL_SetAttribute` failure handling.**
  **Resolution:** Edge-case text updated to: *"`SDL_GL_SetAttribute` return values are not checked individually. If an attribute fails, `SDL_GL_CreateContext` will fail shortly after, producing `RenderDeviceCreationFailed` with the SDL error message."* ✅

### Cycle 2 (resolved)

- [x] No new warnings found in Cycle 2. ✅

### Cycle 3 (resolved)

- [x] **W-04 — `make_error` missing from the API compatibility / public API surface section.**
  **Location:** `implementation-contract.md` §"API compatibility impact" (line 963).
  **Fix verified:** `make_error` is now listed:
  ```cpp
  auto make_error(Error::Category, std::string, int code = 0) -> std::unexpected<Error>;
  ```
  ✅ **Resolved.**

- [x] **W-05 — Done criteria wording "lists all new sources" conflicts with `GLOB_RECURSE` strategy.**
  **Location:** `implementation-contract.md` §"Done criteria", item 2 (line 1021).
  **Fix verified:** Now reads:
  > "The updated file uses `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` to collect all sources, includes FetchContent for SDL3…"
  ✅ **Resolved.**

- [x] **W-06 — Requirement text in §5 still referenced `std::unexpected` instead of `make_error`.**
  **Location:** `implementation-contract.md` §5, requirements bullet (line 379).
  **Fix verified:** Now reads:
  > "On failure, return `make_error(Error::Category::WindowCreationFailed, ...)` with the SDL error message."
  ✅ **Resolved.**

### Cycle 4 (this review)

- [x] No new warnings found. ✅

---

## Required changes

**None.** All previously identified issues are resolved. The contract is ready for implementation to proceed.

---

## Suggested improvements

Optional ideas (not required, for reviewer awareness only):

- **Observability output format:** The spec's Observability table (§"Observability") suggests the platform init failure output should be `std::cerr << "Platform init failed: " << to_string(error) << "\n"`. The contract outputs `std::cerr << "Platform init failed: SDL_Init failed: " << SDL_GetError() << "\n"` instead. The contract's output is more concise and equally informative. This is consistent with the spec's non-prescriptive language ("All observability uses `std::cerr` directly…") and was accepted in prior cycles. No change needed.

- **Spec-internal inconsistency on `RenderDeviceCreationFailed` message:** The accepted spec has three different phrasings for the OpenGL context creation error: Story 2 says `"SDL_GL_CreateContext failed"`, the Error cases table (line 201) says `"SDL_GL context creation failed: <details>"`, and the Error cases table (line 205) says `"OpenGL 4.5 Core not available"`. The contract consistently uses `"SDL_GL_CreateContext failed: " + std::string(SDL_GetError())`, which is a reasonable resolution (combining Story 2's prefix with the Error cases table's intent to include details). No change needed.

- **`make_error` default parameter redundancy:** The `make_error` helper has a default `code = 0` parameter, and the `Error` struct has a default member initializer `code{0}`. Both defaults are harmless; the function default takes precedence. This was noted in the spec review (spec-critic.md) and is consistent here.

---

## Cross-reference checks

### Consistency with SPEC-002

| Spec area | Contract implementation | Verdict |
|-----------|------------------------|---------|
| Three abstract classes (Platform, Window, RenderDevice) | `platform.h`, `window.h`, `render_device.h` — all pure virtual, no SDL3/GL types exposed | ✅ |
| `Backend` enum (`SDL3`, `Headless`) | Defined in `platform.h`, used in `Platform::create(Backend)` | ✅ |
| `Error` struct + `Result<T>` alias + `make_error` helper | `error.h` with `Category` enum (5 values), `int code{0}`, `std::string message`, `Result<T> = std::expected<T, Error>`, `make_error(Category, string, code=0)` | ✅ |
| `Platform::create(Backend)` factory | Static method, creates `PlatformSDL3` or `PlatformHeadless` via `new` (friend access) | ✅ |
| `RenderDevice::create(Window&)` dispatch | Checks `native_handle()` for `nullptr` → headless, otherwise → OpenGL via `static_cast<SDL_Window*>` | ✅ |
| SDL3 backend: `PlatformSDL3`, `WindowSDL3`, `RenderDeviceOpenGL` | All three present with specified code | ✅ |
| Headless backend: no SDL3/OpenGL includes | Verified: `*_headless.*` files have no SDL3/GL includes | ✅ |
| `SDL_Init` called in `Platform::create()` (not in constructor) | Correctly placed in `platform.cpp` before `new PlatformSDL3()` | ✅ |
| Non-copyable, non-movable | All three interfaces have `= delete` for copy/move ctor/assignment | ✅ |
| Observability via `std::cerr` | Present for all creation paths (platform, window, render device) for both backends | ✅ |
| `make_error(Category, message, code=0)` → `std::unexpected<Error>` | `error.h` line 172 matches spec §34 and AC-005 | ✅ |
| **SDL_CreateWindow failure returns SDL error message** | **§5 code block uses `SDL_GetError()` — matches spec** | **✅** |
| `Error::Category` values match spec | All five values present: `InitFailed`, `WindowCreationFailed`, `RenderDeviceCreationFailed`, `Unsupported`, `Unknown` | ✅ |
| `to_string()` format `"<Category>: <message> (code <code>)"` | `to_string()` implementation produces this exact format | ✅ |
| `WindowConfig` has no default initializers (matching spec A-08) | Struct has bare `title`/`width`/`height`, no defaults | ✅ |

### Acceptance criteria coverage (SPEC-002)

| AC ID | Description | Contract coverage | Status |
|-------|-------------|-------------------|--------|
| AC-001 | Abstract `Platform` class with `create(Backend)` factory | §2 (`platform.h`) | ✅ |
| AC-002 | `WindowConfig` struct with title/width/height | §8 (`window.h`) | ✅ |
| AC-003 | Abstract `Window` class with width/height/native_handle | §8 (`window.h`) | ✅ |
| AC-004 | Abstract `RenderDevice` with `create(Window&)`, begin/end_frame, size | §13 (`render_device.h`) | ✅ |
| AC-005 | `Error` struct with Category, code=0, message; `to_string()`; `Result<T>`; `make_error` | §1 (`error.h`) | ✅ |
| AC-006 | `Backend` enum with SDL3, Headless | §2 (`platform.h`) | ✅ |
| AC-007 | `PlatformSDL3`, `SDL_Init` in create, `SDL_Quit` in destructor | §§4–5 (`platform_sdl3.*`) | ✅ |
| AC-008 | `WindowSDL3` via `SDL_CreateWindow` with `SDL_WINDOW_OPENGL` | §10 (`window_sdl3.cpp`) | ✅ |
| AC-009 | `RenderDeviceOpenGL` via `SDL_GL_CreateContext` | §§15–16 | ✅ |
| AC-010 | OpenGL 4.5 Core profile + debug context in debug builds | `#ifndef NDEBUG` guard in `render_device.cpp` | ✅ |
| AC-011 | `begin_frame()` → `glClear`, `end_frame()` → `SDL_GL_SwapWindow` | §16 (`render_device_opengl.cpp`) | ✅ |
| AC-012 | Headless backend classes, no SDL3/OpenGL | §§6–7, 11–12, 17–18 | ✅ |
| AC-013 | Compiles without warnings | Done criteria #3 | ✅ (specified) |
| AC-014 | Non-copyable, non-movable | All three interfaces | ✅ |
| AC-015 | Architecture boundary (no SDL3/GL leaks in public headers) | Done criteria #5 (grep command provided) | ✅ |

### Constitutional alignment

| Rule | Check | Status |
|------|-------|--------|
| CONST-001-architecture-boundaries | Placeholder "TODO" text — no active rule. Contract references this correctly and adds its own boundary enforcement (Done criteria #5). | ✅ N/A |
| CONST-002-testing-policy | Unit tests for all testable code required. Contract specifies tests T-01 through T-13 covering all headless paths and conditional SDL3 tests. | ✅ |
| CONST-003-documentation-policy | Placeholder "TODO" text — no active rule. | ✅ N/A |
| CONST-004-security-policy | Placeholder "TODO" text — no active rule. | ✅ N/A |
| Principles: Prefer explicit contracts | Contract is highly prescriptive — every code line, test, and edge case specified. No room for accidental design deviation. | ✅ |
| Principles: Prefer testable requirements | All tests have specific verification criteria (T-01 through T-13). | ✅ |

### Edge-case coverage (spec cross-reference)

| Spec edge case | Contract edge case | Covered? |
|----------------|--------------------|----------|
| Platform created, no window before destruction | "Platform created but no window created before destruction" | ✅ |
| Window created, no render device before destruction | "Window created but no render device before destruction" | ✅ |
| Render device created, window destroyed first | "Render device created and window destroyed before render device" | ✅ |
| `Platform::create()` called twice | "`Platform::create()` called twice without destroying the first instance" | ✅ |
| Headless window native handle | "Headless window native handle accessor" | ✅ |
| Multiple windows from single platform | "Multiple `create_window()` calls on same Platform" | ✅ |
| Window size zero/negative | "`WindowConfig.width` or `height` ≤ 0" | ✅ |
| Window size larger than desktop | Not mentioned in contract — spec says "no specific error reported" | Minor omission — non-blocking |
| `WindowConfig` title empty string | "`WindowConfig.title` empty string" | ✅ |
| SDL3 `SDL_CreateWindow` fails | "SDL_CreateWindow failed: ..." (matches code) | ✅ |
| `SDL_GL_SetAttribute` failure | `SDL_GL_SetAttribute` return values not checked individually | ✅ |
| OpenGL 4.5 Core profile not available | `SDL_GL_CreateContext` fails and returns `RenderDeviceCreationFailed` | ✅ |

### Naming and API consistency verification

| Check | Result |
|-------|--------|
| `Platform_SDL3` → `PlatformSDL3` in all code blocks | ✅ Zero old references |
| `Window_SDL3` → `WindowSDL3` in all code blocks | ✅ Zero old references |
| `RenderDevice_GL45` → `RenderDeviceOpenGL` in all code blocks | ✅ Zero old references |
| `render_device_gl45.*` → `render_device_opengl.*` in file lists | ✅ Updated |
| CMakeLists.txt uses `GLOB_RECURSE ... CONFIGURE_DEPENDS` | ✅ Present at line 863 |
| `error.h` picked up by GLOB (at `src/engine/error.h`) | ✅ GLOB_RECURSE catches all `*.h` under `src/engine/` |
| `make_error` signature: `(Category, string, code=0) → std::unexpected<Error>` | ✅ Matches spec AC-005 |
| All `std::unexpected(Error{...})` → `make_error(...)` | ✅ Zero remaining raw patterns |
| Test IDs sequential T-01 through T-13 | ✅ 13 tests, all sequential, T-09/T-10/T-11 for `make_error` |
| `make_error` in public API surface section | ✅ Listed at line 963 |
| Done criteria item 2 references GLOB_RECURSE | ✅ Updated at line 1021 |
| §5 requirement text uses `make_error` | ✅ Updated at line 379 |

### Code correctness notes

| Check | Result |
|-------|--------|
| `error.h` `to_string()` switch covers all 5 enum values | ✅ |
| `make_error` inline definition prevents ODR violations | ✅ |
| `make_error` passes arguments in correct order to `Error{category, code, std::move(message)}` | ✅ (matches `Error(Category, int, string)` constructor) |
| `platform.cpp` includes `<SDL3/SDL.h>` | ✅ |
| `render_device.cpp` gets SDL3 transitively via `render_device_opengl.h` | ✅ (compilable) |
| `#include <GL/gl.h>` present in `render_device_opengl.cpp` | ✅ |
| `Result<T>` = `std::expected<T, Error>` definition correct | ✅ |
| Friend declarations match `Platform::create(Backend)` signature | ✅ |
| `WindowSDL3::native_handle()` casts `SDL_Window*` to `void*` | ✅ |
| `WindowHeadless::native_handle()` returns `nullptr` | ✅ |
| `PlatformSDL3::create_window` validates width/height | ✅ |
| `PlatformHeadless::create_window` validates width/height | ✅ |
| All `static_assert`-ready non-copyable/non-movable patterns | ✅ |
| CMake `FetchContent` uses `GIT_TAG release-3.2.30` | ✅ |
| CMake links `SDL3::SDL3` and `OpenGL::GL` PUBLIC | ✅ |
| **SDL_CreateWindow error message correct** | **✅ Now uses `SDL_GetError()`** |
| No `default` in `Platform::create()` switch (catches missing enum values) | ✅ Return after switch handles unknown values |
| `friend` declaration uses full trailing return type signature | ✅ Consistent across both `PlatformSDL3` and `PlatformHeadless` |
| No copy/move operations on any abstract class | ✅ All three abstract classes have `= delete` |
| Tests specify `REQUIRE`/`REQUIRE_FALSE` (not `CHECK`) | ✅ Non-functional requirements section |
| Tests explicitly assigned to test-author, not implementation-author | ✅ Line 892 |

---

## Questions for the human

No open questions. All previously identified issues are resolved. The contract is ready for implementation to proceed.

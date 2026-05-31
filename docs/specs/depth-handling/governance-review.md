# Governance Review — Depth Buffer Support for OpenGL Renderer (SPEC-012 / IMPL-012)

## Status

`Approved`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec → Implementation contract consistency**: All 14 acceptance criteria from SPEC-012 are fully reflected in IMPL-012 with concrete verification methods (static inspection, `diff`, existing tests, manual demo). The debug-build-only `glGetError()` check that was flagged by the spec critic (AC-010/Q-02 tension) is now resolved: AC-010 was updated to require a debug-build-only `glGetError()` check with `std::cerr` logging, and IMPL-012 §3 specifies the exact implementation. → **Resolved — consistent**.
- [x] **Implementation contract → code consistency**: The actual `git diff` matches IMPL-012 precisely:
  - `render_device.cpp`: `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)` inserted after the `#endif` of the debug flag block, before `SDL_GL_CreateContext`. Debug-only log present. Call is unconditional. → **Matches contract exactly**.
  - `render_device_opengl.cpp` constructor: `glEnable(GL_DEPTH_TEST)`, `glDepthFunc(GL_LESS)`, debug-only log, debug-only `glGetError()` check (clear-then-read pattern). → **Matches contract exactly**.
  - `render_device_opengl.cpp begin_frame()`: `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`. → **Matches contract exactly**.
  - No other files modified. → **Matches contract exactly**.
  - [x] **Code review → code consistency**: Code review confirms all changes match the implementation. All 14 ACs met. → **Consistent**.
- [x] **Contract critic → code consistency**: The contract critic noted that `to_hex_string` IS visible from the constructor (contradicting the contract's rationale). The implementation correctly prints the raw integer value, which is simpler. The code does not rely on the incorrect rationale. → **Not a cross-doc contradiction; implementation is correct**.
- [x] **Wiki → spec consistency**:
  - `docs/wiki/architecture/overview.md`: Updated to mention 24-bit depth buffer allocation, `GL_DEPTH_TEST` with `GL_LESS`, and references SPEC-012. Accurately reflects the spec. → **Consistent**.
  - `docs/wiki/architecture/data-flow.md`: Platform lifecycle diagram updated to show `SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24)`, `RenderDeviceOpenGL` constructor enabling `GL_DEPTH_TEST`, and `GL_DEPTH_BUFFER_BIT` in the clear call. References SPEC-012 and IMPL-012 added. → **Consistent**.
- [x] **Spec → Wiki references**: SPEC-012 and IMPL-012 are referenced in both wiki files. → **Consistent**.
- [x] **Spec → constitution**: SPEC-012 explicitly states "architecture boundary (CONST-001) is unaffected — all changes are inside `src/engine/render/`" and the spec includes a Permissions and Security section. → **Consistent**.
- [x] **Implementation contract → done criteria**: The contract's 20+ done criteria checklist items are all verified by the code review and build/test results. → **All satisfied**.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Constitution violations

Checks against `docs/constitution/**`:

| Rule | Status | Evidence |
|------|--------|----------|
| **CONST-001** — Architecture Boundaries | ✅ **Compliant** | All changes inside `src/engine/render/`. No SDL3/OpenGL types leak outside the engine boundary. The abstract `RenderDevice` interface (`render_device.h`) is unchanged — confirmed by `git diff` showing zero changes to `render_device.h`, `render_device_opengl.h`, `render_device_headless.h`, `render_device_headless.cpp`. AMEND-2026-001 (SDL3 test file exception) is not triggered. |
| **CONST-002** — Testing Policy | ✅ **Compliant** | All new behavior is verifiable by: (a) static inspection for OpenGL state (AC-001 through AC-004, AC-011, AC-014), (b) existing SDL3 backend tests that exercise the modified code paths (tests #207–#212: SDL3 RenderDevice creation, SDL3 frame cycle — both pass), (c) manual visual verification via `buddd capture cube --frame 120`. The spec was accepted with the explicit understanding that depth testing requires a display-backed OpenGL context and cannot be tested via the headless backend. This is an inherent limitation of graphics-level changes. No new depth-specific automated tests were added — this was accepted during spec review. |
| **CONST-003** — Documentation Policy | ✅ **Compliant** | The constitution rule body is `TODO`. No documentation files outside the spec/contract/wiki are modified. The wiki has been updated to reflect the current state. |
| **CONST-004** — Security Policy | ✅ **Compliant** | The constitution rule body is `TODO`. No security impact: standard OpenGL/SDL3 API calls only (`SDL_GL_SetAttribute`, `glEnable`, `glDepthFunc`, `glGetError`, `glClear`). No elevated privileges, no I/O, no network access, no new dependencies. All changes are inside `src/engine/render/` which already has access to SDL3 and OpenGL headers. |

**Engineering principles** (from `docs/constitution/principles.md`):

| Principle | Compliance |
|-----------|------------|
| Prefer explicit contracts over implicit assumptions | ✅ `glDepthFunc(GL_LESS)` is set explicitly (not relying on driver default). All changes documented with exact code snippets. |
| Prefer small scoped changes over broad rewrites | ✅ 28 lines added, 2 removed, exactly 2 files modified. Scope strictly limited to depth buffer. |
| Prefer existing conventions over new patterns | ✅ Follows `#ifndef NDEBUG` guards, `std::cerr` logging, `SDL_GL_SetAttribute` unchecked return pattern, `glGetError()` clear-first pattern. |
| Prefer testable requirements over vague intent | ✅ All 14 ACs are testable via static inspection, existing tests, or manual visual verification. |
| Governance documents must not contradict each other | ✅ No contradictions found across any of the 8+ documents reviewed. |

- [x] No constitution violations found.

## ADR alignment

Required ADRs exist or are proposed:

| ADR | Status | Assessment |
|-----|--------|------------|
| **ADR-001** — `Result<T>` / `Error` Pattern | ✅ **Compliant** | No new fallible operations introduced. `SDL_GL_SetAttribute` return value not checked — consistent with pre-existing pattern in same file. |
| **ADR-002** — GLM Wrapper Math | ✅ **Compliant** | No GLM changes. Depth buffer is a pure OpenGL feature — no math wrapper impact. |
| **ADR-003** — Render Pipeline Architecture | ✅ **Compliant** | Draw methods remain `void`. Draw path is unchanged. The depth changes are transparent to all render pipeline abstractions. |
| **ADR-004** — Demo System Architecture | ✅ **Compliant** | No demo files modified. Cube demos benefit automatically from depth testing because `RenderDeviceOpenGL` now enables it during construction. |
| **ADR-005** — `std::optional<T&>` Component API | ✅ **Compliant** | No scene graph changes. Not affected. |
| **ADR-006** — RTTI Component Dispatch | ✅ **Compliant** | No component changes. Not affected. |
| **ADR-007** — Release Dependency Build | ✅ **Compliant** | No build system changes. CMakeLists.txt not modified. |
| **ADR-008** — Docker CI Infrastructure | ✅ **Compliant** | No CI changes. |
| **ADR-009** — Test File Naming Convention | ✅ **Compliant** | No test files created or renamed. |
| **ADR-010** — No Raw Pointers in Public API | ✅ **Compliant** | No public API changes at all. |
| **ADR-011** — (empty file) | ✅ **Not applicable** | File has no content; no impact on this work. |

- [x] No new ADR is required. The implementation follows all existing ADR decisions without requiring new exceptions.
- [x] No constitution update is required. The implementation complies with all existing rules.

## Wiki alignment

Wiki reflects current state and does not become law:

| File | Change | Assessment |
|------|--------|------------|
| `docs/wiki/architecture/overview.md` | Updated `begin_frame()` description to mention `GL_DEPTH_BUFFER_BIT`. Added sentence about 24-bit depth buffer allocation and `GL_DEPTH_TEST` with SPEC-012 reference. | ✅ Accurately reflects implementation. Does not introduce new requirements or constitutional rules. Descriptive only. |
| `docs/wiki/architecture/data-flow.md` | Updated platform lifecycle diagram: `[OpenGL 4.5 Core context created with 24-bit depth buffer]`, `RenderDeviceOpenGL constructor enables GL_DEPTH_TEST (GL_LESS)`, `glClear(GL_COLOR_BUFFER_BIT \| GL_DEPTH_BUFFER_BIT)`. Added SPEC-012 and IMPL-012 references. | ✅ Accurately reflects implementation. Does not introduce new requirements. |

- [x] Wiki reflects current operational state without becoming law. No contradictions with constitution, ADRs, or spec.
- [x] The wiki does not claim any authority beyond its position at authority order #4 (below constitution, specs, and ADRs per AGENTS.md).

## Warnings

Non-blocking concerns for awareness:

1. **No automated depth-specific OpenGL state queries**: The implementation adds `glEnable(GL_DEPTH_TEST)`, `glDepthFunc(GL_LESS)`, and `GL_DEPTH_BUFFER_BIT` to the clear mask, but there is no automated test that queries OpenGL state (`glIsEnabled(GL_DEPTH_TEST)` or `glGetIntegerv(GL_DEPTH_FUNC)`) to confirm the configuration at runtime. This was accepted during spec review as unavoidable without a display-backed test. A future hardening pass should consider adding such coverage.

2. **No automated test for the `glGetError()` warning path**: The debug-build-only `glGetError()` check in the constructor (lines 92–99 of `render_device_opengl.cpp`) has no test that deliberately injects a GL error and verifies the warning message on `std::cerr`. The error-handling code path is untested. This was deferred by the spec's open question Q-02 resolution. A future spec may add comprehensive GL error checking across the renderer.

3. **`to_hex_string` visibility rationale in IMPL-012 is factually incorrect**: The implementation contract states (line 97) that `to_hex_string` "may not be visible at this point in the file" when warning against its use in the constructor. In fact, `to_hex_string` is defined in the anonymous namespace at lines 68–72, which precedes the constructor at lines 80–101 — it IS visible. The recommended approach (printing the raw integer) is still valid; only the stated rationale is wrong. This does not affect the code, which correctly prints the raw integer.

4. **CONST-002 justification relies on existing test coverage**: The argument that depth-specific behavior is adequately tested by existing SDL3 backend tests and static inspection is defensible but weakens the testing guarantee. The spec was accepted with this justification, but a reader could reasonably argue that CONST-002's "None" exceptions column is being stretched here. Mitigation: the spec's Assumptions already document why no new test file is created (headless has no depth concept, display-backed testing is out of scope).

5. **The "additive changes" claim in IMPL-012 (§Files allowed to change) is technically imprecise**: The contract states "only additive changes (no existing statements are removed)." The constructor change replaces `{}` with `{ ... }`, which technically deletes the `{}` and inserts a multi-line body. The spirit of "additive" is understood (no existing logic is removed), but the phrasing was clarified in response to the contract critic (suggestion 4). The actual git diff confirms `+22/-2` lines — two lines were removed (the `{}`) and 24 lines were added.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

- **None required.** The implementation complies with all existing constitution rules and ADRs. The wiki has been updated to reflect the current state. No constitution amendments or new ADRs are needed.
- **Recommended (non-blocking):** If the implementation contract is revised in the future, correct the `to_hex_string` visibility rationale (see Warning #3).

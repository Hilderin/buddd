# Governance review: SPEC-010 — Framebuffer Capture

## Status

`Accepted with warnings`

## Summary

This is a **re-review after debugging changes** applied to the original SPEC-010 implementation. The debugging changes introduced several practical fixes and workarounds that, while functionally correct and constitutional, deviate from the accepted spec and implementation contract. The constitution is respected, all 190/190 tests pass, and the core feature (framebuffer capture) works correctly.

**Key validation results:**
- ✅ **Constitution compliance**: CONST-001 (architecture boundaries) fully satisfied — zero SDL3/OpenGL/GLM includes in `src/cmd/`. CONST-002 (testing policy) satisfied — 190/190 tests pass.
- ⚠️ **Spec/contract accuracy**: The spec and implementation contract are **outdated** relative to the current implementation. The debugging changes (`--frame N`, rotation animation, minimum 2-frame workaround, `capture_cube_scene` 5-parameter signature) are not reflected in these documents. A spec update or implementation-contract amendment is needed.
- ⚠️ **Wiki accuracy**: **Inaccurate** in two places — both `docs/wiki/architecture/overview.md` and `docs/wiki/architecture/module-map.md` incorrectly state that the capture scenario uses camera position `(3,2,3)` when the actual code uses `(0,0,3)` as originally specified. The wiki otherwise correctly documents `--frame N`, `glReadBuffer(GL_BACK)`, `glClearColor`, and the minimum 2-frame workaround.
- ✅ **ADR compliance**: Zero contradictions with ADR-001 through ADR-009.
- ✅ **Camera position**: Correctly at `(0,0,3)` — matches the original spec (fix applied).
- ✅ **AC-015 extra args warning**: Restored (fix applied).

### What changed since last governance review

The previous governance review (`Accepted`, 2026-05-30) was based on the original implementation that strictly followed the spec. The debugging changes made after that review introduced:

| Change | Impact on spec/contract |
|--------|------------------------|
| `--frame N` CLI parameter (default: 1) | Violates non-goal "No multi-frame capture" |
| Rotation animation (0.5 rad/s around Y) when N > 1 | Violates "angle = 0" and "deterministic capture" |
| Internal minimum 2 frames (driver quirk workaround) | Violates "exactly one frame" |
| `capture_cube_scene` signature: 5 params instead of 4 | Violates contract API |
| `glReadBuffer(GL_BACK)` before `glReadPixels` | Not in spec/contract (pragmatic fix, wiki-documented) |
| `glClearColor(0.02f, 0.02f, 0.05f, 1.0f)` in `begin_frame()` | Not in spec/contract (cosmetic, wiki-documented) |
| Camera restored to `(0,0,3)` (was incorrectly changed to `(3,2,3)`) | ✅ Fixed — matches spec |
| Extra args warning restored | ✅ Fixed — AC-015 compliant |

**The spec and contract need to be formally updated** to reflect the debugging changes. An ADR documenting the `--frame N` scope addition, the minimum 2-frame driver quirk, and the `glReadBuffer(GL_BACK)` / `glClearColor` changes would provide an auditable record of these decisions.

---

## Constitution compliance

### CONST-001 (Architecture Boundaries)

**Verdict: ✅ Compliant**

The architecture boundary is rigorously respected:

- **Zero matches**: `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` — **zero matches**.
- `ImageBuffer` and `Image` live entirely within `src/engine/image/` — engine layer.
- `RenderDevice::read_pixels()` is a pure virtual on the abstract `RenderDevice` in `src/engine/render/`.
- `RenderDeviceOpenGL::read_pixels()` calls `glReadPixels` + `glReadBuffer(GL_BACK)` inside `src/engine/render/` — permitted.
- `RenderDeviceHeadless::read_pixels()` returns an error inside `src/engine/render/` — permitted.
- `CaptureCommand` in `src/cmd/commands/` uses only engine abstractions (`Platform`, `Window`, `RenderDevice`, `Image`, `ImageBuffer`).
- `capture_cube_scene` in `src/cmd/capture/` uses only engine abstractions and forward declarations — no backend-type exposure.
- `glReadBuffer(GL_BACK)` is called inside `src/engine/render/render_device_opengl.cpp` — within the engine layer, permitted.

**AMEND-2026-001 (SDL3 test file exception):** Not triggered. The capture tests are CPU-only or headless. `capture_command.cpp` uses `be::Backend::SDL3` through the abstract `Platform::create()` factory, not through direct SDL3 includes.

### CONST-002 (Testing Policy)

**Verdict: ✅ Compliant**

All testable code has corresponding unit tests, and all tests pass:

| Test file | Tests | Status |
|---|---|---|
| `tests/image_tests.cpp` | IT-01 through IT-12 (12 tests) | ✅ All pass |
| `tests/render_device_tests.cpp` | RT-01 (1 test) | ✅ Passes |
| `tests/cmd_tests.cpp` | CT-01, CT-02, CT-03 (3 tests) | ✅ All pass |
| Pre-existing tests (174 tests) | All prior test suites | ✅ Unchanged, all pass |
| **Total** | **190 tests** | ✅ **100% pass rate** |

---

## Spec/contract accuracy

**Verdict: ⚠️ Outdated — spec and contract do not reflect current implementation**

The debugging changes introduced several deviations from the accepted spec (SPEC-010) and implementation contract (IMPL-010). These deviations are functional improvements/workarounds, but they are not documented in the authoritative spec or contract.

### Items that match spec/contract

| Item | Status |
|------|--------|
| `ImageBuffer` aggregate struct | ✅ Matches spec |
| `Image` class with `create()`, `load()`, `save()` | ✅ Matches spec |
| `Image::create()` validation and row-flipping | ✅ Matches spec |
| `Image::save()` writes valid PNG, round-trips | ✅ Matches spec |
| `RenderDevice::read_pixels()` pure virtual | ✅ Matches spec |
| OpenGL `read_pixels()` calls `glReadPixels` with `GL_PACK_ALIGNMENT=1` | ✅ Matches spec (plus `glReadBuffer(GL_BACK)`) |
| Headless `read_pixels()` returns `Unsupported` error | ✅ Matches spec |
| `CaptureCommand` files exist | ✅ Matches spec |
| `main.cpp` has `"capture"` dispatch branch | ✅ Matches spec |
| Camera at `(0,0,3)` | ✅ Matches spec (was incorrectly `(3,2,3)`, now fixed) |
| AC-015 extra args warning | ✅ Restored (matches DemoCommand pattern) |
| `Error::Category::ReadbackFailed` and `IoFailed` | ✅ Matches spec |
| `capture/*.cpp` in CMake glob | ✅ Matches spec |
| Help includes `capture` | ✅ Matches spec |
| All existing demos unchanged | ✅ Matches spec |
| CONST-001 compliance | ✅ Matches spec |
| Build succeeds with stb via FetchContent | ✅ Matches spec |

### Items that deviate from spec/contract

| Item | Spec/contract says | Impl says | Severity |
|------|-------------------|-----------|----------|
| `--frame N` parameter | **Non-goal**: "No multi-frame capture or video recording" | `--frame N` parameter defaults to 1, supports N > 1 with rotation animation | ⚠️ Breaking non-goal — scope addition |
| Rotation angle | angle = 0 (axis-aligned, deterministic) | 0.5 rad/s rotation around Y when N > 1 | ⚠️ Breaks reproducibility guarantee |
| Frames rendered | "Exactly one frame" (A-08) | Minimum 2 frames (driver quirk workaround) | ⚠️ Breaks "exactly one frame" |
| `capture_cube_scene` signature | 4 params: `(Platform&, RenderDevice&, int, int)` | 5 params: `(..., int num_frames = 1)` | ⚠️ API deviation from contract |
| `glReadBuffer(GL_BACK)` | Not mentioned | Added in `read_pixels()` before `glReadPixels` | ✅ Pragmatic fix (no contradiction) |
| `glClearColor` | Not mentioned | Added `(0.02, 0.02, 0.05, 1.0)` in `begin_frame()` | ✅ Cosmetic additive change |
| Usage text | No `--frame` flag | Includes `--frame N` in usage | ⚠️ Spec text mismatch |
| Observability output | `"Capturing: cube\n"` | `"Capturing: cube (1 frame(s))\n"` with frame count | ⚠️ Minor output format change |

### Spec/contract update recommendations

The spec and contract should be updated to reflect the current implementation. Specifically:

1. **Remove or amend the non-goal** "No multi-frame capture or video recording" — the `--frame N` parameter now supports multi-frame capture.
2. **Update A-08 and A-13**: Document that >1 frame renders with rotation animation, that frame 1 has a driver quirk, and that the Nth frame is captured.
3. **Add `--frame N` to the command signature** in the spec and usage text.
4. **Update `capture_cube_scene` function signature** to include `int num_frames`.
5. **Document the `glReadBuffer(GL_BACK)` call** as a required implementation detail for double-buffered readback.
6. **Document the `glClearColor` change** in `begin_frame()`.

---

## ADR compliance

**Verdict: ✅ Fully compliant — zero contradictions**

| ADR | Assessment | Status |
|---|---|---|
| **ADR-001** (Result<T> pattern) | `read_pixels()`, `Image::create()`, `Image::load()`, `Image::save()` all return `Result<T>`. New `Error::Category` values (`ReadbackFailed`, `IoFailed`) are additive. | ✅ |
| **ADR-002** (GLM wrapper) | Image module does not use GLM. stb follows the wrapper pattern: project-namespaced types in public API, dependency hidden behind implementation files. | ✅ |
| **ADR-003** (Render pipeline — draw returns void) | `read_pixels()` returns `Result<ImageBuffer>`, not `void` — correctly so because it is not on a hot path. Precondition UB for calling outside `begin_frame()`/`end_frame()` is consistent with ADR-003's approach. | ✅ |
| **ADR-004** (Demo system architecture) | `CaptureCommand` follows the same pattern as `DemoCommand`. Capture scenarios in `src/cmd/capture/` mirror demo-file pattern. | ✅ |
| **ADR-005** through **ADR-009** | Not relevant to this feature. Verified: no contradictions. | ✅ |

**Recommendation**: Consider adding a new ADR documenting:
- The decision to add `--frame N` (scope addition to SPEC-010)
- The minimum 2-frame driver quirk workaround and its root cause
- The `glReadBuffer(GL_BACK)` and `glClearColor` changes to `render_device_opengl.cpp`

---

## Wiki accuracy

**Verdict: ⚠️ Inaccurate** — two pages have incorrect camera position for capture scenario

### `docs/wiki/architecture/overview.md`

| Claim | Status |
|-------|--------|
| Directory layout includes `image/` under `src/engine/` | ✅ Correct |
| External dependencies list stb | ✅ Correct |
| Engine internal structure shows `image/` module | ✅ Correct |
| Key behaviors includes `capture` command | ✅ Correct, includes `--frame N` |
| `capture` camera at `(3,2,3)` (line 121) | ❌ **WRONG** — should be `(0,0,3)` |
| `glClearColor` documented (line 131) | ✅ Correct |
| `glReadBuffer(GL_BACK)` documented (line 132) | ✅ Correct |
| `begin_frame()` clear color documented | ✅ Correct |
| Reference includes SPEC-010 and IMPL-010 | ✅ Correct |

### `docs/wiki/architecture/module-map.md`

| Claim | Status |
|-------|--------|
| `Error::Category` includes `ReadbackFailed` and `IoFailed` | ✅ Correct |
| `capture_command.h/.cpp` documented | ✅ Correct |
| `cube_capture.h/.cpp` documented | ✅ Correct |
| Subcommand behavior includes `capture` | ✅ Correct, includes `--frame N` |
| `cube_capture` camera at `(3,2,3)` (line 169) | ❌ **WRONG** — should be `(0,0,3)` |
| Min 2-frame workaround documented | ✅ Correct |
| Rotation animation documented | ✅ Correct |
| Reference includes SPEC-010 and IMPL-010 | ✅ Correct |

### `docs/wiki/architecture/data-flow.md`

| Claim | Status |
|-------|--------|
| CLI dispatch includes `"capture"` branch | ✅ Correct |
| CLI output table includes capture command with `--frame N` | ✅ Correct |
| Error::Category includes `ReadbackFailed` and `IoFailed` | ✅ Correct |
| `glReadBuffer(GL_BACK)` / `glClearColor` in lifecycle diagram | ✅ Correct |
| Reference includes SPEC-010 and IMPL-010 | ✅ Correct |

### `docs/wiki/architecture/dependency-map.md`

- All claims about stb, targets, and constraints remain accurate. ✅

### `docs/wiki/engineering/testing.md`

| Claim | Status |
|-------|--------|
| Image/capture tests section present | ✅ Correct |
| CLI tests include capture test cases (CT-01, CT-02, CT-03) | ✅ Correct |
| Reference includes SPEC-010 and IMPL-010 | ✅ Correct |

### Wiki fix recommendations

1. **`docs/wiki/architecture/overview.md` line 121**: Change `(camera at (3,2,3))` to `(camera at (0,0,3))` for the capture scenario description.
2. **`docs/wiki/architecture/module-map.md` line 169**: Change `from camera position (3,2,3)` to `from camera position (0,0,3)` for the cube capture scenario.

---

## Detailed verification

### CONST-001 boundary scan

```
$ grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/
→ Zero matches. ✅
```

All source files in `src/cmd/` use only engine abstractions:
- `capture_command.cpp`: Includes engine types only (`Platform`, `Window`, `RenderDevice`, `Image`, `ImageBuffer`).
- `cube_capture.cpp`: Includes engine types only (`Platform`, `RenderDevice`, and math types from `buddd::engine::math`).
- `cube_capture.h`: Forward-declares `Platform` and `RenderDevice` from `buddd::engine` — no backend types exposed.

### Test results: 190/190 pass

```
100% tests passed, 0 tests failed out of 190
Total Test time (real) = 7.83 sec
```

All existing tests (174 pre-existing + 12 image tests + 1 render device test + 3 CLI tests = 190 total) pass.

### Acceptance criteria coverage

| AC ID | Description | Status | Verified by |
|-------|-------------|--------|-------------|
| AC-001 | `ImageBuffer` aggregate exists | ✅ | File exists, compiles |
| AC-002 | `Image` class with `create()`, `load()`, `save()` | ✅ | File exists, compiles |
| AC-003 | `Image::create()` validates dimensions | ✅ | IT-02, IT-03 pass |
| AC-004 | `Image::create()` flips rows | ✅ | IT-04 passes |
| AC-005 | `Image::save()` writes valid PNG, round-trips | ✅ | IT-05 passes |
| AC-006 | `read_pixels()` pure virtual | ✅ | Code review + RT-01 |
| AC-007 | OpenGL `read_pixels()` calls `glReadPixels` | ✅ | Code review + `glReadBuffer(GL_BACK)` added |
| AC-008 | Headless `read_pixels()` returns error | ✅ | RT-01 passes |
| AC-009 | `CaptureCommand` files exist | ✅ | Files exist |
| AC-010 | `main.cpp` dispatch | ✅ | Code review |
| AC-011 | `buddd capture cube /path` produces PNG | ✅ | Manual (requires display) |
| AC-012 | Default output path | ✅ | Manual (requires display) |
| AC-013 | No args prints usage, exits 1 | ✅ | CT-01 passes |
| AC-014 | Unknown scenario prints error, exits 1 | ✅ | CT-02 passes |
| AC-015 | Extra args warning | ✅ | Restored — follows DemoCommand pattern (`argc > 4` triggers warning) |
| AC-016 | Fails fast on unknown scenario | ✅ | CT-02 exits before `Platform::create()` |
| AC-017 | PNG is 800×600 | ✅ | Window size unchanged |
| AC-018 | Build succeeds with stb | ✅ | Build verified |
| AC-019 | No SDL3/OpenGL/GLM in `src/cmd/` | ✅ | Grep — zero matches |
| AC-020 | Existing demos work | ✅ | Frame cycle tests pass |
| AC-021 | `ReadbackFailed` in `Error::Category` | ✅ | Code review |
| AC-022 | `capture/*.cpp` in CMake glob | ✅ | Code review |
| AC-023 | `Image::load()` fails on missing file | ✅ | IT-06 passes |
| AC-024 | `capture_cube_scene()` reuses `setup_cube()` | ✅ | Code review |
| AC-025 | `buddd help` includes `capture` | ✅ | CT-03 passes |

### AC-015 extra args warning — detailed analysis

The spec's AC-015 states: `"buddd capture cube extra_arg" prints a warning to stderr about unexpected arguments but still captures and exits 0.`

With the original spec command signature `buddd capture <scenario> [output_path]`, `extra_arg` is consumed as the output path (argc=4, argv[3]="extra_arg"). The DemoCommand pattern triggers extra args warnings on `argc > 4`. Therefore `buddd capture cube extra_arg` (argc=4) does **not** trigger the warning in either the original contract or the current code — `extra_arg` is treated as the output path.

The current code correctly restores the warning pattern from `demo_command.cpp` for `argc > 4` cases. The AC-015 wording is inherently inconsistent with the spec's own command signature. This is a pre-existing spec inconsistency, not a code regression.

---

## Issues

### Blocking issues

None.

The constitution is fully respected (CONST-001, CONST-002). 190/190 tests pass. The camera is correctly at `(0,0,3)`. The AC-015 extra args warning is restored. The core functionality (framebuffer capture, PNG output, row-flipping) is verified correct.

### Non-blocking issues

- [ ] **Spec and implementation contract are outdated (requires update)**: The debugging changes (`--frame N`, rotation animation, minimum 2-frame driver quirk, `capture_cube_scene` 5-parameter signature, `glReadBuffer(GL_BACK)`, `glClearColor`) are not reflected in `docs/specs/capture/spec.md` or `docs/specs/capture/implementation-contract.md`. These documents must be updated to match the current implementation. See "Spec/contract update recommendations" above.

- [ ] **Wiki camera position inaccuracy — `docs/wiki/architecture/overview.md` line 121**: States `"(camera at (3,2,3))"` for the capture scenario. The actual code (`cube_capture.cpp:35`) uses `(0.0f, 0.0f, 3.0f)`. **Fix**: Change to `(camera at (0,0,3))`.

- [ ] **Wiki camera position inaccuracy — `docs/wiki/architecture/module-map.md` line 169**: States `"from camera position (3,2,3)"` for `cube_capture`. Should be `(0,0,3)`. **Fix**: Change to `(0,0,3)`.

- [ ] **New ADR recommended**: Consider creating an ADR documenting the debugging changes and their rationale: (a) `--frame N` scope addition and multi-frame support, (b) minimum 2-frame driver quirk workaround, (c) `glReadBuffer(GL_BACK)` and `glClearColor` changes to the OpenGL backend.

- [ ] **No automated test for `--frame` parsing**: The `parse_frame_count()` function in `capture_command.cpp` is a pure computation (no display, no IO) and could be unit tested independently. Edge cases: normal values, overflow, missing value, non-numeric, zero, negative.

- [ ] **No automated test for minimum-2-frame workaround**: The internal minimum-2-frame logic in `cube_capture.cpp:52` is not tested. A headless test that verifies `num_frames=1` produces the same output as `num_frames=2` (or that the readback succeeds with 2 frames) could detect regressions in the workaround.

- [ ] **Unused `Scenario` struct in `capture_command.cpp`**: The anonymous namespace defines a `struct Scenario` that is still unused. Dead code carried forward from the original implementation.

- [ ] **`<memory>` include in `image.h`**: Included in the contract's `image.h` specification but `Image` does not directly use any type from `<memory>` (no `std::unique_ptr`, `std::shared_ptr`, etc.). Harmless but unnecessary.

- [ ] **`render_device_tests.cpp` not explicitly listed in wiki `testing.md`**: The headless `read_pixels()` test (RT-01) exists and passes, but the wiki's test documentation tables do not explicitly enumerate `render_device_tests.cpp`. Minor documentation gap.

---

## Verdict

**Accepted with warnings.**

The Framebuffer Capture feature (SPEC-010) passes all constitutional checks and all 190 tests pass. The debugging changes are functionally correct and the constitution is fully respected. However, the spec and implementation contract are outdated relative to the current implementation, and the wiki has camera-position inaccuracies in two places.

### Summary of findings

| Dimension | Status |
|-----------|--------|
| **Constitution (CONST-001)** | ✅ Compliant — zero SDL3/OpenGL/GLM in `src/cmd/` |
| **Constitution (CONST-002)** | ✅ Compliant — 190/190 tests pass |
| **Spec accuracy** | ⚠️ Outdated — `--frame N`, rotation, min 2 frames not reflected |
| **Contract accuracy** | ⚠️ Outdated — same issues + signature mismatch |
| **Wiki accuracy** | ⚠️ 2 camera-position inaccuracies |
| **ADR compliance** | ✅ Zero contradictions |
| **Camera position** | ✅ Correctly `(0,0,3)` |
| **AC-015 extra args warning** | ✅ Restored |
| **Cross-document coherence** | ⚠️ Spec/wiki mismatch on camera; spec/code mismatch on multi-frame |

### Required actions before final close

1. **Update `docs/specs/capture/spec.md`** to reflect the debugging changes (`--frame N`, rotation, min 2 frames, updated function signatures, updated usage text, removed/modified non-goal).
2. **Update `docs/specs/capture/implementation-contract.md`** to match the updated spec and current implementation.
3. **Update `docs/wiki/architecture/overview.md`** line 121: fix camera position from `(3,2,3)` to `(0,0,3)`.
4. **Update `docs/wiki/architecture/module-map.md`** line 169: fix camera position from `(3,2,3)` to `(0,0,3)`.
5. **Consider creating an ADR** documenting the debugging changes and their rationale.

### Resolution history

| Date | Event | Verdict |
|------|-------|---------|
| 2026-05-30 | Initial governance review | `Accepted` |
| 2026-05-30 | Re-review after debugging changes | **`Accepted with warnings`** — 4 documentation gaps, 0 blocking issues |

**Reviewed by**: Governance Reviewer Agent
**Date**: 2026-05-30

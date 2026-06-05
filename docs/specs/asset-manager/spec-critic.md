# Spec Review — Asset Manager (SPEC-019)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

*All 8 previously reported blocking issues have been resolved in the latest revision.*

- [x] **BLOCKING-1: `RenderDevice::create_material()` API mismatch.** — **RESOLVED.** The spec now defines both overloads: the new `create_material(shared_ptr<ShaderProgram>)` (line 539) and the existing `create_material(unique_ptr<Shader>, unique_ptr<Shader>, span<const string>)` (line 543). The new overload is used in the `create<MaterialAsset>()` pseudo-code (line 335). Both signatures are clearly documented.

- [x] **BLOCKING-2: Location and CONST-001 compliance of `ShaderProgram` is ambiguous.** — **RESOLVED.** The spec explicitly states `ShaderProgram` is defined in `src/engine/render/shader_program.h` (line 86, line 502). The rationale (exposes `GLuint`, respects CONST-001) is documented. File listing A-14 includes `src/engine/render/shader_program.h` and `.cpp`.

- [x] **BLOCKING-3: Texture hot-reload strategy is contradictory (Strategy A vs B).** — **RESOLVED.** The contradictory material is removed. Flow 2 (lines 694–728) now describes a single, consistent "mutable Texture handle swap" approach: mutate the existing Texture object in-place via a `swap_handle()` method, no material iteration needed.

- [x] **BLOCKING-4: AC-023 is untestable as specified.** — **RESOLVED.** AC-023 (line 992) is now a headless-only test (pipeline verification via `ShaderProgram::testing_handle()`). AC-025 (line 994) is the OpenGL counterpart, guarded by `BUDDD_HAS_DISPLAY`, verifying GPU handle change. AC-024 covers failure recovery in headless mode.

- [x] **BLOCKING-5: AC-025/AC-026 require internal state inspection.** — **RESOLVED.** AC-026 (line 995) and AC-027 (line 996) now explicitly specify `#ifdef BUDDD_TESTING` test-only accessor (`get_dependency_map() const`). This is the chosen approach, clearly stated.

- [x] **BLOCKING-6: yaml-cpp exception safety is stated but not reflected in pseudo-code.** — **RESOLVED.** Both `create<TextureAsset>()` (lines 228–237) and `create<MaterialAsset>()` (lines 290–299) pseudo-code now have proper `try-catch` blocks around `YAML::LoadFile()`, catching `YAML::Exception` and `std::exception`.

- [x] **BLOCKING-7: `FileWatcher` base class methods are not declared `virtual`.** — **RESOLVED.** `poll_events()`, `start()`, and `stop()` now have `virtual` keyword and `= 0` (lines 615–621). Destructor is `virtual` (line 613).

- [x] **BLOCKING-8: `EngineService::render_device()` naming conflicts with existing `EngineService::device()`.** — **RESOLVED.** All references now use the existing `device()` accessor (line 773, line 779). No `render_device()` method name appears in the spec body. The `render_device` matches in the file listing (lines 1162–1164) refer to existing file names (`render_device.h`, `render_device_opengl.h`, etc.), not a method.

## Additional fixes verified

- [x] **Duplicate AC-022 removed.** AC-022 has been deleted. AC-018 remains. No duplicate.
- [x] **Thread safety documented.** Section 10 (lines 793–804) now documents thread safety for `clear()`, `set_file_watcher_enabled()`, `poll_file_events()`, `create<T>(id)`, `base_path()`, and all other methods.
- [x] **Open Questions table updated.** All previously open questions (Q-01 through Q-07) are documented as resolved in the Open Questions section (lines 1173–1181).

## New issues

No new blocking issues found.

## Warnings

Non-blocking concerns for awareness:

- **W-01 (Shader handle resolution in Materials):** The spec says the new `ShaderProgram` handle is seen "automatically" by Materials (line 553). This depends on Materials holding `shared_ptr<ShaderProgram>` (not a cached `GLuint`). The spec makes this clear, but implementers should confirm `Material::bind()` dereferences the `shared_ptr` at bind time rather than caching the raw GL handle.

- **W-02 (Texture settings not applied in V1):** Texture settings (`wrap_s`, `wrap_t`, etc.) are parsed and validated but NOT applied in V1. GPU texture creation uses current defaults. This is documented but worth flagging for implementers to avoid wiring up settings application prematurely.

- **W-03 (FileWatcher inotify watch limit):** The spec notes that `inotify_add_watch` may fail if the system watch limit is exceeded (line 1039). Asset loading continues but hot-reload coverage may be reduced. Documented but not testable in CI without specific kernel configuration.

- **W-04 (Microsecond timing):** SC-005 (line 1010) requires `poll_file_events()` to return in microsecond range when empty. This is a performance goal that is difficult to verify reliably across CI hardware.

## Required changes

None — all previous blocking issues are resolved.

## Suggested improvements

Optional ideas (not required):

- Consider adding an AC for empty-ID validation (currently in Edge cases line 1062 but not in AC table).
- The `ShaderProgramKey` hash (line 490: XOR with shift) could produce collisions for swapped paths. Consider using a proper hash combination pattern (`hash_combine`).
- Consider noting that `poll_file_events()` is non-reentrant (already stated in Assumption A-10 and Edge case line 1027, but worth restating in the method's API docs).

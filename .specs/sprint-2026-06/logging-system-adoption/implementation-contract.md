# IMPL-022 — Logging System Adoption (Ad-hoc stdout/stderr Migration)

## Source spec

- `.specs/sprint-2026-06/logging-system-adoption/spec.md`

## Goal

Migrate ~202 ad-hoc diagnostic output statements (`std::cerr`, `fprintf(stderr, ...)`, `printf(...)`, `write(STDERR_FILENO, ...)`) across 31 source files in `src/engine/` and `src/cmd/` to the `BUDDD_LOG_*` macro-based logging system. Each modified file receives a `BUDDD_LOG_TAG` declaration at file scope and an `#include "log/log.h"` if not already present. Each statement is mapped to the correct log level (ERROR, WARN, INFO, or DEBUG) per the spec's level mapping table. Pre-`Logger::init` bootstrap code in `main.cpp` (line 37) and user-facing help/usage text blocks are preserved as `fprintf(stderr)`. The result is a single, consistent, controllable diagnostic output path with level filtering, source tagging, and thread safety.

## Non-goals

- Do NOT modify logging system implementation files (`log/console_sink.cpp`, `log/file_sink.cpp`, `log/log.h`, `log/logger.cpp`, `log/log_filter.cpp`).
- Do NOT modify `help_command.cpp`, `version_command.cpp`, or `Image::save()`.
- Do NOT modify the `std::ofstream` YAML data file writes in `hot_reload_gltf_app.cpp` (lines 31–39).
- Do NOT change message content, conditions, or semantics — only the output mechanism changes.
- Do NOT add new log messages or restructure code.
- Do NOT touch any file outside `src/engine/` and `src/cmd/`.
- Do NOT introduce new dependencies.
- Do NOT change stdout/stderr separation for user-facing text in `main.cpp` (usage/help blocks stay as `fprintf(stderr)`).
- Do NOT perform arbitrary reformatting of files — only the targeted diagnostic statement changes and the required `#include`/`BUDDD_LOG_TAG` additions.

## Relevant ADRs

| ADR | Relevance |
|-----|-----------|
| ADR-020 (Custom Logging System) | Defines the `BUDDD_LOG_*` macro API, `BUDDD_LOG_TAG` convention, sink interface, LogLevel enum, and `std::format`-style `{}` placeholders. This contract implements the deferred migration referenced in ADR-020's "Consequences — Negative" section. |
| ADR-014 (CLI App System) | CLI flag parsing lives in `src/cmd/`; `main.cpp` is the entry point that calls `Logger::init()`. This contract preserves the pre-init bootstrap error on line 37. |
| ADR-001 (Result/Error Pattern) | Logging system is decoupled from `Error`/`Result<T>` — migrated calls use `BUDDD_LOG_*` macros directly, not through error types. |

## Files to inspect

The Code Agent MUST read the following files to understand existing patterns before editing:

- `src/engine/log/log.h` — Macro definitions, `BUDDD_LOG_TAG` usage, `std::format`-style formatting
- `src/engine/log/console_sink.h` — Output format: `[LEVEL] [Tag] message\n`
- `src/engine/log/memory_sink.h` — Test helper (relevant for test adaptation)
- `src/cmd/main.cpp` — Pre-init bootstrap, usage/help exemptions, tag declaration needed
- `src/cmd/app.cpp` — Multiple `std::cerr` FATAL, `printf`, `fprintf(stderr)` lifecycle messages
- `src/engine/asset/asset_manager.cpp` — Largest migration target (42 statements)
- `src/engine/asset/asset_manager.tpp` — Template file with one `#ifndef NDEBUG` statement
- `src/engine/asset/model_loader.cpp` — glTF loader with warnings and errors
- `src/engine/asset/file_watcher_inotify.cpp` — File watcher with start/stop debug messages
- `src/engine/render/render_device.cpp` — Factory with device creation messages
- `src/engine/render/render_device_opengl.cpp` — OpenGL backend with many diagnostic messages
- `src/engine/render/render_device_headless.cpp` — Headless backend with matching messages
- `src/engine/render/render_system.cpp` — Render system with light collection messages
- `src/engine/render/material_opengl.cpp` — OpenGL material with uniform caching messages
- `src/engine/render/material_headless.cpp` — Headless material with uniform messages
- `src/engine/render/texture_opengl.cpp` — OpenGL texture with destructor message
- `src/engine/render/shader_program_opengl.cpp` — OpenGL shader program link message
- `src/engine/render/shader_program_headless.cpp` — Headless shader program create message
- `src/engine/render/phong/phong_material.cpp` — Phong material with FATAL/error messages
- `src/engine/render/pbr/pbr_material.cpp` — PBR material with FATAL/error messages
- `src/cmd/apps/hot_reload_app.cpp` — Hot-reload demo with fprintf messages
- `src/cmd/apps/hot_reload_gltf_app.cpp` — Hot-reload glTF demo (YAML data write excluded)
- `src/cmd/apps/gltf_demo_app.cpp` — glTF demo with error messages
- `src/cmd/apps/textured_cube_app.cpp` — Textured cube with FATAL messages
- `src/cmd/apps/asset_demo_app.cpp` — Asset demo with FATAL messages
- `src/cmd/apps/phong_app.cpp` — Phong demo with FATAL/Warning messages
- `src/cmd/apps/cube_app.cpp` — Cube demo with one error message
- `src/cmd/apps/multi_material_app.cpp` — Multi-material with one error message
- `tests/cmd_tests.cpp` — Existing tests that check stderr strings (must be adapted)

## Files allowed to change

The following 31 source files MAY be modified. No other files may be touched:

1. `src/cmd/main.cpp`
2. `src/cmd/app.cpp`
3. `src/cmd/apps/hot_reload_app.cpp`
4. `src/cmd/apps/hot_reload_gltf_app.cpp`
5. `src/cmd/apps/gltf_demo_app.cpp`
6. `src/cmd/apps/textured_cube_app.cpp`
7. `src/cmd/apps/asset_demo_app.cpp`
8. `src/cmd/apps/phong_app.cpp`
9. `src/cmd/apps/cube_app.cpp`
10. `src/cmd/apps/multi_material_app.cpp`
11. `src/engine/asset/asset_manager.cpp`
12. `src/engine/asset/asset_manager.tpp`
13. `src/engine/asset/model_loader.cpp`
14. `src/engine/asset/file_watcher_inotify.cpp`
15. `src/engine/render/render_device.cpp`
16. `src/engine/render/render_device_opengl.cpp`
17. `src/engine/render/render_device_headless.cpp`
18. `src/engine/render/render_system.cpp`
19. `src/engine/render/material_opengl.cpp`
20. `src/engine/render/material_headless.cpp`
21. `src/engine/render/texture_opengl.cpp`
22. `src/engine/render/shader_program_opengl.cpp`
23. `src/engine/render/shader_program_headless.cpp`
24. `src/engine/render/phong/phong_material.cpp`
25. `src/engine/render/pbr/pbr_material.cpp`
26. `src/engine/platform/platform.cpp`
27. `src/engine/platform/platform_headless.cpp`
28. `src/engine/platform/platform_sdl3.cpp`
29. `src/engine/input/input_system.cpp`
30. `src/engine/input/input_system_sdl3.cpp`
31. `src/engine/scene/camera_component.cpp`

The following test expectation file MAY be modified to match new log output format:

32. `tests/cmd_tests.cpp`

## Files forbidden to change

| File | Reason |
|------|--------|
| `src/engine/log/console_sink.cpp` | Logging system implementation |
| `src/engine/log/file_sink.cpp` | Logging system implementation |
| `src/engine/log/log.h` | Logging system header (macro definitions) |
| `src/engine/log/logger.cpp` | Logging system implementation |
| `src/engine/log/log_filter.cpp` | Logging system implementation |
| `src/cmd/commands/help_command.cpp` | Intentional stdout output |
| `src/cmd/commands/version_command.cpp` | Intentional stdout output |
| `src/engine/image/image.cpp` (Image::save()) | Intentional data output |
| Any file outside `src/engine/` and `src/cmd/` (except `tests/cmd_tests.cpp`) | Out of scope |

## Existing conventions to follow

1. **Include style**: Engine files use `#include "log/log.h"` (relative to `src/engine/`). Cmd files can use the same path because `buddd_engine` exports `src/engine/` as a public include directory. All 31 files that get `BUDDD_LOG_*` calls must have `#include "log/log.h"` at the top (with existing includes, in alphabetical order within their group if a `#include` is being added).

2. **BUDDD_LOG_TAG declaration**: Must appear at file scope (outside any namespace), typically after the last `#include` and before any code. Convention: one blank line before and after. Format: `BUDDD_LOG_TAG("Module:Sub");`

3. **`std::format`-style formatting**: `std::cerr << "x=" << x` becomes `BUDDD_LOG_INFO("x={}", x)`. `fprintf(stderr, "x=%d y=%s\n", x, y.c_str())` becomes `BUDDD_LOG_INFO("x={} y={}", x, y)`. Trailing `\n` in original format strings MUST be removed (the sink appends its own newline).

4. **`#ifndef NDEBUG` removal**: All `#ifndef NDEBUG` guards around `std::cerr` diagnostic calls are removed entirely (not just replaced with a still-guarded `BUDDD_LOG_DEBUG`). The `BUDDD_LOG_DEBUG` call becomes unconditional — the level check replaces compile-time gating.

5. **File scope tag variable naming**: After `BUDDD_LOG_TAG("Tag")`, the macro defines `static constexpr std::string_view BUDDD_CURRENT_LOG_TAG = "Tag"`. No manual declaration needed.

6. **Multiple statements**: Each `std::cerr` / `fprintf(stderr)` / `printf()` statement is independently migrated to its own `BUDDD_LOG_*` call. Consecutive diagnostic lines remain consecutive but as individual macro calls.

7. **No `<<` after migration**: All stream concatenation `<<` is eliminated. The format string uses `{}` placeholders and arguments are passed as function arguments.

8. **FATAL pattern**: `std::cerr << "FATAL: ..."` → `BUDDD_LOG_ERROR(...)`. `std::exit()`/`std::terminate()` calls following FATAL logging are KEPT in locations where removal would introduce undefined behavior (dereferencing failed `Result`s or falling off a non-void function without returning a value). The following locations KEEP their termination calls:
   - `phong_app.cpp` `make_checkerboard_texture` (2 `std::exit()` calls) — function returns `shared_ptr<Texture>`, no non-terminating return path.
   - `phong_app.cpp` `make_solid_texture` (2 `std::exit()` calls) — same reason.
   - `phong_app.cpp` `create_phong_cube` (1 `std::exit()` call) — function returns `be::Model`, no non-terminating return path.
   - `phong_material.cpp` `Impl::create_material` (3 `std::exit()` calls) — next line dereferences `*vs`, `*fs`, or `*mat` on a failed `Result`.
   - `pbr_material.cpp` `Impl::create_material` (3 `std::exit()` calls) — same pattern.
   - `render_device_opengl.cpp` `fallback_material()` (3 `std::terminate()` calls) — function returns `Material&`, dereferences failed `Result`s; truly unrecoverable.
   - `render_device_headless.cpp` `fallback_material()` (3 `std::terminate()` calls) — same reason.
   All other `std::exit()`/`std::terminate()` calls after FATAL logging (e.g., in `app.cpp`-style `return EXIT_FAILURE` patterns) are removed or left as normal control flow per AC-016 ("where feasible").

## Required implementation behavior

### A. Common requirements for ALL 31 files

1. **Add `#include "log/log.h"`** if not already present. For files already including it (e.g., `main.cpp`, `hot_reload_gltf_app.cpp`, `asset_manager.tpp`), no additional include needed.

2. **Add `BUDDD_LOG_TAG("TagName")`** at file scope (outside any namespace) after all includes, if the file does not already have one. Currently NO file has a `BUDDD_LOG_TAG` declaration — all 31 files need one.

3. **Replace each diagnostic output statement** with the equivalent `BUDDD_LOG_*` macro using the tag and level from the spec's Tag Mapping and Level Mapping tables.

4. **Trailing `\n` removal**: All migrated messages MUST remove any trailing `\n` or `std::endl` — the sink adds its own newline.

5. **`<<` to `{}` conversion**: Every `<<` concatenation is converted to a `std::format`-compatible string with `{}` placeholders, with arguments passed as macro arguments.

6. **`#ifndef NDEBUG` removal**: The entire `#ifndef NDEBUG` / `#endif` guard around a `std::cerr` diagnostic call is removed. The new `BUDDD_LOG_DEBUG(...)` call is unconditional.

7. **`std::exit()`/`std::terminate()` removal after FATAL** (AC-016, "where feasible"): Where a `std::cerr << "FATAL: ..."` is immediately followed by `std::exit(EXIT_FAILURE)` or `std::terminate()`, the termination call is REMOVED only if doing so does not introduce undefined behavior. The following locations KEEP their termination calls (removal would cause UB):
   - `src/engine/render/render_device_opengl.cpp` (`fallback_material()`, 3 `std::terminate()` calls) — KEEP `std::terminate()`. Function returns `Material&`; after a failed `Result`, `*vs` dereferences garbage and `fallback_material_` stays null. Truly unrecoverable.
   - `src/engine/render/render_device_headless.cpp` (`fallback_material()`, 3 `std::terminate()` calls) — KEEP `std::terminate()`. Same pattern as OpenGL.
   - `src/engine/render/phong/phong_material.cpp` (`Impl::create_material`, 3 `std::exit(EXIT_FAILURE)` calls) — KEEP `std::exit()`. Next line dereferences `*vs`, `*fs`, or `*mat` on failed `Result`.
   - `src/engine/render/pbr/pbr_material.cpp` (`Impl::create_material`, 3 `std::exit(EXIT_FAILURE)` calls) — KEEP `std::exit()`. Same pattern.
   - `src/cmd/apps/phong_app.cpp` (`make_checkerboard_texture`, 2 `std::exit()` calls) — KEEP `std::exit()`. Function returns `shared_ptr<Texture>`, no valid non-terminating return path.
   - `src/cmd/apps/phong_app.cpp` (`make_solid_texture`, 2 `std::exit()` calls) — KEEP `std::exit()`. Same reason.
   - `src/cmd/apps/phong_app.cpp` (`create_phong_cube`, 1 `std::exit()` call) — KEEP `std::exit()`. Function returns `be::Model`, no valid non-terminating return path.
   The following locations do NOT call `std::exit()`/`std::terminate()` after FATAL and need no change:
   - `src/cmd/app.cpp` (`run_app`, 4 FATAL blocks) — already `return EXIT_FAILURE`, not an `exit()` call.
   - `src/cmd/apps/hot_reload_app.cpp` (`setup`, 3 FATAL blocks) — already `return std::unexpected(...)`.
   - `src/cmd/apps/textured_cube_app.cpp` (`setup`, multiple FATAL blocks) — already `return std::unexpected(...)`.
   - `src/cmd/apps/asset_demo_app.cpp` (`setup`, 2 FATAL blocks) — already `return std::unexpected(...)`.

### B. Per-file changes

#### `src/cmd/main.cpp` — Tag: `App`
- **Line 37**: `fprintf(stderr, "Error: %s\n", ...)` — KEEP AS-IS (pre-Logger::init).
- **Line 50**: `fprintf(stderr, "Error: %s\n", ...)` → `BUDDD_LOG_ERROR("Error: {}", ...)` (post-init, tag `App`).
- **Lines 64-65**: `fprintf(stderr, "Unknown command: ...")` → `BUDDD_LOG_ERROR("Unknown command: '{}'", argv[1])`. Line 65 (usage text fwrite to stderr) — KEEP AS-IS.
- **Lines 103-104**: `fprintf(stderr, "Unknown scene: ...")` → `BUDDD_LOG_ERROR("Unknown scene: '{}'", argv[2])`. Lines 104-125 (usage text fprintf block) — KEEP AS-IS.
- **Line 133**: `fprintf(stderr, "Error: %s\n", ...)` → `BUDDD_LOG_ERROR("Error: {}", ...)`.
- **Lines 152-155**: `fprintf(stderr, "Warning: unexpected arguments...")` → `BUDDD_LOG_WARN("Warning: unexpected arguments after '{}': {} ...", argv[flags_start - 1], ...)`. The loop's `fprintf(stderr, " %s", argv[i])` and trailing `fprintf(stderr, "\n")` collapse into a single `BUDDD_LOG_WARN` call with concatenated message.
- Add `#include "log/log.h"` (already present).
- Add `BUDDD_LOG_TAG("App");` after includes.

#### `src/cmd/app.cpp` — Tag: `App`
- Line 37: `std::cerr << "FATAL: " << ...` → `BUDDD_LOG_ERROR("FATAL: {}", to_string(platform.error()))`.
- Lines 48-49: Same pattern → `BUDDD_LOG_ERROR("FATAL: {}", to_string(window.error()))`.
- Line 52: `std::printf("Window opened: %dx%d\n", ...)` → `BUDDD_LOG_INFO("Window opened: {}x{}", (*window)->width(), (*window)->height())`.
- Lines 57-58: `std::cerr << "FATAL: "` → `BUDDD_LOG_ERROR("FATAL: {}", to_string(device.error()))`.
- Lines 63-64: `std::cerr << to_string(...)` → `BUDDD_LOG_ERROR("{}", to_string(setup_result.error()))`.
- Lines 72-77: Two `std::fprintf(stderr, "Scene started: ...\n")` → `BUDDD_LOG_INFO("Scene started: {} ({} frames)", ...)` and `BUDDD_LOG_INFO("Scene started: {} (interactive)", ...)`.
- Line 93: `std::fprintf(stderr, "Scene aborted by user\n")` → `BUDDD_LOG_INFO("Scene aborted by user")`.
- Line 100: `std::fprintf(stderr, "Scene aborted by user (frame %d)\n", ...)` → `BUDDD_LOG_INFO("Scene aborted by user (frame {})", frame + 1)`.
- Lines 131: `std::printf("Captured: %s\n", ...)` → `BUDDD_LOG_INFO("Captured: {}", spec.path)`.
- Lines 134, 138, 142: `std::cerr << to_string(...)` (capture failure) → `BUDDD_LOG_ERROR("{}", ...)`.
- Lines 155-156: `std::fprintf(stderr, "Scene complete: ...\n")` → `BUDDD_LOG_INFO("Scene complete: {} ({} frames rendered)", ...)`.
- Line 162: `std::printf("Window closed, shutting down.\n")` → `BUDDD_LOG_INFO("Window closed, shutting down.")`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("App");`.

#### `src/cmd/apps/hot_reload_app.cpp` — Tag: `HotReload`
- Line 43: `std::fprintf(stderr, "[HotReload] Initial texture...\n")` → `BUDDD_LOG_INFO("Initial texture: hot_reload_a.png -> hot_reload_live.png")`. Remove `[HotReload]` prefix from message (the tag provides it).
- Lines 48-50: `std::fprintf(stderr, "FATAL: could not create AssetManager: %s\n", ...)` → `BUDDD_LOG_ERROR("FATAL: could not create AssetManager: {}", ...)`.
- Lines 56-58: `std::fprintf(stderr, "FATAL: could not load material: %s\n", ...)` → `BUDDD_LOG_ERROR("FATAL: could not load material: {}", ...)`.
- Line 139: `std::fprintf(stderr, "FATAL: could not create cube model\n")` → `BUDDD_LOG_ERROR("FATAL: could not create cube model")`.
- Line 150: `std::fprintf(stderr, "[HotReload] Setup complete...\n")` → `BUDDD_LOG_INFO("Setup complete. Will swap texture at frame 30.")`.
- Line 162: `std::fprintf(stderr, "[HotReload] Frame 30: swapping texture...\n")` → `BUDDD_LOG_INFO("Frame 30: swapping texture...")`.
- Line 171: `std::fprintf(stderr, "[HotReload] Texture swapped...\n")` → `BUDDD_LOG_INFO("Texture swapped and poll_file_events() called.")`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("HotReload");`.

#### `src/cmd/apps/hot_reload_gltf_app.cpp` — Tag: `HotReload`
- Line 39: `std::cerr << "[HotReload] " << desc << ...` → `BUDDD_LOG_INFO("{} (scale={})", desc, scale)`. Note: this is the `write_yaml` function in anonymous namespace — must include `log/log.h` in this file.
- Line 79: `std::cerr << "[HotReload] Started..."` → `BUDDD_LOG_INFO("Started — will scale to 2.0 at frame 30")`.
- Lines 96: `std::cerr << "[HotReload] Load failed: "` → `BUDDD_LOG_ERROR("Load failed: {}", ...)`.
- Line 101: `std::cerr << "[HotReload] Reloaded: "` → `BUDDD_LOG_INFO("Reloaded: {} entities", model_entities_.size())`.
- Line 126: `std::cerr << "[HotReload] \u2192 Box is now..."` → `BUDDD_LOG_INFO("→ Box is now 2x bigger (scale=2.0)")`.
- The `std::ofstream` YAML data write in `write_yaml()` (lines 32-38) is KEPT AS-IS — not diagnostic output.
- Add `#include "log/log.h"` (check if already present; if so, no duplicate).
- Add `BUDDD_LOG_TAG("HotReload");`.

#### `src/cmd/apps/gltf_demo_app.cpp` — Tag: `GltfDemo`
- Lines 37-38: `std::cerr << "Failed to create AssetManager: "` → `BUDDD_LOG_ERROR("Failed to create AssetManager: {}", ...)`.
- Lines 73-74: `std::cerr << "Failed to load model: "` → `BUDDD_LOG_ERROR("Failed to load model: {}", ...)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("GltfDemo");`.

#### `src/cmd/apps/textured_cube_app.cpp` — Tag: `TexturedCube`
- Lines 34-35: `std::cerr << "FATAL: could not load..."` → `BUDDD_LOG_ERROR("FATAL: could not load assets/brick.png: {}", ...)`.
- Lines 42-44: `std::cerr << "FATAL: could not create texture: "` → `BUDDD_LOG_ERROR("FATAL: could not create texture: {}", ...)`.
- Lines 147-148: `std::cerr << "FATAL: " << to_string(...)` → `BUDDD_LOG_ERROR("FATAL: {}", ...)`.
- Lines 153-154: Same pattern → `BUDDD_LOG_ERROR("FATAL: {}", ...)`.
- Lines 160-161: `std::cerr << "FATAL: " << to_string(...)` → `BUDDD_LOG_ERROR("FATAL: {}", ...)`.
- Lines 168-170: `std::cerr << "FATAL: set_texture failed: "` → `BUDDD_LOG_ERROR("FATAL: set_texture failed: {}", ...)`.
- Lines 183-185: `std::cerr << "FATAL: Failed to create textured cube model: "` → `BUDDD_LOG_ERROR("FATAL: Failed to create textured cube model: {}", ...)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("TexturedCube");`.

#### `src/cmd/apps/asset_demo_app.cpp` — Tag: `AssetDemo`
- Lines 40-41: `std::cerr << "FATAL: could not create AssetManager: "` → `BUDDD_LOG_ERROR("FATAL: could not create AssetManager: {}", ...)`.
- Lines 49-50: `std::cerr << "FATAL: could not load material: "` → `BUDDD_LOG_ERROR("FATAL: could not load material: {}", ...)`.
- Lines 139-141: `std::cerr << "FATAL: Failed to create textured cube model: "` → `BUDDD_LOG_ERROR("FATAL: Failed to create textured cube model: {}", ...)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("AssetDemo");`.

#### `src/cmd/apps/phong_app.cpp` — Tag: `Phong`
- Lines 66-68 (make_checkerboard_texture): `std::cerr << "FATAL: Failed to create checkerboard image..."` → `BUDDD_LOG_ERROR("FATAL: Failed to create checkerboard image: {}", ...)`. KEEP `std::exit(EXIT_FAILURE)` after it — function returns `std::shared_ptr<be::Texture>`, no valid non-terminating return path. Removing exit would dereference a failed `Result` on the next line (UB).
- Lines 73-75: Same for texture creation → `BUDDD_LOG_ERROR(...)`. KEEP `std::exit()` (same reason).
- Lines 99-101 (make_solid_texture): Same pattern → `BUDDD_LOG_ERROR(...)`. KEEP `std::exit()` (same reason).
- Lines 106-108: Same → `BUDDD_LOG_ERROR(...)`. KEEP `std::exit()` (same reason).
- Lines 167-169 (create_phong_cube): `std::cerr << "FATAL: Failed to create phong cube model..."` → `BUDDD_LOG_ERROR("FATAL: Failed to create phong cube model: {}", ...)`. KEEP `std::exit(EXIT_FAILURE)` — function returns `be::Model`, no valid non-terminating return path (same as make_checkerboard_texture, make_solid_texture).
- Lines 229-230: `std::cerr << "Warning: set_uniform(...) failed: "` → `BUDDD_LOG_WARN("set_uniform(u_material_specular) failed: {}", to_string(...))`.
- Lines 235-236: Same → `BUDDD_LOG_WARN("set_uniform(u_material_shininess) failed: {}", ...)`.
- Lines 242-243: Same → `BUDDD_LOG_WARN("set_uniform(u_material_diffuse_tint) failed: {}", ...)`.
- Lines 249-250: `std::cerr << "Warning: set_texture failed: "` → `BUDDD_LOG_WARN("set_texture failed: {}", ...)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Phong");`.

#### `src/cmd/apps/cube_app.cpp` — Tag: `Cube`
- Lines 84-85: `std::cerr << "Failed to set u_mvp uniform: "` → `BUDDD_LOG_ERROR("Failed to set u_mvp uniform: {}", ...)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Cube");`.

#### `src/cmd/apps/multi_material_app.cpp` — Tag: `MultiMaterial`
- Lines 161-162: `std::cerr << "u_mvp set failed: "` → `BUDDD_LOG_ERROR("u_mvp set failed: {}", ...)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("MultiMaterial");`.

#### `src/engine/asset/asset_manager.cpp` — Tag: `Asset`
All `std::cerr` calls in this file are migrated. This file has 42 statements, many under `#ifndef NDEBUG`. Key patterns:
- Lines 51-52 (construction, non-NDEBUG): `std::cerr << "[FileWatcher] Failed to create: ..."` → `BUDDD_LOG_WARN("FileWatcher failed to create: {} — falling back to NullFileWatcher", ...)`. Note: `\u2014` in the original is preserved as-is (Unicode passes through).
- Lines 83-85 (clear, under NDEBUG): `#ifndef NDEBUG` guard removed, `std::cerr << "[Asset] Cache cleared..."` → `BUDDD_LOG_DEBUG("Cache cleared ({} assets)", count)`.
- Lines 242-245 (load_texture YAML error, under NDEBUG): → `BUDDD_LOG_DEBUG("YAML error: {} - {}", ...)`.
- Lines 255-257 (type mismatch, under NDEBUG): → `BUDDD_LOG_DEBUG("Type mismatch: {} (expected Texture, got {})", ...)`.
- Lines 320-324 (Texture created, under NDEBUG): → `BUDDD_LOG_DEBUG("Texture created: {} ({}x{}, {}ch)", ...)`.
- Lines 348-349 (Material type mismatch, under NDEBUG): → `BUDDD_LOG_DEBUG("Type mismatch: {} (expected Material, got {})", ...)`.
- Lines 398-400 (Shader cache hit, under NDEBUG): → `BUDDD_LOG_DEBUG("Shader program cache hit: ({}, {})", ...)`.
- Lines 415-417 (Shader compiled, under NDEBUG): → `BUDDD_LOG_DEBUG("Shader program compiled: ({}, {})", ...)`.
- Lines 440-443 (set_texture warning, under NDEBUG): → `BUDDD_LOG_DEBUG("Warning: could not set texture '{}' on material {}", ...)`.
- Lines 463-466 (Constant not found, under NDEBUG): → `BUDDD_LOG_DEBUG("Constant '{}' not found in material {}", ...)`.
- Lines 469-472 (warning: constant not float, under NDEBUG): → `BUDDD_LOG_DEBUG("Warning: constant '{}' is not a valid float, skipping", ...)`.
- Lines 491-493 (Material created, under NDEBUG): → `BUDDD_LOG_DEBUG("Material created: {} ({}, {})", ...)`.
- Lines 517-518 (Model type mismatch, under NDEBUG): → `BUDDD_LOG_DEBUG("Type mismatch: {} (expected Model, got {})", ...)`.
- Line 548 (scale is 0.0, non-NDEBUG): `std::cerr << "[Asset] Warn: Model '..."` → `BUDDD_LOG_WARN("Model '{}' scale is 0.0", id)`.
- Lines 554-556 (model load failed, under NDEBUG): → `BUDDD_LOG_DEBUG("Model load failed: {} — {}", ...)`.
- Lines 582-586 (Model loaded, under NDEBUG): → `BUDDD_LOG_DEBUG("Model loaded: {} ({} verts, {} root nodes)", ...)`.
- Lines 608, 617, 625, 632, 646, 652, 682, 695, 732, 739, 747, 761, 771, 782, 788, 794, 802, 807, 811, 817, 827, 834 (hot-reload handlers, all non-NDEBUG): migrate each `std::cerr` to appropriate level:
  - YAML parse/source/creation failures → `BUDDD_LOG_ERROR` (operational failures that prevent an operation)
  - Informational hot-reload status → `BUDDD_LOG_INFO` (e.g., "Hot-reloaded: material X", "Hot-reload: model reloaded Y")
  - Shader compile/link failures → `BUDDD_LOG_ERROR`
- Remove `#include <iostream>` if `std::cerr` was the only usage (keep if `std::cout` or other iostream usage still present). In this file, iostream is used only for `std::cerr`, so it can be removed after migration.
- NOTE: Do NOT add `#include "log/log.h"` or `BUDDD_LOG_TAG("Asset");` here — they are added in `asset_manager.tpp` instead (which is `#include`d by `asset_manager.h`, making the tag and macros available in the same translation unit). Adding them in both files would cause a redefinition error (ODR violation).

#### `src/engine/asset/asset_manager.tpp` — Tag: `Asset`
- Lines 30-32 (under NDEBUG): Remove `#ifndef NDEBUG` guard, `std::cerr << "[Asset] Cache hit: " << id` → `BUDDD_LOG_DEBUG("Cache hit: {}", id)`.
- Add `#include "log/log.h"` (this is a `.tpp` file, template implementation; include after `#pragma once` and existing includes).
- Add `BUDDD_LOG_TAG("Asset");`.

#### `src/engine/asset/model_loader.cpp` — Tag: `Asset:ModelLoader`
- Line 420: `std::cerr << "[Asset] Warn: data URI in glTF..."` → `BUDDD_LOG_WARN("data URI in glTF texture not supported")`.
- Lines 436-437: `std::cerr << "[Asset] Warn: texture load failed..."` → `BUDDD_LOG_WARN("texture load failed for {} — using magenta fallback", image.uri)`.
- Lines 497, 506, 515, 524, 533 (texCoord > 0 warnings): `std::cerr << "[Asset] Warn: texture texCoord > 0 not supported in V1\n"` → each becomes `BUDDD_LOG_WARN("texture texCoord > 0 not supported in V1")`.
- Line 542: `std::cerr << "[Asset] Warn: material ext not supported: ..."` → `BUDDD_LOG_WARN("material ext not supported: {} (KHR_materials_pbrSpecularGlossiness) — using default PBR factors", gltf_mat.name)`.
- Line 554: `std::cerr << "[Asset] Warn: alphaMode..."` → `BUDDD_LOG_WARN("alphaMode '{}' not supported in V1 — treating as opaque", gltf_mat.alphaMode)`.
- Lines 562-568 (PBR material created, under NDEBUG): → `BUDDD_LOG_DEBUG("PbrMaterial created: {}", ...)`.
- Lines 611-612: `std::cerr << "[Asset] Warn: unsupported primitive mode..."` → `BUDDD_LOG_WARN("unsupported primitive mode {} in mesh {} — skipping", prim.mode, mesh.name)`.
- Line 905: `std::cerr << "[Asset] glTF warning: " << warn` → `BUDDD_LOG_WARN("glTF warning: {}", warn)`.
- Line 914: `std::cerr << "[Asset] glTF error: " << err` → `BUDDD_LOG_ERROR("glTF error: {}", err)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Asset:ModelLoader");`.

#### `src/engine/asset/file_watcher_inotify.cpp` — Tag: `Asset:FileWatcher`
- Lines 26-27: `std::cerr << "[FileWatcher] inotify_init1 failed: "` → `BUDDD_LOG_ERROR("inotify_init1 failed: {}", strerror(errno))`.
- Line 39: `std::cerr << "[FileWatcher] pipe2 failed: "` → `BUDDD_LOG_ERROR("pipe2 failed: {}", strerror(errno))`.
- Line 83: `std::cerr << "[FileWatcher] Cannot start: inotify fd invalid\n"` → `BUDDD_LOG_ERROR("Cannot start: inotify fd invalid")`.
- Lines 93-94 (under NDEBUG): `std::cerr << "[FileWatcher] Monitoring: "` → `BUDDD_LOG_DEBUG("Monitoring: {}", watch_path_)`.
- Lines 112-113 (under NDEBUG): `std::cerr << "[FileWatcher] Stopped\n"` → `BUDDD_LOG_DEBUG("Stopped")`.
- Line 143: `std::cerr << "[FileWatcher] poll failed: "` → `BUDDD_LOG_ERROR("poll failed: {}", strerror(errno))`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Asset:FileWatcher");`.

#### `src/engine/render/render_device.cpp` — Tag: `Render`
- Line 15: `std::cerr << "Render device created (Headless)\n"` → `BUDDD_LOG_INFO("Render device created (Headless)")`.
- Lines 33-34 (under NDEBUG): `std::cerr << "Depth buffer requested: 24-bit\n"` → `BUDDD_LOG_DEBUG("Depth buffer requested: 24-bit")`.
- Line 44: `std::cerr << "Render device created (OpenGL 4.5 Core)\n"` → `BUDDD_LOG_INFO("Render device created (OpenGL 4.5 Core)")`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Render");`.

#### `src/engine/render/render_device_opengl.cpp` — Tag: `Render:OpenGL`
- Lines 96-105 (constructor, under NDEBUG): → `BUDDD_LOG_DEBUG("Depth testing enabled (GL_LESS)")` and `BUDDD_LOG_DEBUG("Warning: OpenGL error during depth state setup: {}", ...)`.
- Lines 161: `std::cerr << "Shader compilation failed: " << log` → `BUDDD_LOG_ERROR("Shader compilation failed: {}", log)`.
- Lines 165-167: `std::cerr << "Shader created (type=..."` → `BUDDD_LOG_INFO("Shader created (type={})", ...)`. **Note**: This level is debatable — it's informational lifecycle. Keep as INFO.
- Lines 209: `std::cerr << "Material linking failed: "` → `BUDDD_LOG_ERROR("Material linking failed: {}", log)`.
- Line 224: `std::cerr << "Material created\n"` → `BUDDD_LOG_INFO("Material created")`.
- Line 234: `std::cerr << "Material created (from ShaderProgram)\n"` → `BUDDD_LOG_INFO("Material created (from ShaderProgram)")`.
- Lines 288-289: `std::cerr << "Vertex buffer created..."` → `BUDDD_LOG_INFO("Vertex buffer created ({} vertices, {} attributes)", vertex_count, format.attributes.size())`.
- Lines 311-312: `std::cerr << "Index buffer created..."` → `BUDDD_LOG_INFO("Index buffer created ({} bytes, {})", data.size(), ...)`.
- Lines 372-373: `std::cerr << "Texture created (OpenGL..."` → `BUDDD_LOG_INFO("Texture created (OpenGL, {}x{}, {} channels)", image.width(), image.height(), ch)`.
- Lines 397-399 (draw, under NDEBUG): → `BUDDD_LOG_DEBUG("Draw: {} {} vertices", ...)`.
- Lines 432-434 (draw_indexed, under NDEBUG): → `BUDDD_LOG_DEBUG("Draw indexed: {} {} indices", ...)`.
- Lines 460-462, 466-468, 472-474 (fallback_material, FATAL): `std::cerr << "FATAL: fallback..."` → `BUDDD_LOG_ERROR("FATAL: fallback vertex shader creation failed: {}", ...)`. KEEP the following `std::terminate()` call — removal would cause UB (function returns `Material&`, dereferences `*vs`/`*mat` on failed `Result`s, and `fallback_material_` stays null). Truly unrecoverable per AC-016 exception.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Render:OpenGL");`.

#### `src/engine/render/render_device_headless.cpp` — Tag: `Render:Headless`
- Line 156: `std::cerr << "Shader compilation failed (simulated)\n"` → `BUDDD_LOG_ERROR("Shader compilation failed (simulated)")`.
- Lines 163-165: `std::cerr << "Shader created (Headless, type=..."` → `BUDDD_LOG_INFO("Shader created (Headless, type={})", ...)`.
- Lines 209-210: `std::cerr << "Material linking failed (simulated..."` → `BUDDD_LOG_ERROR("Material linking failed (simulated: no matching vertex output / fragment input variables)")`.
- Line 229: `std::cerr << "Material created (Headless)\n"` → `BUDDD_LOG_INFO("Material created (Headless)")`.
- Line 250: `std::cerr << "Material created (Headless, from ShaderProgram)\n"` → `BUDDD_LOG_INFO("Material created (Headless, from ShaderProgram)")`.
- Lines 276-277: `std::cerr << "Vertex buffer created (Headless..."` → `BUDDD_LOG_INFO("Vertex buffer created (Headless, {} vertices)", vertex_count)`.
- Lines 295-296: `std::cerr << "Index buffer created (Headless..."` → `BUDDD_LOG_INFO("Index buffer created (Headless, {} bytes)", data.size())`.
- Lines 329-330: `std::cerr << "Texture created (Headless..."` → `BUDDD_LOG_INFO("Texture created (Headless, {}x{}, {} channels)", image.width(), image.height(), ch)`.
- Lines 353-355 (draw, under NDEBUG): → `BUDDD_LOG_DEBUG("Draw (Headless)")`. Note: original has `/*vertex_count*/ "?"` — the migrated version just uses `BUDDD_LOG_DEBUG("Draw (Headless)")` since the count is commented out.
- Lines 371-373 (draw_indexed, under NDEBUG): → `BUDDD_LOG_DEBUG("Draw indexed (Headless)")`.
- Lines 395-397, 399-401, 405-407 (fallback_material, FATAL): `std::cerr << "FATAL: fallback..."` → `BUDDD_LOG_ERROR(...)`. KEEP `std::terminate()` after each — removal would cause UB (same pattern as OpenGL fallback_material). Truly unrecoverable per AC-016 exception.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Render:Headless");`.

#### `src/engine/render/render_system.cpp` — Tag: `Render`
- Lines 31-32: `std::cerr << "RenderSystem: no active camera..."` → `BUDDD_LOG_WARN("RenderSystem: no active camera — rendering skipped")`. **Level note**: This is a warning (operation continues without rendering but it's a significant state issue).
- Lines 104-106 (under NDEBUG): `BUDDD_LOG_DEBUG("RenderSystem: collected {} lights", light_count)`.
- Lines 120-123: `std::cerr << "RenderSystem: set_uniform(u_mvp) failed..."` → `BUDDD_LOG_ERROR("RenderSystem: set_uniform(u_mvp) failed for entity {}: {}", entity.id().index, to_string(r.error()))`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Render");`.

#### `src/engine/render/material_opengl.cpp` — Tag: `Render:OpenGL`
- Lines 69-71 (set_uniform float, under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform cached: {} (type=float)", name)`.
- Lines 79-81 (int, under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform cached: {} (type=int)", name)`.
- Lines 89-91 (bool, under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform cached: {} (type=bool)", name)`.
- Lines 99-101 (Vec3, under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform cached: {} (type=Vec3)", name)`.
- Lines 109-111 (Vec4, under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform cached: {} (type=Vec4)", name)`.
- Lines 119-121 (Mat4, under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform cached: {} (type=Mat4)", name)`.
- Lines 150-152 (set_texture, under NDEBUG): → `BUDDD_LOG_DEBUG("Texture set: {}", name)`.
- Lines 173-174 (bind, under NDEBUG): → `BUDDD_LOG_DEBUG("Material bind: program {}", prog)`.
- Lines 201-203 (bind texture, under NDEBUG): → `BUDDD_LOG_DEBUG("Bind texture: {} (unit={})", name, unit)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Render:OpenGL");`.

#### `src/engine/render/material_headless.cpp` — Tag: `Render:Headless`
- Lines 22: `std::cerr << "Uniform not found: "` (non-NDEBUG but not under guard — it's a normal error path) → `BUDDD_LOG_ERROR("Uniform not found: {}", name)`. Note: this is in the `set_uniform` function's not-found path, which is an operational error, not just debug info.
- Lines 28-30 (under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform set: {} (type=float)", name)`.
- Lines 44-46 (under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform set: {} (type=int)", name)`.
- Lines 60-62 (under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform set: {} (type=bool)", name)`.
- Lines 76-78 (under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform set: {} (type=Vec3)", name)`.
- Lines 92-94 (under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform set: {} (type=Vec4)", name)`.
- Lines 108-110 (under NDEBUG): → `BUDDD_LOG_DEBUG("Uniform set: {} (type=Mat4)", name)`.
- Lines 123-124 (set_texture not found): `std::cerr << "Uniform not found: "` → `BUDDD_LOG_ERROR("Uniform not found: {}", name)`.
- Lines 130-132 (set_texture success, under NDEBUG): → `BUDDD_LOG_DEBUG("Texture set (Headless): {}", name)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Render:Headless");`.

#### `src/engine/render/texture_opengl.cpp` — Tag: `Render:OpenGL`
- Lines 15-17 (destructor, under NDEBUG): `std::cerr << "TextureOpenGL destroyed: " << texture_` → `BUDDD_LOG_DEBUG("TextureOpenGL destroyed: {}", texture_)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Render:OpenGL");`.

#### `src/engine/render/shader_program_opengl.cpp` — Tag: `Render:OpenGL`
- Lines 54-56 (create, under NDEBUG): `std::cerr << "Shader program linked (OpenGL, id=" << program` → `BUDDD_LOG_DEBUG("Shader program linked (OpenGL, id={})", program)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Render:OpenGL");`.

#### `src/engine/render/shader_program_headless.cpp` — Tag: `Render:Headless`
- Lines 111-113 (create, under NDEBUG): `std::cerr << "Shader program created (Headless, gen=" << gen` → `BUDDD_LOG_DEBUG("Shader program created (Headless, gen={})", gen)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Render:Headless");`.

#### `src/engine/render/phong/phong_material.cpp` — Tag: `Render:Phong`
- Lines 63-65 (FATAL, create_material): `std::cerr << "FATAL: Failed to create Phong vertex shader: "` → `BUDDD_LOG_ERROR("FATAL: Failed to create Phong vertex shader: {}", to_string(vs.error()))`. KEEP the following `std::exit(EXIT_FAILURE);` — removing it would cause UB (next line dereferences `*vs` on a failed `Result`). Truly unrecoverable; no Material can ever be created without a valid shader.
- Lines 70-73 (FATAL, fragment shader): Same pattern. KEEP `std::exit(EXIT_FAILURE)` (same reason).
- Lines 86-89 (FATAL, material creation): Same pattern. KEEP `std::exit(EXIT_FAILURE)` (same reason).
- Lines 165-166 (set_camera_position error): `std::cerr << "PhongMaterial: set_uniform(u_camera_pos) failed: "` → `BUDDD_LOG_WARN("PhongMaterial: set_uniform(u_camera_pos) failed: {}", to_string(r.error()))`. Note: this is a recoverable warning, not an error.
- Lines 172-174 (set_lights error): `std::cerr << "PhongMaterial: set_uniform(u_light_count) failed\n"` → `BUDDD_LOG_WARN("PhongMaterial: set_uniform(u_light_count) failed")`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Render:Phong");`.

#### `src/engine/render/pbr/pbr_material.cpp` — Tag: `Render:Pbr`
- Lines 63-65 (FATAL): `std::cerr << "FATAL: Failed to create PBR vertex shader: "` → `BUDDD_LOG_ERROR("FATAL: Failed to create PBR vertex shader: {}", to_string(vs.error()))`. KEEP `std::exit(EXIT_FAILURE)` — removing it would cause UB (next line dereferences `*vs` on a failed `Result`).
- Lines 70-73 (FATAL, fragment): Same pattern. KEEP `std::exit(EXIT_FAILURE)` (same reason).
- Lines 80-82 (FATAL, material): Same pattern. KEEP `std::exit(EXIT_FAILURE)` (same reason).
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Render:Pbr");`.

#### `src/engine/platform/platform.cpp` — Tag: `Platform`
- Line 14-17 (SDL init failure): `std::cerr << "Platform init failed: SDL_Init failed: " << SDL_GetError() << "\n"` → `BUDDD_LOG_ERROR("Platform init failed: SDL_Init failed: {}", SDL_GetError())`. Keep the `return make_error(...)` on the next line.
- Line 19: `std::cerr << "Platform backend: SDL3\n"` → `BUDDD_LOG_INFO("Platform backend: SDL3")`.
- Line 20: `std::cerr << "Platform initialized\n"` → `BUDDD_LOG_INFO("Platform initialized")`.
- Line 24: `std::cerr << "Platform backend: Headless\n"` → `BUDDD_LOG_INFO("Platform backend: Headless")`.
- Line 25: `std::cerr << "Platform initialized\n"` → `BUDDD_LOG_INFO("Platform initialized")`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Platform");`.

#### `src/engine/platform/platform_headless.cpp` — Tag: `Platform:Headless`
- Line 26: `std::cerr << "Window created (Headless): " << config.width << "x" << config.height << "\n"` → `BUDDD_LOG_INFO("Window created (Headless): {}x{}", config.width, config.height)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Platform:Headless");`.

#### `src/engine/platform/platform_sdl3.cpp` — Tag: `Platform:SDL3`
- Line 11: `std::cerr << "Platform shutdown (SDL3)\n"` → `BUDDD_LOG_INFO("Platform shutdown (SDL3)")`.
- Line 64: `std::cerr << "Window created: " << config.width << "x" << config.height << "\n"` → `BUDDD_LOG_INFO("Window created: {}x{}", config.width, config.height)`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Platform:SDL3");`.

#### `src/engine/input/input_system.cpp` — Tag: `Input`
- Line 14: `std::cerr << "InputSystem backend: SDL3\n"` → `BUDDD_LOG_INFO("InputSystem backend: SDL3")`.
- Line 18: `std::cerr << "InputSystem backend: Headless\n"` → `BUDDD_LOG_INFO("InputSystem backend: Headless")`.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Input");`.

#### `src/engine/input/input_system_sdl3.cpp` — Tag: `Input:SDL3`
- Line 105 (under `#ifndef NDEBUG`): `std::cerr << "InputSystemSDL3: unrecognised scancode " << static_cast<int>(scancode) << "\n"` → `BUDDD_LOG_DEBUG("InputSystemSDL3: unrecognised scancode {}", static_cast<int>(scancode))`. Remove `#ifndef NDEBUG` guard.
- Line 118 (under `#ifndef NDEBUG`): Same pattern → `BUDDD_LOG_DEBUG("InputSystemSDL3: unrecognised scancode {}", static_cast<int>(scancode))`. Remove `#ifndef NDEBUG` guard.
- Add `#include "log/log.h"`.
- Add `BUDDD_LOG_TAG("Input:SDL3");`.

#### `src/engine/scene/camera_component.cpp` — Tag: `Scene:ECSCamera`
- Line 23 (under `#ifndef NDEBUG`): `std::cerr << "CameraComponent: registered entity " << entity().id().index << " as active camera\n"` → `BUDDD_LOG_DEBUG("CameraComponent: registered entity {} as active camera", entity().id().index)`. Remove `#ifndef NDEBUG` guard.
- Line 33 (under `#ifndef NDEBUG`): `std::cerr << "CameraComponent: unregistered entity " << entity_id_.index << "\n"` → `BUDDD_LOG_DEBUG("CameraComponent: unregistered entity {}", entity_id_.index)`. Remove `#ifndef NDEBUG` guard.
- Add `#include "log/log.h"`. Remove `#include <iostream>` (no longer needed).
- Add `BUDDD_LOG_TAG("Scene:ECSCamera");`.

### C. Test adaptation — `tests/cmd_tests.cpp`

The existing cmd_tests verify exact stderr strings. After migration, the error lines will appear as `[ERROR] [App] ...` formatted by ConsoleSink instead of bare text. The usage/help text blocks (still `fprintf(stderr)`) remain unchanged.

Specific test adaptations needed:

1. **`"buddd unknowncommand exits with code 1"`** (line 39-45):
   - Change `res.stderr_str.find("Unknown command: 'unknowncommand'")` → `res.stderr_str.find("[ERROR] [App] Unknown command: 'unknowncommand'")`
   - The `"Usage: buddd <command> [<args>]"` check stays as-is (usage text unchanged).

2. **`"buddd demo is unknown command"`** (lines 64-70):
   - `res.stderr_str.find("Unknown command: 'demo'")` → `res.stderr_str.find("[ERROR] [App] Unknown command: 'demo'")`.

3. **`"buddd capture prints unknown command"`** (lines 72-79):
   - `res.stderr_str.find("Unknown command: 'capture'")` → `res.stderr_str.find("[ERROR] [App] Unknown command: 'capture'")`.
   - Usage text check stays as-is.

4. **`"buddd run unknownscene prints error"`** (lines 81-87):
   - `res.stderr_str.find("Unknown scene: 'unknownscene'")` → `res.stderr_str.find("[ERROR] [App] Unknown scene: 'unknownscene'")`.
   - Usage text check stays as-is.

5. **`"buddd test is unknown command"`** (lines 89-95):
   - `res.stderr_str.find("Unknown command: 'test'")` → `res.stderr_str.find("[ERROR] [App] Unknown command: 'test'")`.
   - Usage text check stays as-is.

6. **`"buddd with no arguments defaults to run command"`** (lines 97-126):
   - The assertion `stdout_str.find("Window opened: 1024x768")` on line 125 MUST be changed. After migration, `std::printf("Window opened: %dx%d\n", ...)` becomes `BUDDD_LOG_INFO(...)` which writes to stderr (via ConsoleSink), not stdout.
   - **Fix**: Change the assertion to check stderr instead of stdout: `res.stderr_str.find("[INFO] [App] Window opened: 1024x768")`. All other assertions in this test case (exit code, stdout checks for `"Starting interactive loop..."`) remain unchanged.

**IMPORTANT for test adaptation**: Since the `run_buddd` helper captures `stderr_str` from the child process's stderr, and ConsoleSink writes to stderr, the log output will appear in the same stream. The test assertions just need to match the new formatted format. No change to the test infrastructure needed.

### D. Build verification

- No CMakeLists.txt changes needed. The logging system headers are already accessible via `#include "log/log.h"` from `src/cmd/` (through `buddd_engine`'s public include path) and from `src/engine/` (direct).
- Full build: `cmake --build build` must succeed with zero errors and zero new warnings.

## Required tests

### Unit tests

None new required. The existing `logging_tests.cpp` already tests the logging system itself. The migration introduces no new testing infrastructure.

### Existing test adaptations

As specified in section C above, `tests/cmd_tests.cpp` assertions must be updated to expect `[ERROR] [App]` prefixed error lines from ConsoleSink. The test adaptation is part of the implementation. All existing tests must continue to pass.

### E2E / Integration verification

1. **Build verification**: `cmake --build build` succeeds with zero errors and zero new warnings.
2. **Grep verification**: After migration, run:
   ```
   grep -rn 'std::cerr\|fprintf(stderr\|printf(\|write(STDERR_FILENO' src/engine/ src/cmd/
   ```
   The output should contain ONLY:
   - The exempted `fprintf(stderr)` lines in `main.cpp` (pre-init error, usage/help texts)
   - False positives from non-diagnostic context (e.g., strings containing these patterns, unrelated uses)
   - Zero diagnostic-only `std::cerr`, `fprintf(stderr)`, `printf()`, or `write(STDERR_FILENO)` statements.
3. **Smoke test**: Run `buddd run cube --capture 1:/tmp/test.png` and verify:
   - Output appears as `[LEVEL] [Tag] message` lines on stderr
   - All lifecycle messages present (Window opened, Scene started, Captured, Scene complete, Window closed)
4. **Log level test**: Run `buddd run cube --capture 1:/tmp/test.png --log-level=error` and verify that INFO and DEBUG messages are suppressed.
5. **Log file test**: Run `buddd run cube --capture 1:/tmp/test.png --log-file=/tmp/buddd_test.log` and verify log file contains formatted output with timestamps.
6. **Test suite**: `ctest --test-dir build` passes (or `cmake --build build --target test`).
7. **Tagged filtering test**: Run `buddd run textured-cube --log-level=error --log-filter=TexturedCube=debug --capture 1:/tmp/test.png` and verify that debug messages from TexturedCube appear while all other messages are suppressed.

## Edge cases

The following edge cases from the spec MUST be handled:

1. **Pre-init logger not available**: `main.cpp` line 37 stays as raw `fprintf(stderr)`. Verify by checking the line is unmodified.

2. **`#ifndef NDEBUG` removal**: All `#ifndef NDEBUG` guards around `std::cerr` diagnostic calls are removed. The `BUDDD_LOG_DEBUG` call is unconditional. Verify by diff.

3. **Empty format arguments**: `std::cerr << "constant string"` → `BUDDD_LOG_INFO("constant string")` — no `\n`, no format args.

4. **Trailing `\n` removal**: Every migrated message drops trailing `\n`. The ConsoleSink adds its own. Verify by diff.

5. **Format string conversion**: `std::cerr << "x=" << x` → `BUDDD_LOG_INFO("x={}", x)`. `<<` concatenation completely eliminated.

6. **`BUDDD_LOG_TAG` not already declared**: Despite spec claim, `main.cpp` and all other files do NOT currently declare `BUDDD_LOG_TAG`. Every modified file MUST get one.

7. **`std::cerr` used for intentional output**: No mixed-use cases exist in the modified files.

8. **Unicode characters**: Pass through as-is (e.g., `\u2014` em dash in `asset_manager.cpp`, `\u2192` arrow in `hot_reload_gltf_app.cpp`).

9. **`std::exit()`/`std::terminate()` removal** (AC-016 "where feasible"): Where FATAL logging is followed by termination calls, the termination call is KEPT if removal would introduce undefined behavior (dereferencing failed `Result`s or falling off a non-void function without returning a value). The following locations KEEP their termination calls (documented in section A.7):
   - `phong_app.cpp`: `make_checkerboard_texture` (2 calls), `make_solid_texture` (2 calls), `create_phong_cube` (1 call) — all return non-`Result` types with no valid non-terminating return path.
   - `phong_material.cpp` `Impl::create_material` (3 calls) — dereferences `*vs`, `*fs`, or `*mat` on next line.
   - `pbr_material.cpp` `Impl::create_material` (3 calls) — same pattern.
   - `render_device_opengl.cpp` `fallback_material()` (3 `std::terminate()` calls) — function returns `Material&`; truly unrecoverable.
   - `render_device_headless.cpp` `fallback_material()` (3 `std::terminate()` calls) — same reason.
   All other `std::exit()`/`std::terminate()` calls after FATAL logging are removed per AC-016.

10. **Multiple `std::cerr` on consecutive lines**: Each becomes its own `BUDDD_LOG_*` call. No combining.

## Security impact

- No elevated permissions needed.
- No new file I/O beyond existing `FileSink`.
- No sensitive data in migrated messages (paths, shader errors, render state only).
- No network access.
- Input validation: none changed (the migrated calls only output data, they don't process untrusted input for logging).

## Data and migration impact

- **Schema changes**: None.
- **Data migration**: None.
- **Seed data**: None.
- **Data loss risk**: None. All diagnostic output is preserved, just routed through the logging system.

## API compatibility impact

- **Public API**: No changes. All existing `BUDDD_LOG_*` macros remain unchanged.
- **CLI output format**: Changes from bare text to `[LEVEL] [Tag] message` on stderr for diagnostic output. User-facing help/usage text unchanged.
- **stdout changes**: `printf()` diagnostic calls (Window opened, Captured, Window closed) now go to stderr via `BUDDD_LOG_INFO`. This is an intentional user-visible change documented in the spec.
- **Backward compatibility**: Test assertions in `cmd_tests.cpp` that check stderr strings must be updated to match the new format.

## Documentation impact

- **README**: None.
- **Wiki pages**: The following wiki pages must be updated (per spec § Documentation to update):
  - `docs/wiki/domain/logging.md` — Best practice #2 must be updated from "A future feature will migrate existing ad-hoc output" to past tense ("has been migrated"). Add the new tags (`App`, `FileWatcher`, `Render:OpenGL`, `Render:Headless`, etc.) to the tag naming convention table.
  - `docs/wiki/architecture/module-map.md` — References to `std::cerr` in `render_system.cpp` (line 230) and other per-file behavior notes should reflect `BUDDD_LOG_*` equivalents.
  - `docs/wiki/domain/business-rules.md` — The table enumerating `fprintf(stderr)`, `std::cerr`, `printf` calls must be updated or removed.
  - `docs/wiki/architecture/data-flow.md` — Lines referencing `fprintf(stderr)` for unknown command/scene output should be updated.
  - `docs/adr/ADR-020-custom-logging-system.md` — The "Negative" consequence about deferred migration should be updated to reflect completion.
- **Other specs**: None.

## ADR impact

No new ADR needed. A minor update to ADR-020 (Consequences — Negative) is warranted to reflect that the deferred migration is now complete. This update should be done by the wiki-agent or governance reviewer, not the code implementer.

## Done criteria

The implementation is complete when ALL of the following are true:

- [ ] **C1**: All 31 source files listed in "Files allowed to change" have been modified to:
  - Include `#include "log/log.h"` (if not already present).
  - Declare `BUDDD_LOG_TAG("...")` at file scope with the correct tag from the spec's tag mapping table.
  - Replace every ad-hoc diagnostic output statement (`std::cerr`, `fprintf(stderr)`, `printf`, `write(STDERR_FILENO)`) with the equivalent `BUDDD_LOG_*` macro using the correct level and message.
  - Remove all trailing `\n` from migrated messages.
  - Convert all `<<` stream concatenation to `std::format`-style `{}` placeholders.
  - Remove all `#ifndef NDEBUG` guards around migrated `std::cerr` calls (the `BUDDD_LOG_DEBUG` call is unconditional).
  - Remove `std::exit()` and `std::terminate()` calls that immediately follow "FATAL" log messages only in locations where removal does NOT introduce UB (the 6 locations in section A.7 that KEEP their termination calls are exempted from removal).
- [ ] **C2**: The existing exempted lines are NOT modified:
  - `main.cpp` line 37 (`fprintf(stderr, "Error: ...")` before `Logger::init`).
  - `main.cpp` usage/help text blocks (fprintf calls for usage text after unknown command/scene).
  - `help_command.cpp`, `version_command.cpp`, `Image::save()`, `hot_reload_gltf_app.cpp` `std::ofstream` data writes.
  - All logging system implementation files (`log/console_sink.cpp`, `log/file_sink.cpp`, `log/log.h`, `log/logger.cpp`, `log/log_filter.cpp`).
- [ ] **C3**: Build succeeds: `cmake --build build` compiles with zero errors and zero new warnings.
- [ ] **C4**: `tests/cmd_tests.cpp` assertions are updated to match new `[LEVEL] [Tag] message` output format. All test cases pass.
- [ ] **C5**: Grep verification: `grep -rn 'std::cerr\|fprintf(stderr\|printf(\|write(STDERR_FILENO' src/engine/ src/cmd/` returns ONLY exempted lines and false positives (no diagnostic ad-hoc output remains).
- [ ] **C6**: Smoke test: Running `buddd run cube --capture 1:/tmp/test.png` produces `[LEVEL] [Tag]` formatted output on stderr with all lifecycle messages present.
- [ ] **C7**: Log level filtering works: `buddd run cube --log-level=error --capture 1:/tmp/test.png` suppresses INFO/DEBUG messages.
- [ ] **C8**: Log file works: `buddd run cube --log-file=/tmp/buddd_test.log --capture 1:/tmp/test.png` produces a log file with timestamped output.
- [ ] **C9**: No new dependencies introduced.

# Wiki Assessment — SPEC-010 Framebuffer Capture

## Executive Summary

**Wiki updates are required.** The current wiki has no reference to the Framebuffer Capture feature (SPEC-010). All five key wiki pages are missing content about the new `image/` module, `CaptureCommand`, `read_pixels()`, stb dependency, and capture tests. Below is a detailed inventory of every required change.

---

## 1. `docs/wiki/architecture/overview.md`

### 1.1 Directory layout (line ~25)

The tree under `src/engine/` does not include the `image/` subdirectory.

**Change needed:** Add a line after `render/`:
```
│   │   └── image/           # Image I/O (ImageBuffer, Image, stb_image)
```

### 1.2 Engine library internal structure (lines ~56–106)

The tree listing for `src/engine/` does not include the `image/` submodule.

**Change needed:** Add after the `render/` block:
```
├── image/
│   ├── image_buffer.h       # ImageBuffer — raw GPU readback data (width, height, channels, bytes)
│   ├── image.h              # Image — create from buffer, load/save PNG
│   └── image.cpp            # Image implementation (stb_image, stb_image_write, row-flipping)
```

### 1.3 Build system / External dependencies (line 42)

The line listing external dependencies mentions SDL3, GLM, OpenGL, but not stb.

**Change needed:** Append `, stb (fetched via FetchContent)` to the external dependencies sentence.

### 1.4 Key behaviors (lines ~111–118)

The behavior list shows commands `run`, `demo`, `version`, `help`. The `capture` command is missing.

**Change needed:** Add a new bullet (after the `buddd demo` bullet or after the help bullet):
```
- `./build/debug/src/cmd/buddd capture <scenario> [output_path]` — opens an 800×600 window, renders a single frame of the named scenario, captures the framebuffer as a PNG file, and exits. Currently available: `cube` (front view, angle=0, camera at (0,0,3)). If no scenario is given or the scenario is unknown, prints an error to stderr and exits with code 1.
```

### 1.5 Reference section (lines ~147–164)

The reference list goes up to SPEC-009 / IMPL-009.

**Change needed:** Append two entries:
```markdown
- Spec: [SPEC-010](/docs/specs/capture/spec.md) — Framebuffer Capture (ImageBuffer, Image, read_pixels, capture command, cube capture scenario)
- Implementation contract: [IMPL-010](/docs/specs/capture/implementation-contract.md)
```

---

## 2. `docs/wiki/architecture/module-map.md`

### 2.1 New image/ submodule under buddd_engine

**Change needed:** Add a new subsection between the Scene submodule and the Render submodule (or after Render, before the `buddd` CLI section):

````markdown
### Image submodule (`image/`)

All types in namespace `buddd::engine`. Provides pixel buffer representation and PNG I/O via stb_image/stb_image_write. Depends on `error.h` for `Result<T>` types.

| File | Role |
|---|---|
| `image_buffer.h` | `ImageBuffer` aggregate struct — `int width`, `int height`, `int channels`, `std::vector<std::byte> data`. Pure aggregate, no methods. |
| `image.h` | `Image` class — static `create(const ImageBuffer&) -> Result<Image>` (validates, flips rows), static `load(std::string_view) -> Result<Image>` (PNG via stb_image), `save(std::string_view) const -> Result<void>` (PNG via stb_image_write), and accessors. Non-copyable, movable. |
| `image.cpp` | Image implementation: row-flipping logic (bottom-left → top-left), stb_image/stb_image_write implementation via `#define STB_IMAGE_IMPLEMENTATION` / `STB_IMAGE_WRITE_IMPLEMENTATION`. |
````

### 2.2 Render submodule — render_device.h update

In the render submodule table, the `render_device.h` entry (line 90) should note the addition of `read_pixels()`.

**Change needed:** Update the `render_device.h` row description from:
> `create_shader`, `create_material`, `create_vertex_buffer`, `create_index_buffer`, and draw methods (`draw`, `draw_indexed`)

to:
> `create_shader`, `create_material`, `create_vertex_buffer`, `create_index_buffer`, `read_pixels()`, and draw methods (`draw`, `draw_indexed`)

### 2.3 RenderDeviceOpenGL and RenderDeviceHeadless

Update their descriptions to mention `read_pixels()`.

**Change needed:**
- `render_device_opengl.h`: append `and `glReadPixels` framebuffer readback`
- `render_device_headless.h`: append `and unconditional `read_pixels()` error`

### 2.4 Error handling module — error.h

The `Error::Category` enum now has `ReadbackFailed` and `IoFailed`.

**Change needed:** Update the `error.h` description to mention:
> `Error::Category` now includes: `InitFailed`, `WindowCreationFailed`, `RenderDeviceCreationFailed`, `ShaderCompilationFailed`, `LinkingFailed`, `ResourceCreationFailed`, `InvalidArgument`, `UniformNotFound`, `ReadbackFailed`, `IoFailed`, `Unsupported`, `Unknown`

### 2.5 CLI — capture command

Under the `buddd` CLI section, `### Command files (src/cmd/commands/)`, add:

```markdown
| File | Role |
|---|---|
| `capture_command.h` / `capture_command.cpp` | `buddd::cmd::CaptureCommand` — parses scenario and optional output path, creates SDL3 platform, window, and render device, delegates to a scenario function, saves the resulting PNG. Registered via `else if (cmd == "capture")` in `main.cpp`. |
```

### 2.6 CLI — capture files

Under the `buddd` CLI section, add a new subheading after `### Demo files`:

```markdown
### Capture files (`src/cmd/capture/`)

Each capture scenario is a `.h`/`.cpp` pair exposing a single free function in the `buddd::cmd::capture` namespace.

| File | Role |
|---|---|
| `cube_capture.h` / `cube_capture.cpp` | Declares `buddd::cmd::capture::capture_cube_scene()` — renders one frame of the cube (reusing `setup_cube()`) from camera position (0,0,3) with angle=0, calls `read_pixels()`, returns the raw `ImageBuffer`. |
```

### 2.7 CLI — subcommand behavior

Update the subcommand behavior list to include `capture`:

```markdown
- `buddd capture <scenario> [output_path]` → opens 800×600 window titled "Buddd Engine — Capture: \<scenario\>", renders one frame, captures as PNG, saves to `output_path` (or `/tmp/buddd_capture_<scenario>_<timestamp>.png`), prints `"Captured: <path>"`. Currently available: `cube`. If no scenario is given, prints usage to stderr and exits 1. If scenario is unknown, prints error to stderr and exits 1.
```

### 2.8 Help command — k_usage_text

The help usage text now includes `capture`. The wiki should note that `help_command.h` was updated.

### 2.9 Test files

Under `buddd_tests`, add:

| File | Role |
|---|---|
| `image_tests.cpp` | Image unit tests (tagged `[image]`): ImageBuffer aggregate, Image::create validation, row-flipping, save/load round-trip, load error cases, copy/move semantics, accessors, save error cases. All headless (CPU-only). |

Also update the `cmd_tests.cpp` entry to note capture tests were added.

### 2.10 Reference section

Append SPEC-010 / IMPL-010.

---

## 3. `docs/wiki/architecture/data-flow.md`

### 3.1 CLI data flow diagram (lines ~7–23)

The dispatch tree is missing the `capture` branch.

**Change needed:** Add a branch after `"demo"`:
```
│       ├── argv[1] == "capture" ──► CaptureCommand.run(argc, argv)
```

### 3.2 CLI output table (lines ~28–36)

Add a row for the `capture` command:

| Command | stdout | stderr |
|---|---|---|
| `capture <scenario> [path]` | `"Captured: <path>"` | `"Capturing: <scenario>"` then error or success. If no scenario: `"Usage: buddd capture <scenario> [output_path]"` + scenario list. If unknown scenario: `"Unknown capture scenario: '<name>'"` + usage. If extra args: `"Warning: unexpected arguments..."`. |

### 3.3 Error propagation — Error::Category (line ~124)

The list of `Error::Category` values is missing `ReadbackFailed` and `IoFailed`.

**Change needed:** Append:
> - `ReadbackFailed` — GPU framebuffer readback error (glReadPixels failure)
> - `IoFailed` — File I/O error (read/write image file)

### 3.4 Reference section

Append SPEC-010 / IMPL-010.

---

## 4. `docs/wiki/architecture/dependency-map.md`

### 4.1 Target dependencies diagram (lines ~5–13)

The diagram needs stb added as a dependency of `buddd_engine`:

```
buddd ──PRIVATE──► buddd_engine ──PUBLIC──► SDL3::SDL3
                       │                    ├──► OpenGL::GL
                       │                    ├──► glm::glm
                       │                    └──► stb (PRIVATE, FetchContent)
                       │
buddd_tests ──PRIVATE─┤
              ──PRIVATE──► Catch2::Catch2WithMain
```

### 4.2 Target dependencies table (lines ~17–24)

Add a row for stb:

| `buddd_engine` | `stb` | PRIVATE | Single-header public-domain library for PNG I/O (stb_image + stb_image_write). Fetched via FetchContent. |

### 4.3 External dependencies table (lines ~29–33)

Add:

| **stb** | `31c1ad37456438565541f9958214b6e762fb4` | `https://github.com/nothings/stb.git` | CMake `FetchContent` (automatic download at configure time) — header-only, only `#include` path needed. |

### 4.4 Key constraints / Architecture boundary

Add mention that stb is included privately in `src/engine/image/` only, and is not exposed outside the engine.

### 4.5 Reference section

Append SPEC-010 / IMPL-010.

---

## 5. `docs/wiki/engineering/testing.md`

### 5.1 New test file: `image_tests.cpp`

Add a new subsection (suggested after the Model/cube tests section):

```markdown
### Image / capture tests

The image test suite (`tests/image_tests.cpp`) provides unit tests for `ImageBuffer`, `Image`, PNG I/O, and error handling. All tests are **headless** (no display, no GPU required) — they use stb_image and stb_image_write which are CPU-only operations. The test file is registered in `tests/CMakeLists.txt` (auto-discovered via the `*_tests.cpp` glob).

Tags used: `[image]`.

| Category | Test case coverage |
|---|---|
| ImageBuffer | Default construction (zero-initialised aggregate) |
| Image::create validation | Zero width, zero height, zero channels, mismatched data size — each returns `InvalidArgument` error |
| Row-flipping | 4×2 greyscale buffer: bottom row = 0xFF, top row = 0x00; after create, image row 0 = 0x00, row 1 = 0xFF |
| Save/load round-trip | Create, save to temp PNG, verify magic bytes (`\x89PNG`), load back, compare pixel data |
| Load error cases | Non-existent file → `IoFailed`; corrupt file → `IoFailed` |
| Copy/move semantics | Static asserts for non-copyable; move construction transfers ownership; moved-from source has empty data |
| Accessors | width/height/channels/data() return stored values |
| Save error cases | Non-existent directory → `IoFailed`; path is a directory → `IoFailed` |
```

### 5.2 Capture CLI tests

Under the CLI tests section (line ~46), add rows to the existing table:

| Test case | Verification |
|---|---|
| `buddd capture with no args prints usage and exits 1` | stderr contains `"Usage: buddd capture <scenario>"`; exit code 1 |
| `buddd capture unknown_scenario prints error and exits 1` | stderr contains `"Unknown capture scenario: 'unknown_scenario'"`; exit code 1 |
| `buddd help output includes capture` | stdout contains `"capture"` in command list |

### 5.3 Headless read_pixels test

If a test for `RenderDeviceHeadless::read_pixels()` exists in `render_device_tests.cpp` or similar, add it to the headless platform abstraction tests table. (The spec calls for extending `tests/render_device_test.cpp` — verify whether this was done and add the entry.)

### 5.4 Reference section

Append SPEC-010 / IMPL-010.

---

## Summary of Changes

| Wiki page | Changes required |
|---|---|
| `overview.md` | Directory layout, engine tree, build deps, key behaviors, reference section |
| `module-map.md` | New image/ submodule, render_device.h/OpenGL/Headless updates, error.h update, capture command + capture files, subcommand behavior, test files, reference section |
| `data-flow.md` | CLI dispatch diagram, CLI output table, Error::Category list, reference section |
| `dependency-map.md` | Target deps diagram + table, external deps table, reference section |
| `testing.md` | New image tests section, capture CLI test table, reference section |

The `domain/` and `decisions/` wiki pages do not need updates — no domain concepts were changed and no new ADRs were accepted.

## Order of Application

1. `dependency-map.md` — simplest, atomic addition of stb
2. `data-flow.md` — small additions to dispatch and error tables
3. `testing.md` — test documentation
4. `module-map.md` — largest number of additions across engine and CLI
5. `overview.md` — architecture-level summary of all the above

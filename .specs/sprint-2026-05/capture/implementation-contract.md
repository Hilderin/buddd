# IMPL-010 — Framebuffer Capture

## Status

`Accepted`

Allowed values: `Draft`, `In Review`, `Accepted`

## Approval

> This section is filled when the human validates the spec and implementation contract, authorizing implementation to proceed.

| | |
|---|---|---|
| Approved by | Guillaume (human) |
| Date | 2026-05-30 |
| Time | ~11:30 UTC |

## Source spec

`.specs/sprint-2026-05/capture/spec.md` (SPEC-010), accepted.

All blocking issues (B-01 through B-03) from the spec-critic review (`.specs/sprint-2026-05/capture/spec-critic.md`) have been resolved in the accepted spec:

| Issue | Resolution |
|---|---|
| B-01 (untagged stb dependency) | stb pinned to commit hash `31c1ad37456438565541f4919958214b6e762fb4` |
| B-02 (Image::save channel coverage) | Documented support for 1–4 channels |
| B-03 (cross-directory render_device.h → image_buffer.h) | Include explicitly added to spec |

## Goal

Implement framebuffer capture for the Buddd Engine:

1. **ImageBuffer** aggregate struct (`src/engine/image/image_buffer.h`) — raw GPU readback data (width, height, channels, byte vector). Bottom-left origin (OpenGL convention).
2. **Image** class (`src/engine/image/image.h`, `src/engine/image/image.cpp`) — static factories `create(ImageBuffer)` (validates + row-flips), `load(path)` (stb_image PNG load), `save(path)` (stb_image_write PNG save). Non-copyable, movable.
3. **RenderDevice::read_pixels()** — pure virtual returning `Result<ImageBuffer>`. OpenGL backend calls `glReadPixels`. Headless backend returns `Unsupported` error.
4. **Error::Category** — adds `ReadbackFailed` and `IoFailed` values.
5. **stb integration** — CMake FetchContent for stb single-header library (commit `31c1ad...`).
6. **CaptureCommand** (`src/cmd/commands/capture_command.h/.cpp`) — `buddd capture <scenario> [output_path]` CLI command, SDL3 backend unconditionally.
7. **"cube" capture scenario** (`src/cmd/capture/cube_capture.h/.cpp`) — reuses `setup_cube()` from demo_helpers, camera at (0,0,3), angle=0, single frame.

## Non-goals

- No `image_analyzer` subagent or image verification/validation of captured output.
- No multi-frame capture or video recording.
- No capture scenarios beyond `"cube"` (future spec).
- No modification to existing demos (`triangle`, `cube`) or `demo_helpers` beyond their current API. `setup_cube` is reused, not modified.
- No headless capture support — `read_pixels()` in the headless backend returns an error.
- No GUI or editor integration for captures.
- No texture system integration (`Image::load()` exists for future use but is not wired into any texture pipeline).
- No changes to `Window`, `Platform`, or other engine abstractions beyond the additions specified here.
- No image format conversion beyond the row-flip (bottom-left to top-left) in `Image::create()`.
- No colour space conversion, gamma correction, premultiplied alpha, or HDR capture.

## Relevant constitution rules

- **CONST-001-architecture-boundaries.md**: Enforces the architecture boundary — no code outside `src/engine/` may include SDL3, OpenGL, or GLM headers. All new types in `src/engine/image/` must not expose backend types. `CaptureCommand` and `cube_capture.h/.cpp` in `src/cmd/` must use engine abstractions only.
- **CONST-002-testing-policy.md**: Requires unit tests for all testable code. This contract specifies required tests (see Required tests section).

## Relevant ADRs

- **ADR-001** (`docs/adr/001-result-error-pattern.md`): Establishes `Result<T>` / `Error` as the project-wide error handling pattern. This contract extends `Error::Category` with new values (`ReadbackFailed`, `IoFailed`). `ReadPixels()`, `Image::create()`, `Image::load()`, `Image::save()` all return `Result<T>`.
- **ADR-003** (`docs/adr/003-render-pipeline-architecture.md`): Draw methods return `void`. `read_pixels()` returns `Result<ImageBuffer>` (it is not on a hot path and its failure is not a precondition violation).
- **ADR-004** (`docs/adr/004-demo-system-architecture.md`): Commands follow the established pattern. `CaptureCommand` follows the same `.h`/`.cpp` pair, `run(int, const char* const*) -> int` signature, and extra-args-warning pattern as `DemoCommand`.

## Files to inspect

| File | Purpose |
|---|---|
| `src/engine/error.h` | Current `Error::Category` enum — must be extended with `ReadbackFailed` and `IoFailed`. |
| `src/engine/render/render_device.h` | Current abstract `RenderDevice` — must gain `read_pixels()` pure virtual. Must add `#include "image/image_buffer.h"`. |
| `src/engine/render/render_device_opengl.h` | Current OpenGL `RenderDevice` — must gain `read_pixels()` override. |
| `src/engine/render/render_device_opengl.cpp` | Current OpenGL implementation — must implement `read_pixels()` with `glReadPixels`. |
| `src/engine/render/render_device_headless.h` | Current Headless `RenderDevice` — must gain `read_pixels()` override. |
| `src/engine/render/render_device_headless.cpp` | Current Headless implementation — must implement `read_pixels()` returning `Unsupported` error. |
| `src/engine/CMakeLists.txt` | Must add FetchContent for stb. |
| `src/cmd/commands/help_command.h` | `k_usage_text` — must add `capture` to command list. |
| `src/cmd/commands/demo_command.cpp` | Reference for command pattern (backend selection, scenario validation, extra args warning). |
| `src/cmd/demo/demo_helpers.h` | `setup_cube()` declaration — used by `cube_capture.cpp`. |
| `src/cmd/demo/demo_helpers.cpp` | `setup_cube()` implementation — reused by `cube_capture.cpp`. |
| `src/cmd/main.cpp` | Current dispatch chain — must add `"capture"` branch. |
| `src/cmd/CMakeLists.txt` | Current glob — must add `capture/*.cpp`. |
| `tests/cmd_tests.cpp` | Existing CLI tests — must add capture validation tests. |
| `tests/test_helpers.h` | `run_buddd()` helper — used by CLI tests. |
| `tests/CMakeLists.txt` | Test glob — new `_test.cpp` files are auto-discovered. |

## Files allowed to change

### New files to create (9 files)

All paths are relative to the repository root.

| # | File | Purpose |
|---|---|---|
| 1 | `src/engine/image/image_buffer.h` | `ImageBuffer` aggregate struct (`int width/height/channels`, `std::vector<std::byte> data`). |
| 2 | `src/engine/image/image.h` | `Image` class declaration: `create()`, `load()`, `save()`, accessors. Includes stb headers (without IMPLEMENTATION macros). |
| 3 | `src/engine/image/image.cpp` | `Image` implementation: row-flipping, validation, stb_image/stb_image_write calls. Includes stb headers WITH `STB_IMAGE_IMPLEMENTATION` and `STB_IMAGE_WRITE_IMPLEMENTATION`. |
| 4 | `src/cmd/commands/capture_command.h` | `CaptureCommand` class declaration (same pattern as `DemoCommand`). |
| 5 | `src/cmd/commands/capture_command.cpp` | `CaptureCommand` implementation: scenario validation, platform/window/device creation, scenario dispatch, `Image::create()` + `Image::save()`. |
| 6 | `src/cmd/capture/cube_capture.h` | `capture_cube_scene()` function declaration in `buddd::cmd::capture` namespace. |
| 7 | `src/cmd/capture/cube_capture.cpp` | `capture_cube_scene()` implementation: calls `setup_cube()`, renders one frame at angle=0, camera at (0,0,3), calls `device.read_pixels()`. |
| 8 | `tests/image_tests.cpp` | Unit tests for `ImageBuffer` and `Image`: validation, row-flipping, save/load round-trip, error cases, copy/move semantics. |
| 9 | `tests/render_device_tests.cpp` | Unit test for `RenderDeviceHeadless::read_pixels()` error return. |

### Files to modify (11 files)

| # | File | Change |
|---|---|---|
| 1 | `src/engine/error.h` | Add `ReadbackFailed` and `IoFailed` to `Error::Category` (before `Unknown`). Update `to_string()` switch. |
| 2 | `src/engine/render/render_device.h` | Add `#include "image/image_buffer.h"`. Add `read_pixels() -> Result<ImageBuffer> = 0` pure virtual method. |
| 3 | `src/engine/render/render_device_opengl.h` | Add `read_pixels() -> Result<ImageBuffer> override` declaration. No new includes needed. |
| 4 | `src/engine/render/render_device_opengl.cpp` | Implement `read_pixels()`: call `size()` for dimensions, `glPixelStorei(GL_PACK_ALIGNMENT, 1)`, `glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,data)`, check `glGetError()`, return `ImageBuffer` with `channels=4`. |
| 5 | `src/engine/render/render_device_headless.h` | Add `read_pixels() -> Result<ImageBuffer> override` declaration. `#include "image/image_buffer.h"` needed for return type. |
| 6 | `src/engine/render/render_device_headless.cpp` | Implement `read_pixels()`: return `make_error(Error::Category::Unsupported, "read_pixels is not supported in headless mode")`. |
| 7 | `src/engine/CMakeLists.txt` | Add FetchContent for stb library. Add `${stb_SOURCE_DIR}` as PUBLIC include directory (required because `image.h` includes stb headers). |
| 8 | `src/cmd/main.cpp` | Add `#include "commands/capture_command.h"`. Add `if (cmd == "capture")` dispatch branch before the "Unknown command" fallthrough. |
| 9 | `src/cmd/CMakeLists.txt` | Add `${CMAKE_CURRENT_SOURCE_DIR}/capture/*.cpp` to the existing `file(GLOB_RECURSE ...)`. |
| 10 | `src/cmd/commands/help_command.h` | Add `"  capture     Capture a rendered scene to a PNG file\n"` to `k_usage_text`. |
| 11 | `tests/cmd_tests.cpp` | Add capture CLI tests: `buddd capture` (no args), `buddd capture unknown` (unknown scenario), `buddd help` includes `capture`. |

## Files forbidden to change

- Any file outside the explicitly listed "Files allowed to change" set.
- `src/engine/version.h`, `src/engine/version.cpp`.
- `src/engine/math/` (any file) — math types are stable.
- `src/engine/platform/` (any file) — platform abstractions are stable.
- `src/engine/window/` (any file) — window abstraction is stable.
- `src/cmd/demo/` (any file) — demo helpers and demo implementations are stable (not to be modified; `setup_cube` is reused, not changed).
- `src/cmd/commands/demo_command.h`, `src/cmd/commands/demo_command.cpp` — demo command is stable.
- `src/cmd/commands/run_command.h`, `src/cmd/commands/run_command.cpp` — run command is stable.
- `src/cmd/commands/version_command.h`, `src/cmd/commands/version_command.cpp` — version command is stable.
- `docs/adr/` (any file).
- `docs/constitution/` (any file).
- `docs/wiki/` (any file).
- Root `CMakeLists.txt`, `CMakePresets.json`, `.clang-format`, `.vscode/`, `AGENTS.md`, `opencode.json`.

## Existing conventions to follow

| Convention | Rule |
|---|---|
| Namespace | `buddd::engine` for `ImageBuffer`, `Image`, and `Error` extensions. `buddd::cmd` for `CaptureCommand`. `buddd::cmd::capture` for capture scenario functions. |
| File naming | `snake_case` — `image_buffer.h`, `capture_command.h`, `cube_capture.h`. |
| Class/Struct naming | PascalCase — `ImageBuffer`, `Image`, `CaptureCommand`. |
| Header guards | `#pragma once` only (no `#ifndef` guards). |
| Function style | Trailing return type syntax (`auto foo() -> int`). |
| Formatting | `.clang-format` at repo root: LLVM style, 4-space indent, 100 column limit. |
| Includes in engine | Use `"image/image_buffer.h"` — quoted paths relative to `src/engine/`, resolved via PUBLIC include directory. Standard library includes use `<>`. |
| Includes in cmd | Use `"commands/capture_command.h"` — quoted paths relative to `src/cmd/`. |
| Include order | 1. Standard library headers (`<>`), 2. Engine headers (`""`), 3. Demo/capture headers (`""`). Empty line between groups. |
| `noexcept` | Non-allocating accessors are `noexcept`. Factory methods (`create`, `load`, `save`) are NOT `noexcept` (they allocate/fail). |
| Move semantics | `Image` is movable but not copyable. |
| Error handling | All fallible methods return `Result<T>`. Use `make_error()` to return errors. |
| Observability | Use `std::cerr` for lifecycle events (compatible with SPEC-002 and SPEC-005 conventions). |
| `[[nodiscard]]` | `CaptureCommand::run()`, `capture_cube_scene()`, and all `Image` factory methods are `[[nodiscard]]`. |
| Demo error handling | `setup_cube` on failure: `std::fprintf(stderr, "FATAL: ...")` + `std::exit(EXIT_FAILURE)` — unchanged, reused as-is. |
| Command error handling | `CaptureCommand` prints errors to `std::cerr` and returns `EXIT_FAILURE` (same as `DemoCommand`). |

## Required implementation behavior

### 0. `src/engine/error.h` — Add `Error::Category` values

Add `ReadbackFailed` and `IoFailed` to the enum **before** `Unsupported` (to keep the enum logically grouped — `Unsupported` and `Unknown` stay at the end):

```cpp
enum class Category {
    InitFailed,
    WindowCreationFailed,
    RenderDeviceCreationFailed,
    ShaderCompilationFailed,
    LinkingFailed,
    ResourceCreationFailed,
    InvalidArgument,
    UniformNotFound,
    ReadbackFailed,    // NEW: Framebuffer readback (glReadPixels) failure
    IoFailed,          // NEW: File I/O error (read/write image file)
    Unsupported,
    Unknown
};
```

Update `to_string()` switch:

```cpp
case Error::Category::ReadbackFailed: category_str = "ReadbackFailed"; break;
case Error::Category::IoFailed:       category_str = "IoFailed"; break;
```

**Requirements:**
- New values are added before `Unsupported` (existing values must not be reordered).
- All existing values remain unchanged.
- `to_string()` handles both new values.

### 1. `src/engine/image/image_buffer.h` — ImageBuffer aggregate

```cpp
#pragma once

#include <cstddef>
#include <vector>

namespace buddd::engine {

/// Raw pixel data read back from the GPU framebuffer.
/// Bottom-left origin (OpenGL convention). No methods — pure aggregate.
struct ImageBuffer {
    int width = 0;
    int height = 0;
    int channels = 0;            // 4 for RGBA framebuffer reads
    std::vector<std::byte> data; // raw pixels, size = width * height * channels
};

} // namespace buddd::engine
```

**Requirements:**
- Header-only, no `.cpp` file.
- All fields have default values (zeros for ints, empty vector for data).
- No constructors, no methods, no virtual anything. Pure aggregate.
- `channels` is `int` (not `enum`) — the value 4 is conventional for RGBA readback, but any positive value is valid for `Image::create()`.

### 2. `src/engine/image/image.h` — Image class header

```cpp
#pragma once

#include "error.h"

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

namespace buddd::engine {

struct ImageBuffer; // forward declaration — full definition in image_buffer.h

class Image {
public:
    /// Creates an Image from a raw framebuffer ImageBuffer.
    /// Flips rows vertically (bottom-left → top-left).
    /// Validates dimensions and channels are positive.
    [[nodiscard]] static auto create(const ImageBuffer& buffer) -> Result<Image>;

    /// Loads a PNG image from disk using stb_image.
    [[nodiscard]] static auto load(std::string_view path) -> Result<Image>;

    /// Writes the image to disk as a PNG file using stb_image_write.
    [[nodiscard]] auto save(std::string_view path) const -> Result<void>;

    /// Accessors
    auto width() const noexcept -> int;
    auto height() const noexcept -> int;
    auto channels() const noexcept -> int;
    auto data() const noexcept -> const std::vector<std::byte>&;

    // Non-copyable, movable
    Image(const Image&) = delete;
    auto operator=(const Image&) -> Image& = delete;
    Image(Image&&) noexcept = default;
    auto operator=(Image&&) noexcept -> Image& = default;

    ~Image() = default;

private:
    Image() = default;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
    std::vector<std::byte> data_;
};

} // namespace buddd::engine
```

**Requirements:**
- `image.h` does NOT include any stb headers. stb headers are included only in `image.cpp` (with the `IMPLEMENTATION` macros). This keeps stb as a private implementation dependency.
- The forward declaration of `ImageBuffer` is sufficient for `create(const ImageBuffer&)` (reference parameter — incomplete type is allowed).
- `Image` is non-copyable (`= delete`), movable (`= default`).
- `create()` takes `const ImageBuffer&` (not by value).
- `load()` and `save()` use `std::string_view` for path arguments (consistent with the spec).

### 3. `src/engine/image/image.cpp` — Image implementation

**Includes:**

```cpp
#include "image/image.h"
#include "image/image_buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <string>
```

The `STB_IMAGE_IMPLEMENTATION` and `STB_IMAGE_WRITE_IMPLEMENTATION` macros are defined **before** their respective includes. This is the canonical stb single-header pattern: each `IMPLEMENTATION` macro is defined exactly once in one translation unit before including the corresponding header. `image.h` does not include stb, so there is no risk of double inclusion or ODR violations.

**`Image::create(const ImageBuffer& buffer)`:**
1. Validate `buffer.width > 0`, `buffer.height > 0`, `buffer.channels > 0`. If any fails, return `make_error(Error::Category::InvalidArgument, "ImageBuffer dimensions must be positive")`.
2. Validate `buffer.data.size() == static_cast<size_t>(buffer.width) * static_cast<size_t>(buffer.height) * static_cast<size_t>(buffer.channels)`. If mismatch, return `make_error(Error::Category::InvalidArgument, "ImageBuffer data size does not match dimensions")`.
3. Create a `data_` vector of the same size.
4. **Row-flip**: For each row `r` (0 = bottom in the buffer), copy the bytes for that row to position `(height - 1 - r)` in the output `data_` vector. Each row is `width * channels` bytes.
5. Set `width_ = buffer.width`, `height_ = buffer.height`, `channels_ = buffer.channels`.
6. Return the constructed `Image`.

**`Image::load(std::string_view path)`:**
1. Call `stbi_load(path.data(), &w, &h, &channels_in_file, 0)`. The `0` parameter means "request the number of channels in the file" (no conversion).
2. If `stbi_load` returns `nullptr`, return `make_error(Error::Category::IoFailed, ...)` with `stbi_failure_reason()` appended.
3. stb_image loads with top-left origin — no row flip needed.
4. Copy pixel data from the `unsigned char*` returned by stb_image into a `std::vector<std::byte>`.
5. Call `stbi_image_free(decoded_data)`.
6. Return the constructed `Image` with `width = w`, `height = h`, `channels = channels_in_file`.

**`Image::save(std::string_view path) const`:**
1. Call `stbi_write_png(path.data(), width_, height_, channels_, data_.data(), width_ * channels_)`.
2. The `stride_in_bytes` parameter is `width_ * channels_` (no extra stride).
3. If `stbi_write_png` returns 0 (failure), return `make_error(Error::Category::IoFailed, "Failed to write image: " + path)`.
4. Return `Result<void>{}` (success).

**Accessors:**
```cpp
auto Image::width() const noexcept -> int { return width_; }
auto Image::height() const noexcept -> int { return height_; }
auto Image::channels() const noexcept -> int { return channels_; }
auto Image::data() const noexcept -> const std::vector<std::byte>& { return data_; }
```

### 4. `src/engine/render/render_device.h` — Add `read_pixels()`

Add `#include "image/image_buffer.h"` after the existing includes (before `namespace buddd::engine {`).

Add after the `draw_indexed` declaration and before the copy/move deletions:

```cpp
    /// Reads the current framebuffer contents into an ImageBuffer.
    /// The returned ImageBuffer has bottom-left pixel origin (OpenGL convention).
    /// The caller should use Image::create() to flip rows to top-left origin.
    /// @return ImageBuffer with width, height, channels=4, raw RGBA data.
    ///         Returns an error if the backend does not support readback.
    virtual auto read_pixels() -> Result<ImageBuffer> = 0;
```

**Requirements:**
- `#include "image/image_buffer.h"` is required for the `Result<ImageBuffer>` return type (ImageBuffer must be a complete type).
- No forward declaration of ImageBuffer — the include is required.
- Pure virtual.

### 5. `src/engine/render/render_device_opengl.h` — Add `read_pixels()` override

Add after the `draw_indexed` declaration and before the copy/move deletions:

```cpp
    auto read_pixels() -> Result<ImageBuffer> override;
```

No new includes needed. The base class `render_device.h` already includes `image/image_buffer.h`.

### 6. `src/engine/render/render_device_opengl.cpp` — Implement `read_pixels()`

Add the following implementation after the drawing methods section:

```cpp
auto RenderDeviceOpenGL::read_pixels() -> Result<ImageBuffer> {
    auto [width, height] = size();

    ImageBuffer buffer;
    buffer.width = width;
    buffer.height = height;
    buffer.channels = 4;  // RGBA
    buffer.data.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);

    // Set pixel storage alignment to 1 (tightly packed)
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // Clear previous GL error
    glGetError();

    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data.data());

    GLenum gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        return make_error(Error::Category::ReadbackFailed,
            "glReadPixels failed with error code 0x" + to_hex_string(gl_error));
    }

    return buffer;
}
```

**Requirements:**
- Uses `size()` to retrieve the current framebuffer dimensions (width and height).
- Allocates a buffer of exactly `width * height * 4` bytes.
- Sets `GL_PACK_ALIGNMENT` to 1 before calling `glReadPixels` (ensures tightly packed RGBA bytes).
- Clears the GL error state with `glGetError()` before `glReadPixels` to avoid reporting stale errors.
- Returns `ImageBuffer` with `channels = 4` (RGBA).
- The helper function `to_hex_string(GLenum)` formats a GLenum as `0x` followed by 4 lowercase hex digits (e.g., `"0x0500"` for `GL_INVALID_ENUM`). Implement as a free function in an anonymous namespace at the top of `render_device_opengl.cpp`.
- The include `<GL/gl.h>` is available via the existing `#include <SDL3/SDL_opengl.h>` in `render_device_opengl.cpp`. The functions `glPixelStorei`, `glReadPixels`, `glGetError` are standard GL functions.

### 7. `src/engine/render/render_device_headless.h` — Add `read_pixels()` override

Add after the `draw_indexed` declaration and before the copy/move deletions:

```cpp
    auto read_pixels() -> Result<ImageBuffer> override;
```

Add `#include "image/image_buffer.h"` after the existing includes (required for the `Result<ImageBuffer>` return type).

### 8. `src/engine/render/render_device_headless.cpp` — Implement `read_pixels()`

Add the following implementation after the drawing methods section:

```cpp
auto RenderDeviceHeadless::read_pixels() -> Result<ImageBuffer> {
    return make_error(Error::Category::Unsupported,
        "read_pixels is not supported in headless mode");
}
```

**Requirements:**
- Unconditionally returns an error with category `Unsupported`.
- No framebuffer readback is attempted.
- No memory allocation for pixel data.

### 9. `src/engine/CMakeLists.txt` — Add FetchContent for stb

Add AFTER the existing SDL3 and GLM FetchContent blocks, BEFORE the `find_package(OpenGL REQUIRED)`:

```cmake
# ----- stb (single-header image library) -----
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 31c1ad37456438565541f4919958214b6e762fb4
)
FetchContent_MakeAvailable(stb)
```

Add AFTER the `target_include_directories(buddd_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})` line:

```cmake
target_include_directories(buddd_engine PRIVATE ${stb_SOURCE_DIR})
```

**PRIVATE visibility** because stb is only used internally in `image.cpp`. No public header depends on stb includes.

### 10. `src/cmd/commands/capture_command.h` — CaptureCommand declaration

```cpp
#pragma once

namespace buddd::cmd {

class CaptureCommand {
public:
    /// Parses argv[2] as a scenario name, validates it BEFORE creating resources
    /// (fails fast), then creates a platform/window/device (800×600, SDL3 backend
    /// unconditionally), runs the scenario to capture one frame, writes a PNG file,
    /// and exits.
    ///
    /// Signature: buddd capture <scenario> [output_path]
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd
```

**Requirements:**
- Same pattern as `DemoCommand` header.
- No includes beyond `#pragma once` (forward declarations and includes are in the `.cpp`).

### 11. `src/cmd/commands/capture_command.cpp` — CaptureCommand implementation

**Includes:**
```cpp
#include "commands/capture_command.h"
#include "capture/cube_capture.h"

#include "image/image.h"
#include "image/image_buffer.h"
#include "platform/platform.h"
#include "window/window.h"
#include "render/render_device.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <string_view>
```

**Implementation logic:**

```cpp
namespace be = buddd::engine;
namespace bc = buddd::cmd;

namespace {

inline constexpr std::string_view k_capture_usage =
    "Usage: buddd capture <scenario> [output_path]\n"
    "\n"
    "Available scenarios:\n"
    "  cube    Capture a single frame of the rotating cube demo\n"
    "\n"
    "Scenario names are case-sensitive.\n";

struct Scenario {
    std::string_view name;
    // Function pointer type for future extensibility
};

/// Returns true if the given scenario name is valid.
auto is_valid_scenario(std::string_view name) -> bool {
    return name == "cube";
}

/// Generates a default output path: /tmp/buddd_capture_<scenario>_<timestamp>.png
auto default_output_path(std::string_view scenario) -> std::string {
    auto now = std::time(nullptr);
    return "/tmp/buddd_capture_" + std::string(scenario) + "_"
         + std::to_string(static_cast<long>(now)) + ".png";
}

} // anonymous namespace

auto bc::CaptureCommand::run(int argc, const char* const* argv) -> int {
    // No scenario provided
    if (argc < 3) {
        std::fwrite(k_capture_usage.data(), 1, k_capture_usage.size(), stderr);
        return EXIT_FAILURE;
    }

    const std::string_view scenario{argv[2]};

    // Validate scenario BEFORE creating any resources (fails fast on CI/headless)
    if (!is_valid_scenario(scenario)) {
        std::fprintf(stderr, "Unknown capture scenario: '%s'\n\n", argv[2]);
        std::fwrite(k_capture_usage.data(), 1, k_capture_usage.size(), stderr);
        return EXIT_FAILURE;
    }

    // Determine output path
    std::string output_path;
    if (argc >= 4) {
        output_path = argv[3];
    } else {
        output_path = default_output_path(scenario);
    }

    // Create platform (SDL3 unconditionally — headless capture is unsupported)
    auto platform = be::Platform::create(be::Backend::SDL3);
    if (!platform) {
        std::cerr << "Failed to create platform: "
                  << be::to_string(platform.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Build window title
    auto window_title = std::string("Buddd Engine \u2014 Capture: ") + std::string(scenario);
    auto window = (*platform)->create_window({
        .title = window_title,
        .width = 800,
        .height = 600
    });
    if (!window) {
        std::cerr << "Failed to create window: "
                  << be::to_string(window.error()) << "\n";
        return EXIT_FAILURE;
    }

    auto device = be::RenderDevice::create(**window);
    if (!device) {
        std::cerr << "Failed to create render device: "
                  << be::to_string(device.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Warn about unexpected extra arguments (argv[4] and beyond)
    if (argc > 4) {
        std::fprintf(stderr, "Warning: unexpected arguments after 'capture %s':",
                     argv[2]);
        for (int i = 4; i < argc; ++i) {
            std::fprintf(stderr, " %s", argv[i]);
        }
        std::fprintf(stderr, "\n");
    }

    // Observability: capture start
    std::fprintf(stderr, "Capturing: %s\n", scenario.data());

    // Dispatch to scenario function
    // Current implementation supports only "cube". Future scenarios add else-if branches.
    be::Result<be::ImageBuffer> readback_result = be::make_error(
        be::Error::Category::Unknown, "Unknown scenario"); // unreachable
    if (scenario == "cube") {
        readback_result = buddd::cmd::capture::capture_cube_scene(
            **platform, **device, 800, 600);
    }

    if (!readback_result) {
        std::cerr << "Capture failed: " << be::to_string(readback_result.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Convert raw buffer to Image (row-flip + validation)
    auto image = be::Image::create(*readback_result);
    if (!image) {
        std::cerr << "Image processing failed: " << be::to_string(image.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Write PNG
    auto save_result = image->save(output_path);
    if (!save_result) {
        std::cerr << "Failed to write image: " << be::to_string(save_result.error()) << "\n";
        return EXIT_FAILURE;
    }

    // Observability: capture succeeded
    std::printf("Captured: %s\n", output_path.c_str());
    return EXIT_SUCCESS;
}
```

**Requirements:**
- Scenario validation happens BEFORE `Platform::create()` (fails fast). This is verified by ordering: `is_valid_scenario()` runs first, then `Platform::create()`.
- Uses `be::Backend::SDL3` unconditionally — no compile-time `BUDDD_HAS_DISPLAY` switch. On headless builds, `Platform::create(SDL3)` fails at runtime with an error message.
- Default output path: `/tmp/buddd_capture_<scenario>_<timestamp>.png` using `std::time(nullptr)` for the Unix timestamp.
- Extra args warning follows the same pattern as `demo_command.cpp` lines 90-97.
- Observability: `"Capturing: <scenario>\n"` to stderr, `"Captured: <path>\n"` to stdout.
- Error handling at every step (platform creation, window creation, device creation, read_pixels, Image::create, Image::save) prints the error to stderr and returns `EXIT_FAILURE`.
- The default-output-path branch is triggered when `argc < 4` (no output path provided).

### 12. `src/cmd/capture/cube_capture.h` — Cube capture scenario declaration

```cpp
#pragma once

#include "image/image_buffer.h" // for Result<ImageBuffer> return type

namespace buddd::engine {
class Platform;
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::capture {

/// Captures a single frame of the rotating cube scene.
/// Sets up the cube via setup_cube(), renders one frame with
/// the camera at position (0, 0, 3), then reads back the framebuffer.
///
/// @param platform  The engine platform (for event polling).
/// @param device    The render device (for rendering and readback).
/// @param window_w  Window width in pixels.
/// @param window_h  Window height in pixels.
/// @return An ImageBuffer containing the raw (bottom-left origin) framebuffer
///         contents, or an error.
[[nodiscard]] auto capture_cube_scene(
    buddd::engine::Platform& platform,
    buddd::engine::RenderDevice& device,
    int window_w,
    int window_h
) -> buddd::engine::Result<buddd::engine::ImageBuffer>;

} // namespace buddd::cmd::capture
```

**Requirements:**
- Only forward-declares `Platform` and `RenderDevice`. No `#include` of any engine headers or backend-specific types.
- `#include "image/image_buffer.h"` is required for the return type.
- Namespace: `buddd::cmd::capture`.
- `capture_cube_scene` is `[[nodiscard]]`.

### 13. `src/cmd/capture/cube_capture.cpp` — Cube capture scenario implementation

**Includes:**
```cpp
#include "capture/cube_capture.h"
#include "demo/demo_helpers.h"

#include "platform/platform.h"
#include "render/render_device.h"

#include "math/camera.h"
#include "math/math.h"
#include "math/mat4.h"
#include "math/vec3.h"

#include <cstdlib>
#include <iostream>
```

**Implementation (read_pixels called BEFORE end_frame to read back buffer before swap):**

```cpp
auto buddd::cmd::capture::capture_cube_scene(
    be::Platform& platform,
    be::RenderDevice& device,
    int window_w,
    int window_h
) -> be::Result<be::ImageBuffer>
{
    // Setup cube resources (reuses setup_cube from demo_helpers)
    auto cube = buddd::cmd::demo::setup_cube(device);

    // Camera setup: (0, 0, 3) looking at origin, 60° FOV
    be::math::Camera camera;
    camera.look_at(
        be::math::Vec3{0.0f, 0.0f, 3.0f},   // eye
        be::math::Vec3{0.0f, 0.0f, 0.0f},   // centre
        be::math::Vec3::unit_y()              // up
    );
    camera.set_perspective(
        be::math::radians(60.0f),
        static_cast<float>(window_w) / static_cast<float>(window_h),
        0.1f,
        100.0f
    );

    // Poll events once to allow the window manager to map the window
    platform.poll_events();

    // Begin frame (sets viewport, clears colour buffer)
    device.begin_frame();

    // Rotation angle = 0 (front-facing, axis-aligned view)
    be::math::Mat4 model_matrix = be::math::Mat4::identity();
    be::math::Mat4 mvp =
        camera.projection_matrix() * camera.view_matrix() * model_matrix;

    // Set MVP uniform
    cube.material->set_uniform("u_mvp", mvp);

    // Draw the cube (single indexed draw call)
    cube.model.draw(device);

    // Read framebuffer BEFORE buffer swap
    auto buffer = device.read_pixels();
    if (!buffer) {
        // end_frame() is still called to clean up state even on error
        device.end_frame();
        return std::unexpected(buffer.error());
    }

    // End frame (swap buffers)
    device.end_frame();

    return std::move(*buffer);
}
```

**Requirements:**
- Calls `setup_cube(device)` to create cube resources.
- Camera: eye `(0, 0, 3)`, looking at `(0, 0, 0)`, up `Vec3::unit_y()`, perspective 60° FOV with aspect = window_w/window_h, near 0.1, far 100.0.
- Rotation angle = 0 (no rotation). The model matrix is `Mat4::identity()` (or `Mat4::rotate(0.0f, unit_y)` which is equivalent).
- MVP = `projection * view * model` (matrix multiplication order: projection, then view, then model).
- `read_pixels()` is called BEFORE `end_frame()` (within the same begin_frame/end_frame pair, after drawing).
- On readback error, calls `device.end_frame()` before returning the error (to keep OpenGL state consistent).
- Returns the `ImageBuffer` by move.
- A single `platform.poll_events()` call is made before `begin_frame()` to allow the window manager to map the window. This improves reliability across different window systems.

### 14. `src/cmd/main.cpp` — Add capture dispatch

Add `#include "commands/capture_command.h"` after the existing includes.

Add the capture dispatch BEFORE the "Unknown command" fallthrough:

```cpp
    if (cmd == "capture") {
        return bc::CaptureCommand{}.run(argc, argv);
    }
```

**Requirements:**
- The capture dispatch is added as an `else if` chain member, after `help` and before the unknown-command fallthrough.
- No change to existing dispatch branches.

### 15. `src/cmd/CMakeLists.txt` — Add capture glob

Add `${CMAKE_CURRENT_SOURCE_DIR}/capture/*.cpp` to the `file(GLOB_RECURSE ...)`:

```cmake
file(GLOB_RECURSE CMD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commands/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/demo/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/capture/*.cpp    # <-- new
)
```

### 16. `src/cmd/commands/help_command.h` — Update k_usage_text

Add `capture` line after `demo`:

```cpp
inline constexpr std::string_view k_usage_text =
    "Usage: buddd <command> [<args>]\n"
    "\n"
    "Commands:\n"
    "  run       Run the engine in interactive mode (empty window)\n"
    "  demo      Run a demo by name (try 'buddd demo triangle')\n"
    "  capture   Capture a rendered scene to a PNG file\n"
    "  version   Print version information\n"
    "  help      Show this help message\n";
```

## Required tests

### Test file: `tests/image_tests.cpp`

All tests use pure CPU operations — no display required. Tests create `ImageBuffer` instances programmatically and exercise `Image::create()`, `Image::save()`, and `Image::load()`.

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| IT-01 | `"ImageBuffer is default-constructible aggregate"` | `[image]` | `ImageBuffer{}` has width=0, height=0, channels=0, data empty. |
| IT-02 | `"Image::create validates positive dimensions"` | `[image]` | `Image::create(buffer)` with width=0 returns `InvalidArgument`. Same for height=0, channels=0. |
| IT-03 | `"Image::create validates data size matches dimensions"` | `[image]` | `Image::create(buffer)` with width=4, height=4, channels=4, data.size()=0 returns `InvalidArgument`. Same for data.size()=63 (not 64). |
| IT-04 | `"Image::create flips rows correctly"` | `[image]` | Create a 4×2 ImageBuffer with distinct pixel patterns per row (e.g., row 0 = all 0xFF blue, row 1 = all 0x00 black). After `Image::create()`, verify row 0 (top) in the Image equals row 1 (bottom) in the buffer, and row 1 (bottom) in the Image equals row 0 (top) in the buffer. |
| IT-05 | `"Image::save writes valid PNG and Image::load round-trips"` | `[image]` | Create a 4×4 RGBA Image with known pixel data. Save to temp path. Load back via `Image::load()`. Compare `data()` byte-for-byte. Verify saved file starts with `\x89PNG` magic bytes (read first 4 bytes of file). |
| IT-06 | `"Image::load returns error for non-existent file"` | `[image]` | Call `Image::load("/tmp/nonexistent_file_12345.png")`. Error category is `IoFailed`. |
| IT-07 | `"Image::load returns error for corrupt file"` | `[image]` | Write a small non-PNG file (e.g., "not a png") to temp path, call `Image::load()` — error category is `IoFailed`. |
| IT-08 | `"Image is non-copyable"` | `[image]` | `static_assert(!std::is_copy_constructible_v<Image>)` and `static_assert(!std::is_copy_assignable_v<Image>)`. |
| IT-09 | `"Image is movable"` | `[image]` | Move-construct Image from another: moved-to has valid data, moved-from has zero dimensions and empty data. |
| IT-10 | `"Image accessors return stored values"` | `[image]` | Create Image from known buffer; `width()`, `height()`, `channels()`, `data()` return correct values. |
| IT-11 | `"Image::save returns error for unwritable path"` | `[image]` | Create a valid Image, call `save()` with a path to a non-existent directory (e.g., `/nonexistent_dir/out.png`). Error category is `IoFailed`. |
| IT-12 | `"Image::save returns error when path is a directory"` | `[image]` | Create a valid Image, call `save()` with an existing directory path (e.g., `/tmp/`). Error category is `IoFailed`. |

**Total: 12 image test cases.**

### Test file: `tests/render_device_tests.cpp`

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| RT-01 | `"Headless read_pixels returns Unsupported error"` | `[render][headless]` | Create `RenderDeviceHeadless(800, 600)`. Call `read_pixels()`. Verify error category is `Unsupported` and message contains "not supported". |

**Total: 1 render device test case.**

### Test modifications: `tests/cmd_tests.cpp`

Add the following test cases:

| ID | Test Name | Tags | Verification |
|---|---|---|---|
| CT-01 | `"buddd capture with no args prints usage and exits 1"` | `[cli]` | Run `buddd capture`. Exit code 1. Stderr contains `"Usage: buddd capture <scenario>"`. |
| CT-02 | `"buddd capture unknown_scenario prints error and exits 1"` | `[cli]` | Run `buddd capture unknown_scenario`. Exit code 1. Stderr contains `"Unknown capture scenario: 'unknown_scenario'"`. Stderr contains scenario usage text. |
| CT-03 | `"buddd help output includes capture"` | `[cli]` | Run `buddd help`. Stdout contains `"capture"` in the command list. |

### Test linkage to acceptance criteria

| AC ID | Test(s) |
|---|---|
| AC-001 (ImageBuffer exists) | IT-01 |
| AC-002 (Image class exists) | IT-01 through IT-12 compile |
| AC-003 (Image::create validates) | IT-02, IT-03 |
| AC-004 (Image::create row-flip) | IT-04 |
| AC-005 (Image::save writes valid PNG) | IT-05 |
| AC-006 (read_pixels pure virtual) | Code review, RT-01 |
| AC-007 (OpenGL read_pixels) | Manual (requires display) |
| AC-008 (Headless read_pixels error) | RT-01 |
| AC-009 (CaptureCommand exists) | Files exist, CT-01, CT-02 compile |
| AC-010 (main.cpp dispatch) | Code review |
| AC-011 (buddd capture cube /path) | Manual (requires display) |
| AC-012 (default output path) | Manual (requires display) |
| AC-013 (no args usage) | CT-01 |
| AC-014 (unknown scenario) | CT-02 |
| AC-015 (extra args warning) | Manual (requires display) |
| AC-016 (fails fast on unknown scenario) | CT-02 (no display needed — exits before Platform::create) |
| AC-017 (PNG is 800×600) | Manual/external tool |
| AC-018 (build succeeds with stb) | Code review, build verification |
| AC-019 (CONST-001 in src/cmd/) | Code review — grep for SDL3/GL/glm in src/cmd/ |
| AC-020 (existing demos work) | Manual (existing demo tests pass) |
| AC-021 (ReadbackFailed in Error::Category) | Code review + compile-time check |
| AC-022 (capture/*.cpp in glob) | Code review of src/cmd/CMakeLists.txt |
| AC-023 (Image::load fails on missing file) | IT-06 |
| AC-024 (capture_cube_scene reuses setup_cube) | Code review |
| AC-025 (help includes capture) | CT-03 |

## Edge cases

| Edge case | Required behavior |
|---|---|
| `buddd capture` with no scenario | Prints usage to stderr; exits 1. |
| `buddd capture CUBE` (uppercase) | Case-sensitive comparison fails; `"Unknown capture scenario: 'CUBE'"` error. |
| `buddd capture cube /tmp/nonexistent_dir/out.png` | `Image::save()` returns `IoFailed`; error printed to stderr; exits non-zero. |
| `buddd capture cube /dev/null` | `Image::save()` returns error; exits non-zero. |
| `buddd capture cube` on a system with no display | `Platform::create(SDL3)` fails; error printed to stderr; exits non-zero. |
| `buddd capture cube` on a system with display but no write permissions to `/tmp` | `Image::save()` fails with `IoFailed`; exits non-zero. |
| `Image::create()` with `channels != 4` (e.g., 3) | Validation passes (any positive channels is valid). Data is preserved as-is. |
| `Image::create()` with empty data vector | Validation fails: `data.size() != width * height * channels`. Returns `InvalidArgument`. |
| `Image::create()` with negative width or height | Validation fails (`width > 0`, `height > 0`). Returns `InvalidArgument`. |
| `Image::create()` with zero channels | Validation fails (`channels > 0`). Returns `InvalidArgument`. |
| `Image::load()` with a non-PNG file (e.g., `.txt`) | stb_image fails to decode; returns error with category `IoFailed`. |
| `Image::load()` with a truncated PNG file | stb_image fails to decode; returns error with category `IoFailed`. |
| `Image::save()` with 0×0 image (empty data) | stb_image_write produces empty PNG or returns error. The Image class prevents zero-dimension construction by validation in `create()`, but `load()` could return a 0×0 image if the file is empty. In that case, `save()` delegates to stb; if stb fails, returns `IoFailed`. |
| `Image::save()` with a path that is an existing directory | stb_image_write fails to open; returns `IoFailed`. |
| `Image::save()` with extremely large image (OOM) | stb_image_write fails allocation; returns `IoFailed`. |
| `Image::create()` with extremely large dimensions (OOM) | `std::vector` allocation throws `std::bad_alloc`. Caller's responsibility. |
| `Image::load()` with extremely large PNG (OOM) | stb_image fails allocation; returns `IoFailed`. |
| `read_pixels()` called before any rendering | Returns current framebuffer contents (clear colour after `begin_frame()`, or undefined before first `begin_frame()`). No crash. |
| `read_pixels()` called outside `begin_frame()`/`end_frame()` | Undefined behaviour in OpenGL backend. Headless backend returns `Unsupported` error. |
| `read_pixels()` called multiple times in one frame | Each call reads current framebuffer state. Legal but may be expensive. |
| `Image::save()` with 1-channel or 2-channel Image | stb_image_write natively handles 1, 2, 3, 4 channels. The image's channel count is preserved. |

## Security impact

- No elevated privileges required. Default output path `/tmp/` is world-writable on POSIX systems.
- No network access required at runtime.
- No secrets, credentials, or environment variables consumed.
- Architecture boundary (CONST-001) preserved: no SDL3, OpenGL, or GLM headers in `src/cmd/`.
- stb_image and stb_image_write are public-domain single-header libraries, fetched via CMake FetchContent at build time and statically linked into `buddd_engine`.
- stb_image is widely audited and considered safe for decoding untrusted PNGs.
- The caller is responsible for trusting the source of images loaded via `Image::load()`.

## Data and migration impact

None. No schema changes, data migrations, seed data, or persistent state. New files only.

## API compatibility impact

- `RenderDevice` gains a new pure virtual method `read_pixels()`. All existing concrete backends (`RenderDeviceOpenGL`, `RenderDeviceHeadless`) must implement it — no binary compatibility concerns as all backends are in the same project and recompiled together.
- `Error::Category` gains two new values (`ReadbackFailed`, `IoFailed`). This is additive and does not break existing code.
- `ImageBuffer` and `Image` are new public types under `buddd::engine` namespace — no backward compatibility concerns.
- `CaptureCommand` is a new command — no breaking changes.
- The `help` command output text changes to include `capture` — existing parsers of `buddd help` output may need updating.

## Documentation impact

- `src/cmd/commands/help_command.h` is modified with updated `k_usage_text`.
- No README or wiki changes are required as part of this contract (wiki updates are handled separately).

## ADR impact

None. The implementation follows existing ADR decisions:
- ADR-001 (`Result<T>` pattern) is followed for `read_pixels()`, `Image::create()`, `Image::load()`, `Image::save()`.
- No new ADR is required.

## Constitution impact

None. The implementation respects CONST-001 (architecture boundaries — no SDL3/OpenGL/GLM in `src/cmd/`) and CONST-002 (testing policy — all testable code has corresponding tests).

## Done criteria

The implementation is complete when ALL of the following are satisfied:

### Build and compilation
- [ ] `src/engine/image/image_buffer.h` compiles without errors.
- [ ] `src/engine/image/image.h` compiles without errors.
- [ ] `src/engine/image/image.cpp` compiles without errors.
- [ ] `src/engine/render/render_device.h` compiles without errors (modified — new include + pure virtual).
- [ ] `src/engine/render/render_device_opengl.h` compiles without errors (modified — new override).
- [ ] `src/engine/render/render_device_opengl.cpp` compiles without errors (modified — `read_pixels()` implementation).
- [ ] `src/engine/render/render_device_headless.h` compiles without errors (modified — new override + include).
- [ ] `src/engine/render/render_device_headless.cpp` compiles without errors (modified — `read_pixels()` error return).
- [ ] `src/engine/error.h` compiles without errors (modified — new `Error::Category` values).
- [ ] `src/engine/CMakeLists.txt` is modified — FetchContent for stb added with PUBLIC include.
- [ ] `src/cmd/commands/capture_command.h` compiles without errors.
- [ ] `src/cmd/commands/capture_command.cpp` compiles without errors.
- [ ] `src/cmd/capture/cube_capture.h` compiles without errors.
- [ ] `src/cmd/capture/cube_capture.cpp` compiles without errors.
- [ ] `src/cmd/main.cpp` compiles without errors (modified — capture dispatch branch).
- [ ] `src/cmd/CMakeLists.txt` is modified — `capture/*.cpp` glob added.
- [ ] `src/cmd/commands/help_command.h` compiles without errors (modified — `k_usage_text` includes `capture`).
- [ ] `tests/image_tests.cpp` compiles without errors.
- [ ] `tests/render_device_tests.cpp` compiles without errors.
- [ ] `tests/cmd_tests.cpp` compiles without errors (modified — capture CLI tests added).
- [ ] `cmake --build --preset debug` succeeds with no warnings related to new code.
- [ ] Build log shows FetchContent downloading stb on first configure.

### CONST-001 compliance
- [ ] No SDL3, OpenGL, or GLM headers are included from any file under `src/cmd/`. Verified by `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` — zero matches.
- [ ] `capture_cube.h` exposes no backend types (only forward declarations of `Platform` and `RenderDevice`).

### Test results
- [ ] All image tests pass: IT-01 through IT-12 (12 tests).
- [ ] Render device test passes: RT-01 (1 test).
- [ ] CLI capture tests pass: CT-01, CT-02, CT-03 (3 tests).
- [ ] All existing tests pass (CTest: all tests pass after changes).
- [ ] `buddd help` output contains `capture` (manually verified).

### Acceptance criteria coverage
- [ ] AC-001 (ImageBuffer struct): file exists with specified fields.
- [ ] AC-002 (Image class): file exists with specified methods.
- [ ] AC-003 (Image::create validation): covered by IT-02, IT-03.
- [ ] AC-004 (row-flipping): covered by IT-04.
- [ ] AC-005 (save+load round-trip): covered by IT-05.
- [ ] AC-006 (read_pixels pure virtual): code review + RT-01.
- [ ] AC-007 (OpenGL read_pixels with glReadPixels): code review — implementation exists.
- [ ] AC-008 (Headless read_pixels error): covered by RT-01.
- [ ] AC-009 (CaptureCommand files): files exist.
- [ ] AC-010 (main.cpp dispatch): code review — `"capture"` branch added.
- [ ] AC-011 (buddd capture cube /path): manual verification (requires display).
- [ ] AC-012 (default output path): manual verification (requires display).
- [ ] AC-013 (no args usage): covered by CT-01.
- [ ] AC-014 (unknown scenario): covered by CT-02.
- [ ] AC-015 (extra args warning): manual verification (requires display).
- [ ] AC-016 (fails fast): CT-02 exits without creating platform resources (no display needed).
- [ ] AC-017 (PNG is 800×600): manual verification with `identify` or Python.
- [ ] AC-018 (build succeeds with stb): build verification.
- [ ] AC-019 (no SDL3/OpenGL/GLM in src/cmd/): CONST-001 grep check.
- [ ] AC-020 (existing demos work): existing demo tests pass.
- [ ] AC-021 (ReadbackFailed in Error::Category): code review + compile-time check.
- [ ] AC-022 (capture/*.cpp in glob): code review of CMakeLists.txt.
- [ ] AC-023 (Image::load fails on non-existent path): covered by IT-06.
- [ ] AC-024 (capture_cube_scene reuses setup_cube): code review — includes `"demo/demo_helpers.h"` and calls `setup_cube(device)`.
- [ ] AC-025 (help includes capture): covered by CT-03.

### Memory safety
- [ ] ASAN build (`-fsanitize=address`) shows no leaks or use-after-free in tests.
- [ ] All failure paths (Image::create validation errors, Image::load errors, Image::save errors) do not leak prior resources.

### Manual verification (requires display — gated by BUDDD_HAS_DISPLAY)
- [ ] `./buddd capture cube /tmp/test_cube.png` — window opens briefly, file `/tmp/test_cube.png` exists, is non-empty, has PNG magic bytes. Stdout contains `"Captured: /tmp/test_cube.png\n"`. Exit code 0.
- [ ] `./buddd capture cube` — file created at `/tmp/buddd_capture_cube_<timestamp>.png`. Stdout contains the path. Exit code 0.
- [ ] `./buddd capture cube extra_arg` — stderr contains warning, file created, exit code 0.
- [ ] `./buddd demo triangle` and `./buddd demo cube` continue to work unchanged.

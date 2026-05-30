# SPEC-010 — Framebuffer Capture

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

## Problem

The Buddd Engine can render 3D content (triangles, cubes) to a window, but there is no way to capture the rendered output as an image file. This limits:

1. **Automated visual verification**: Without framebuffer capture, all visual testing is manual. There is no way to programmatically verify that a rendered scene looks correct.
2. **Screenshot/demo export**: Developers and users cannot save a frame of the rendered output to a PNG file for sharing, documentation, or debugging.
3. **Future image processing**: Downstream features (image analysis, post-processing, HDR capture) depend on a pixel readback pipeline that starts with raw framebuffer data.

The engine has no pixel readback from the GPU (`glReadPixels` is not called anywhere), no image representation in the engine layer, and no mechanism to write PNG files.

## Goals

- **G-01**: Provide `RenderDevice::read_pixels()` to read the current framebuffer as raw pixel data (`ImageBuffer`).
- **G-02**: Define an `ImageBuffer` struct representing raw GPU readback data (width, height, channels, bytes). Bottom-left origin (OpenGL convention). No processing.
- **G-03**: Define an `Image` class that provides a factory from `ImageBuffer` (with row-flipping and validation), PNG loading from disk, and PNG writing to disk.
- **G-04**: Integrate stb_image and stb_image_write via CMake FetchContent for PNG I/O.
- **G-05**: Add a `buddd capture <scenario> [output_path]` CLI command following the established command pattern (SPEC-006, SPEC-007).
- **G-06**: Implement a `"cube"` capture scenario that renders a single frame of the rotating cube scene (reusing `setup_cube` from SPEC-009) and captures the result.
- **G-07**: Ensure all new types respect the architecture boundary (CONST-001): `ImageBuffer`, `Image`, and stb_image live in `src/engine/`; the CLI command lives in `src/cmd/`.

## Non-goals

- No `image_analyzer` subagent or image verification/validation of captured output.
- No multi-frame capture or video recording.
- No capture scenarios beyond `"cube"` (future spec).
- No modification to existing demos (`triangle`, `cube`) or `demo_helpers` beyond their current API. `setup_cube` is reused, not modified.
- No headless capture support — `read_pixels()` in the headless backend returns an error.
- No GUI or editor integration for captures.
- No texture system integration (the `Image::load()` factory exists for future texture loading but is not wired into any texture pipeline yet).
- No changes to `Window`, `Platform`, or other engine abstractions beyond the additions specified here.
- No image format conversion beyond the row-flip (bottom-left to top-left) in `Image::create()`. Colour space, premultiplied alpha, and channel ordering are preserved as-is from the framebuffer readback.

## Actors

| Actor | Description |
|---|---|
| Engine developer | A developer who needs to capture rendered frames for debugging, testing, or export. Uses `RenderDevice::read_pixels()` and `Image` API directly. |
| End user | A developer running `buddd capture cube` from the CLI to save a PNG of the cube scene. |
| Demo author | A developer who adds new capture scenarios (beyond cube) in future specs. Creates files in `src/cmd/capture/`. |
| CLI maintainer | A developer who maintains the binary entry point, command dispatch, and build system integration. |

## User-visible behavior

### ImageBuffer struct (`src/engine/image/image_buffer.h`)

A simple aggregate struct representing raw pixel data read back from the GPU framebuffer:

```cpp
namespace buddd::engine {

struct ImageBuffer {
    int width = 0;
    int height = 0;
    int channels = 0;            // 4 for RGBA framebuffer reads
    std::vector<std::byte> data; // raw pixels, bottom-left origin (OpenGL convention)
};

}
```

- No methods, no constructors beyond defaults. Pure aggregate.
- `data` size = `width * height * channels` bytes.
- The origin is bottom-left (OpenGL convention). `Image::create()` is responsible for flipping rows when converting to a top-left image.

### Image class (`src/engine/image/image.h`, `src/engine/image/image.cpp`)

A higher-level image abstraction that can be created from a raw framebuffer buffer, loaded from a PNG file, or saved to a PNG file.

```cpp
namespace buddd::engine {

class Image {
public:
    /// Creates an Image from a raw framebuffer ImageBuffer.
    /// Flips rows vertically (bottom-left → top-left).
    /// Validates dimensions and channels are positive.
    static auto create(const ImageBuffer& buffer) -> Result<Image>;

    /// Loads a PNG image from disk.
    static auto load(std::string_view path) -> Result<Image>;

    /// Writes the image to disk as a PNG file.
    auto save(std::string_view path) const -> Result<void>;

    /// Accessors
    auto width() const -> int;
    auto height() const -> int;
    auto channels() const -> int;
    auto data() const -> const std::vector<std::byte>&;

    // Non-copyable, movable (pixel data is expensive to copy)
    Image(const Image&) = delete;
    auto operator=(const Image&) -> Image& = delete;
    Image(Image&&) = default;
    auto operator=(Image&&) -> Image& = default;

private:
    Image() = default;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
    std::vector<std::byte> data_;
};

}
```

**`Image::create(const ImageBuffer& buffer)` behavior**:

1. Validates `buffer.width > 0`, `buffer.height > 0`, `buffer.channels > 0`.
2. Validates `buffer.data.size() == buffer.width * buffer.height * buffer.channels`.
3. Allocates a new `data_` vector of the same size.
4. Flips rows vertically: row `r` (0 = bottom) in the buffer becomes row `(height - 1 - r)` in the image. This converts from OpenGL bottom-left origin to standard top-left origin.
5. Copies pixel data row by row, preserving channel ordering and byte layout.
6. Returns the `Image` on success, or an `Error` with category `InvalidArgument` if validation fails.

**`Image::load(std::string_view path)` behavior**:

1. Uses stb_image to load a PNG file from disk.
2. Returns the image with the loaded pixel data (top-left origin, with 1–4 channels as detected from the file).
3. On failure (file not found, invalid PNG, etc.), returns an `Error` with category `IoFailed` and a descriptive message.

**`Image::save(std::string_view path) const` behavior**:

1. Uses stb_image_write to write the image as a PNG file.
2. Supports writing images with 1–4 channels. The channel count is preserved from the Image's `channels()` value. stb_image_write natively supports 1 (greyscale), 2 (greyscale+alpha), 3 (RGB), and 4 (RGBA) channels.
3. On failure (disk full, permission denied, etc.), returns an `Error` with category `IoFailed` and a descriptive message.

**Accessors**: return the stored values. `data()` returns a const reference to the internal `std::vector<std::byte>`.

### RenderDevice::read_pixels() extension

Add a pure virtual method to the `RenderDevice` abstract class in `src/engine/render/render_device.h`. This creates a new include dependency: `render_device.h` must `#include "image/image_buffer.h"` so that `ImageBuffer` is a complete type for the `Result<ImageBuffer>` return type. This is a cross-directory dependency within `src/engine/` (`render/` → `image/`), which is architecturally acceptable as both are internal engine modules.

```cpp
/// Reads the current framebuffer contents into an ImageBuffer.
/// The returned ImageBuffer has bottom-left pixel origin (OpenGL convention).
/// The caller should use Image::create() to flip rows to top-left origin.
/// @return ImageBuffer with width, height, channels=4, raw RGBA data.
///         Returns an error if the backend does not support readback.
virtual auto read_pixels() -> Result<ImageBuffer> = 0;
```

#### OpenGL backend (`RenderDeviceOpenGL`)

1. Retrieves the framebuffer dimensions from the device's internal `width_`/`height_` (or via `size()`).
2. Allocates a local buffer of size `width * height * 4` bytes.
3. Sets pixel packing alignment: `glPixelStorei(GL_PACK_ALIGNMENT, 1)`.
4. Calls `glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data_ptr)`.
5. If `glGetError()` returns an error after the readback, returns `make_error(Error::Category::ReadbackFailed, ...)`.
6. Returns an `ImageBuffer` with `width`, `height`, `channels=4`, and the raw data (bottom-left origin, not flipped).
7. **Undefined behaviour**: Calling `read_pixels()` outside a `begin_frame()`/`end_frame()` pair results in undefined behaviour (OpenGL state may not be valid). This is consistent with ADR-003's precondition UB for `draw()`.

#### Headless backend (`RenderDeviceHeadless`)

Returns `make_error(Error::Category::Unsupported, "read_pixels is not supported in headless mode")`.

No framebuffer exists in headless mode — this is an unconditional error.

### CLI command: `buddd capture`

- **File**: `src/cmd/commands/capture_command.h` and `src/cmd/commands/capture_command.cpp`.
- **Namespace**: `buddd::cmd`.
- **Class**: `CaptureCommand` following the same pattern as `DemoCommand`, `RunCommand`, etc.
- **Dispatch**: Registered in `main.cpp` via an `else if (cmd == "capture")` branch.
- **Backend selection**: The `CaptureCommand` uses the SDL3 backend unconditionally. Unlike `DemoCommand` and `RunCommand` (which follow the compile-time `BUDDD_HAS_DISPLAY` switch), capturing from the headless backend is not supported — `read_pixels()` unconditionally returns an error in headless mode. On a system without a display, `Platform::create(SDL3)` will fail at runtime, producing a clear error message.

#### Command signature

```
buddd capture <scenario> [output_path]
```

- `<scenario>`: Required. Currently only `"cube"` is valid.
- `[output_path]`: Optional. Defaults to `/tmp/buddd_capture_<scenario>_<timestamp>.png`.

#### Behavior details

1. **No args** (`buddd capture` alone): Prints usage to stderr and exits with code 1.
2. **Unknown scenario** (`buddd capture unknown_scenario`): Prints error to stderr and exits with code 1.
3. **Extra arguments** (more than one positional arg after `<scenario>`): Prints a warning to stderr (following the pattern from `demo_command.cpp` lines 90-97) but proceeds with the capture.
4. **Success flow**:
   a. Validate the scenario name **before** creating any resources (fails fast on headless/CI).
   b. Create Platform (SDL3 backend — skips headless path since capture requires a display).
   c. Create Window (800×600, title `"Buddd Engine — Capture: <scenario>"`).
   d. Create RenderDevice.
   e. Delegate to the scenario function (e.g., `capture_cube_scene`), which:
      - Sets up the scene
      - Renders exactly one frame
      - Calls `device.read_pixels()`
      - Returns the raw `ImageBuffer`
   f. Call `Image::create(buffer)` to get a processed `Image` (rows flipped, validated).
   g. Call `image.save(output_path)` to write the PNG.
   h. Print `"Captured: <path>\n"` to stdout.
   i. Exit with code 0.
5. **Error handling**: Any error from `read_pixels()`, `Image::create()`, or `Image::save()` is printed to stderr with the error message and the process exits with a non-zero exit code.
6. **Default output path**: When `[output_path]` is not provided, construct:
   `/tmp/buddd_capture_<scenario>_<timestamp>.png` where `<timestamp>` is the current Unix timestamp (seconds since epoch) using `std::time` or equivalent.

#### Usage text

```
Usage: buddd capture <scenario> [output_path]

Available scenarios:
  cube    Capture a single frame of the rotating cube demo

Scenario names are case-sensitive.
```

#### Observability output

| Condition | Output | Destination |
|---|---|---|
| Capture started | `"Capturing: cube\n"` | stderr |
| Capture succeeded | `"Captured: /tmp/buddd_capture_cube_1234567890.png\n"` | stdout |
| Capture failed (error) | Error message via `be::to_string(error)` | stderr |
| Unknown scenario | `"Unknown capture scenario: '<name>'\n\n"` + usage text | stderr |
| No scenario provided | Usage text | stderr |
| Extra args warning | `"Warning: unexpected arguments after 'capture cube': ..."` | stderr |

### Capture scenario: "cube"

- **File**: `src/cmd/capture/cube_capture.h` and `src/cmd/capture/cube_capture.cpp`.
- **Namespace**: `buddd::cmd::capture`.

#### Function signature

```cpp
namespace buddd::cmd::capture {

/// Captures a single frame of the rotating cube scene.
/// Sets up the cube via setup_cube(), renders one frame with
/// the camera at position (0, 0, 3), then reads back the framebuffer.
///
/// @param platform  The engine platform (for event polling — not strictly needed
///                  for single-frame capture but passed for consistency).
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

#### Implementation behavior

1. Calls `setup_cube(device)` to create `CubeResources` (material + model).
2. Sets up a camera:
   - Position: `(0.0f, 0.0f, 3.0f)` — looking along -Z at the origin.
   - Looking at: `(0.0f, 0.0f, 0.0f)`.
   - Up: `Vec3::unit_y()`.
   - Perspective: 60° FOV, aspect = `window_w / window_h`, near 0.1, far 100.0.
3. Renders exactly **one** frame:
   - Calls `device.begin_frame()`.
   - Computes `u_mvp` with rotation angle = 0 (no rotation for the capture frame) — or a fixed angle such as `0.0f` for a front-facing view.
   - Sets `u_mvp` uniform: `cube.material->set_uniform("u_mvp", mvp)`.
   - Draws: `cube.model.draw(device)`.
   - Calls `device.end_frame()`.
4. Calls `device.read_pixels()` to read the framebuffer.
5. Returns the `ImageBuffer` (raw, bottom-left origin, not flipped — flipping is `Image::create`'s job).

**Camera position note**: The cube demo uses camera position `(3, 2, 3)` for an isometric-like view. The capture scenario uses `(0, 0, 3)` for a straight-on front view of the cube. This is intentional — the capture scenario is not a demo, it produces a reference image.

**Rotation**: The capture "cube" scenario captures a static frame with no rotation (angle = 0). The cube is rendered axis-aligned, showing the front face (+Z, cyan) facing the camera. This produces a deterministic, reproducible capture.

#### File conventions

- `capture_cube_scene()` is a free function in `buddd::cmd::capture` namespace.
- Implementation includes:
  - `"demo/demo_helpers.h"` for `setup_cube()`, `CubeResources`.
  - `"render/render_device.h"` for `RenderDevice`.
  - `"platform/platform.h"` for `Platform`.
  - `"math/camera.h"`, `"math/math.h"`, `"math/mat4.h"`, `"math/vec3.h"` for camera and matrix math.
  - `"image/image_buffer.h"` for the return type.

### Error::Category extension

Add two new categories to `Error::Category` in `src/engine/error.h`:

```cpp
enum class Category {
    // ... existing categories ...
    ReadbackFailed,  // Framebuffer readback (glReadPixels) failure
    IoFailed,        // File I/O error (read/write image file)
};
```

Update `to_string()` to handle both new categories.

`ReadbackFailed` covers GPU readback errors (OpenGL errors from `glReadPixels`).
`IoFailed` covers file I/O errors from `Image::load()` and `Image::save()` (file not found, permission denied, disk full, invalid PNG data, etc.).

### Build system

#### `src/engine/CMakeLists.txt`

Add FetchContent for stb (single-header library, public domain):

```cmake
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 31c1ad37456438565541f4919958214b6e762fb4
)
FetchContent_MakeAvailable(stb)

target_include_directories(buddd_engine PRIVATE ${stb_SOURCE_DIR})
```

The `src/engine/CMakeLists.txt` already uses `file(GLOB_RECURSE ...)` for engine sources, so new files in `src/engine/image/` are picked up automatically on re-configure.

#### `src/engine/image/image.cpp`

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
```

These `#define` lines must appear **before** the `#include` directives. They are placed in `image.cpp` (the only translation unit that needs the implementations).

The public header `image.h` includes `stb_image.h` and `stb_image_write.h` **without** the `STB_IMAGE_IMPLEMENTATION` / `STB_IMAGE_WRITE_IMPLEMENTATION` macros. This ensures that only `image.cpp` contains the implementation, preventing ODR violations if other translation units include `image.h`.

#### `src/cmd/CMakeLists.txt`

No changes needed — the existing glob pattern already includes all `.cpp` files under `src/cmd/commands/` and `src/cmd/demo/`. The new files in `src/cmd/capture/` will need the glob to be extended to also cover `capture/*.cpp`:

```cmake
file(GLOB_RECURSE CMD_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/commands/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/demo/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/capture/*.cpp
)
```

### Boundary rules (CONST-001) verification

| Component | Location | CONST-001 compliance |
|---|---|---|
| `ImageBuffer` | `src/engine/image/image_buffer.h` | Engine layer — fine. |
| `Image` | `src/engine/image/image.h`, `image.cpp` | Engine layer — fine. |
| stb_image / stb_image_write | Engine private include (via FetchContent) | Storage/IO library, not platform/graphics/math — allowed in `src/engine/`. |
| `RenderDevice::read_pixels()` | `src/engine/render/render_device.h` | Engine render abstraction — fine. |
| `RenderDeviceOpenGL::read_pixels()` | `src/engine/render/render_device_opengl.cpp` | Engine render implementation — `glReadPixels` call is inside `src/engine/`, as are all existing GL calls. |
| `RenderDeviceHeadless::read_pixels()` | `src/engine/render/render_device_headless.cpp` | Engine render implementation — fine. |
| `CaptureCommand` | `src/cmd/commands/capture_command.h/.cpp` | CLI layer — uses engine abstractions only, no platform/graphics/math headers. |
| `capture_cube_scene` | `src/cmd/capture/cube_capture.h/.cpp` | CLI layer — uses engine abstractions only. |
| Error::Category::ReadbackFailed | `src/engine/error.h` | Engine error enum — fine. |
| Error::Category::IoFailed | `src/engine/error.h` | Engine error enum — fine. |

### File structure summary

| File | Content |
|---|---|
| `src/engine/image/image_buffer.h` | `ImageBuffer` aggregate struct |
| `src/engine/image/image.h` | `Image` class declaration |
| `src/engine/image/image.cpp` | `Image` implementation (stb_image, stb_image_write, row-flipping, validation) |
| `src/engine/render/render_device.h` | Abstract `RenderDevice` — add `read_pixels()` pure virtual |
| `src/engine/render/render_device_opengl.h/.cpp` | OpenGL backend — implement `read_pixels()` with `glReadPixels` |
| `src/engine/render/render_device_headless.h/.cpp` | Headless backend — implement `read_pixels()` returning error |
| `src/engine/error.h` | Add `ReadbackFailed` to `Error::Category` |
| `src/engine/CMakeLists.txt` | Add FetchContent for stb |
| `src/cmd/commands/capture_command.h` | `CaptureCommand` class declaration |
| `src/cmd/commands/capture_command.cpp` | `CaptureCommand` implementation |
| `src/cmd/capture/cube_capture.h` | `capture_cube_scene()` declaration |
| `src/cmd/capture/cube_capture.cpp` | `capture_cube_scene()` implementation |
| `src/cmd/main.cpp` | Add `"capture"` dispatch branch |
| `src/cmd/commands/help_command.h` | Update `k_usage_text` to include `capture` |
| `src/cmd/CMakeLists.txt` | Add `capture/*.cpp` to glob |

## Key entities

### ImageBuffer

| Field | Type | Description |
|---|---|---|
| `width` | `int` | Image width in pixels (must be > 0) |
| `height` | `int` | Image height in pixels (must be > 0) |
| `channels` | `int` | Number of colour channels (4 for RGBA framebuffer reads) |
| `data` | `std::vector<std::byte>` | Raw pixel data, size = width × height × channels bytes. Bottom-left origin. |

### Image

| Member | Description |
|---|---|
| `create(ImageBuffer)` | Factory: validates, flips rows, returns Image or error |
| `load(path)` | Factory: loads PNG from disk via stb_image |
| `save(path)` | Writes PNG to disk via stb_image_write |
| `width()` / `height()` / `channels()` / `data()` | Accessors |
| Copy semantics | **Deleted** (non-copyable) — copying large pixel buffers is expensive and error-prone |
| Move semantics | **Defaulted** (movable) — enables efficient transfer of pixel ownership |

### Error::Category (new values)

| Value | Description |
|---|---|
| `ReadbackFailed` | `glReadPixels` or equivalent failed during framebuffer readback |
| `IoFailed` | File I/O error (file not found, permission denied, invalid/corrupt image data, etc.) |

## User stories

### Story 1 — Capture a cube frame to a named file (Priority: P1)

As a developer, I want to run `buddd capture cube /tmp/my_cube.png` and get a valid PNG file of the cube scene, so that I can easily export rendered frames.

**Given** the `buddd` binary is compiled with display support

**When** I run `buddd capture cube /tmp/my_cube.png`

**Then** a window opens briefly, the cube renders for one frame, the window closes, and the file `/tmp/my_cube.png` exists and is a valid non-empty PNG of 800×600 pixels. Stdout contains `"Captured: /tmp/my_cube.png\n"`. Exit code is 0.

### Story 2 — Capture a cube frame with default output path (Priority: P1)

As a developer, I want to run `buddd capture cube` without specifying an output path, so that the engine picks a sensible default location.

**Given** the `buddd` binary is compiled with display support

**When** I run `buddd capture cube`

**Then** a file is created at `/tmp/buddd_capture_cube_<timestamp>.png` (where `<timestamp>` is a Unix timestamp). The file is a valid non-empty PNG of 800×600 pixels. Stdout contains `"Captured: /tmp/buddd_capture_cube_<timestamp>.png\n"`. Exit code is 0.

### Story 3 — See usage when no scenario is given (Priority: P1)

As a developer, I want to see a usage message when I run `buddd capture` without a scenario name, so that I know how to use the command.

**Given** the `buddd` binary is compiled

**When** I run `buddd capture`

**Then** stderr contains the capture usage text listing available scenarios, and the process exits with code 1.

### Story 4 — See error for unknown scenario (Priority: P1)

As a developer, I want to see an error when I specify an unknown capture scenario, so that I know what scenarios are available.

**Given** the `buddd` binary is compiled

**When** I run `buddd capture unknown_scenario`

**Then** stderr contains `"Unknown capture scenario: 'unknown_scenario'"` followed by the scenario usage text, and the process exits with code 1.

### Story 5 — Capture fails gracefully in headless mode (Priority: P1)

As a CI maintainer, I want `read_pixels()` to return an error in headless mode, so that CI builds without a display don't crash or produce undefined behaviour.

**Given** a headless `RenderDevice` (no display, no OpenGL context)

**When** I call `device.read_pixels()`

**Then** an error is returned with category `Unsupported` and a descriptive message. No crash, no undefined behaviour.

### Story 6 — Extra arguments produce a warning (Priority: P2)

As a developer, I want to see a warning when I pass unexpected arguments to the capture command, so that I know they are ignored.

**Given** the `buddd` binary is compiled

**When** I run `buddd capture cube extra_arg` (where `extra_arg` is an unexpected positional argument)

**Then** stderr contains a warning message about unexpected arguments, but the capture still proceeds and produces a PNG file. Exit code is 0.

### Story 7 — Image::create flips rows correctly (Priority: P1)

As an engine developer, I want `Image::create(const ImageBuffer&)` to flip the image vertically, so that the saved PNG has top-left origin (standard image convention) rather than bottom-left (OpenGL convention).

**Given** an `ImageBuffer` containing a 2×2 image with known pixel values in bottom-left origin

**When** I call `Image::create(buffer)`

**Then** the resulting `Image` has its rows flipped: the bottom row of the buffer becomes the top row of the image. The pixel data is correctly re-ordered.

### Story 8 — Image::save writes valid PNG (Priority: P1)

As an engine developer, I want `Image::save(path)` to write a valid PNG file that can be opened by standard image viewers.

**Given** a valid `Image` instance with known pixel data

**When** I call `image.save("/tmp/test_output.png")`

**Then** the file `/tmp/test_output.png` exists, is non-empty, and can be decoded by `Image::load()` (round-trip) or by an external PNG reader, producing the same pixel data.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `src/engine/image/image_buffer.h` exists, defining `buddd::engine::ImageBuffer` as an aggregate with `int width`, `int height`, `int channels`, and `std::vector<std::byte> data`. | File compiles; struct is a valid type with the specified fields. |
| AC-002 | `src/engine/image/image.h` and `src/engine/image/image.cpp` exist, defining `buddd::engine::Image` with static `create(const ImageBuffer&) -> Result<Image>`, static `load(std::string_view) -> Result<Image>`, `save(std::string_view) const -> Result<void>`, and accessors `width()`, `height()`, `channels()`, `data()`. | Files compile; class is a valid type with the specified methods. |
| AC-003 | `Image::create(const ImageBuffer&)` validates `width > 0`, `height > 0`, `channels > 0`, and `data.size() == width * height * channels`. Invalid input returns `Error::Category::InvalidArgument`. | Unit test (engine-level): create ImageBuffer with invalid dimensions/channels/data size — each case returns an error. |
| AC-004 | `Image::create(const ImageBuffer&)` flips pixel rows vertically (bottom-left → top-left). The top row of the output image equals the bottom row of the input buffer. | Unit test: construct a small ImageBuffer (e.g., 4×2) with distinct row patterns, call create, verify row order is inverted. |
| AC-005 | `Image::save(path)` writes a valid PNG file that can be loaded back by `Image::load()` producing identical pixel data (round-trip fidelity). The PNG is also decodable by external tools (magic bytes `\x89PNG`). | Unit test: create an Image with known pixel data, save to temp path, load it back via `Image::load()`, compare data() byte-for-byte. Verify magic bytes. |
| AC-006 | `RenderDevice` gains pure virtual `read_pixels() -> Result<ImageBuffer>`. The signature compiles in the abstract class and all concrete backends. | `src/engine/render/render_device.h` compiles; both `RenderDeviceOpenGL` and `RenderDeviceHeadless` implement the method. |
| AC-007 | `RenderDeviceOpenGL::read_pixels()` calls `glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data_ptr)` with `glPixelStorei(GL_PACK_ALIGNMENT, 1)`. Returns `ImageBuffer` with channels=4 and bottom-left origin. | Code review confirms GL calls. Unit test (requires display/OpenGL context): render a cleared frame, call read_pixels(), verify dimensions and channels=4. |
| AC-008 | `RenderDeviceHeadless::read_pixels()` returns an error with category `Unsupported` and a message indicating readback is not available in headless mode. | Unit test (headless): call `read_pixels()` on headless device — error is returned, category is `Unsupported`. |
| AC-009 | `src/cmd/commands/capture_command.h` and `src/cmd/commands/capture_command.cpp` exist, defining `buddd::cmd::CaptureCommand` with `run(int, const char* const*) -> int`. | Files exist and compile. |
| AC-010 | `main.cpp` has a dispatch branch: `if (cmd == "capture") { return bc::CaptureCommand{}.run(argc, argv); }`. | Code review: main.cpp includes `"commands/capture_command.h"` and has the dispatch branch. |
| AC-011 | Running `buddd capture cube /tmp/test_cube.png` opens a window, renders one frame of the cube, writes a valid PNG at `/tmp/test_cube.png`, and exits with code 0. Stdout contains `"Captured: /tmp/test_cube.png\n"`. | Manual/CI verification: file exists, is non-empty, is a valid PNG (identifiable by magic bytes `\x89PNG`). |
| AC-012 | Running `buddd capture cube` (no output path) writes to `/tmp/buddd_capture_cube_<timestamp>.png` and prints the path. Exit code 0. | Manual verification: file created at the default path with a Unix timestamp. |
| AC-013 | Running `buddd capture` with no arguments prints usage to stderr and exits with code 1. | Run `buddd capture`; stderr contains usage text; exit code is 1. |
| AC-014 | Running `buddd capture unknown_scenario` prints `"Unknown capture scenario: 'unknown_scenario'"` to stderr and exits with code 1. | Run command; stderr contains the error; exit code is 1. |
| AC-015 | Running `buddd capture cube extra_arg` prints a warning to stderr about unexpected arguments but still captures and exits 0. | Run command; stderr contains `"Warning: unexpected arguments"`; file is created; exit code is 0. |
| AC-016 | `CaptureCommand` validates the scenario name **before** creating platform resources (fails fast). | Unit test or manual: `buddd capture unknown` errors immediately without creating a window (no delay, no display dependency). |
| AC-017 | The captured PNG is 800×600 pixels (matching the window size). | Verify PNG dimensions using `identify -format "%w %h"` (ImageMagick) or equivalent tool: output is `800 600`. |
| AC-018 | Build succeeds with CMake + Ninja; stb headers are fetched automatically via FetchContent. | `cmake --build --preset debug` succeeds; build log shows FetchContent downloading stb. |
| AC-019 | No SDL3, OpenGL, or GLM headers are included from any file under `src/cmd/`. | Run `grep -rnE '#include.*(SDL3|GL/|glad|glm)' src/cmd/` — zero matches. |
| AC-020 | Existing demos (`buddd demo triangle`, `buddd demo cube`) and commands (`buddd run`, `buddd version`, `buddd help`) continue to work unchanged. | Run each — behavior matches SPEC-007 and SPEC-009 acceptance criteria. |
| AC-021 | `Error::Category` has a new `ReadbackFailed` value. `to_string(Error{Error::Category::ReadbackFailed, 0, "test"})` returns a string containing `"ReadbackFailed"`. | Unit test or compile-time check. |
| AC-022 | `src/cmd/CMakeLists.txt` includes `capture/*.cpp` in its file glob, auto-discovering new capture scenario files. | Code review: `src/cmd/CMakeLists.txt` has `${CMAKE_CURRENT_SOURCE_DIR}/capture/*.cpp` in its `file(GLOB_RECURSE ...)`. |
| AC-023 | `Image::load(path)` returns an error for a non-existent file path. | Unit test: call load with a non-existent path; error is returned with appropriate category. |
| AC-024 | `capture_cube_scene()` reuses `setup_cube()` from `demo_helpers.h` and does not duplicate cube geometry or shader setup. | Code review: cube_capture.cpp includes `"demo/demo_helpers.h"` and calls `setup_cube(device)`. |
| AC-025 | The `help` command output includes `capture` in the list of available commands. | Run `buddd help`; stdout contains a `capture` line. |

## Test implications

### New test files

| File | Content | Display-dependent? |
|---|---|---|
| `tests/image_test.cpp` | Unit tests for `ImageBuffer` validation, `Image::create()` row-flipping, `Image::save()` round-trip, `Image::load()` error cases, `Image` copy/move semantics. | No — stb-based operations are CPU-only. |
| `tests/render_device_test.cpp` (extend existing) | Unit test for `RenderDeviceHeadless::read_pixels()` error return. | No — headless backend has no GPU dependency. |

### Existing test files that need updates

| File | Update |
|---|---|
| `tests/cmd_tests.cpp` | Add test cases for `buddd capture` (no args, unknown scenario, extra args). These tests follow the existing `run_buddd()` pattern from `test_helpers.h`. The `capture cube` scenario cannot be tested in CI (requires display) — only validation-before-resource tests are automated. |
| `tests/help_command.cpp` (if exists) or `tests/cmd_tests.cpp` | Verify `buddd help` output includes `capture` in command list. |

### Tests requiring a display

The following ACs require a display/GPU and cannot run on headless CI:
- AC-007 (read_pixels on OpenGL backend) — unit test, needs display
- AC-011 (buddd capture cube with display) — integration, needs display
- AC-012 (buddd capture cube default path) — integration, needs display
- AC-015 (extra args warning) — integration, needs display
- AC-017 (verify PNG dimensions) — integration, needs display

These should be gated behind a CMake option or Catch2 tag following the project's existing conditional-test pattern.

### Test hierarchy

```
tests/
├── image_test.cpp          // ImageBuffer + Image unit tests (no display)
├── cmd_tests.cpp           // CLI: capture validation, help (no display)
└── ... (existing tests)
```

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A developer can capture a frame of any rendered scene by calling `device.read_pixels()` → `Image::create(buffer)` → `image.save(path)`, using exactly three lines of engine API calls (not counting error handling). | Count lines in the minimal capture code path. |
| SC-002 | The captured PNG is visually identical (pixel-for-pixel) to what was displayed in the window for a simple scene (e.g., cleared framebuffer of a known colour). | Render a solid-colour frame, capture it, verify the PNG's centre pixel matches the clear colour. |
| SC-003 | A new capture scenario can be added by creating one `.h`/`.cpp` pair in `src/cmd/capture/` and adding an `else if` branch in `CaptureCommand::run()`, without modifying any other command file. The glob in CMakeLists.txt picks up the new file automatically. | Create a skeleton `triangle_capture.h/.cpp` in `src/cmd/capture/` with a matching dispatch entry. Build succeeds. |
| SC-004 | All capture-related tests (Image unit tests, headless readback error test) pass on CI without a display or GPU. | `ctest --preset debug` on a headless runner — all tests pass. |

## Edge cases

| Case | Expected behavior |
|---|---|
| `buddd capture` with no scenario | Prints usage to stderr; exits 1. |
| `buddd capture cube` (default output path) | Writes to `/tmp/buddd_capture_cube_<timestamp>.png`; prints path. |
| `buddd capture cube ` (trailing whitespace) | Shell normalises; treated as `buddd capture cube` (no extra args). |
| `buddd capture CUBE` (uppercase scenario) | Case-sensitive comparison fails; `"Unknown capture scenario: 'CUBE'"` error. |
| `buddd capture cube /tmp/nonexistent_dir/out.png` | Directory does not exist; `Image::save()` returns error; message printed to stderr; exits non-zero. |
| `buddd capture cube /dev/null` | `Image::save()` returns error (cannot write PNG to device file); error reported; exits non-zero. |
| `buddd capture cube` on a system with no display (`BUDDD_HAS_DISPLAY=OFF`) | The `CaptureCommand` creates an SDL3 backend which fails at `Platform::create()`; error printed; exits non-zero. |
| `buddd capture cube` on a system with display but no write permissions to `/tmp` | `Image::save()` fails with permission error; error printed; exits non-zero. |
| `Image::create()` with `channels != 4` (e.g., 3) | Validation passes (any positive channels is valid). Data is preserved as-is, no re-interpretation. |
| `Image::create()` with empty data vector (size 0) | Validation fails: `data.size() != width * height * channels` (0 != positive). Returns `InvalidArgument`. |
| `Image::create()` with negative width or height | Validation fails: `width > 0` and `height > 0` checks fail. Returns `InvalidArgument`. |
| `Image::create()` with zero channels | Validation fails: `channels > 0` check fails. Returns `InvalidArgument`. |
| `Image::load()` with a non-PNG file (e.g., `.txt`) | stb_image fails to decode; returns an error with category `IoFailed`. |
| `Image::load()` with a truncated PNG file | stb_image fails to decode; returns an error with category `IoFailed`. |
| `Image::save()` with empty data (0×0 image) | No pixels to write; stb_image_write produces an empty/invalid PNG or returns error. The Image class should prevent this by construction (can't create an Image with zero dimensions). |
| `Image::save()` with a path that is an existing directory (e.g., `/tmp/`) | stb_image_write fails to open the path; returns error with category `IoFailed`. |
| `Image::save()` with an extremely large image (OOM) | stb_image_write fails allocation; returns error with category `IoFailed`. |
| `Image::create()` with extremely large dimensions (OOM) | `std::vector` allocation throws `std::bad_alloc`; caller should be aware of OOM risk. |
| `Image::load()` with an extremely large PNG (OOM) | stb_image fails allocation; returns error with category `IoFailed`. |
| `read_pixels()` called before any rendering | Returns the current framebuffer contents (clear colour if `begin_frame()` has been called, or undefined contents otherwise). No crash. |
| `read_pixels()` called outside `begin_frame()` / `end_frame()` | Undefined behaviour in the OpenGL backend (OpenGL state may not be valid). The headless backend returns an error. |
| `read_pixels()` called multiple times in one frame | Each call reads the current framebuffer state. Legal but may be expensive. |

## Error cases

| Case | Expected behavior |
|---|---|
| `read_pixels()` in headless backend | Returns `make_error(Error::Category::Unsupported, "read_pixels is not supported in headless mode")`. |
| `glReadPixels` fails in OpenGL backend (GL error) | Returns `make_error(Error::Category::ReadbackFailed, "glReadPixels failed: ...")`. |
| `Image::create()` with null/empty ImageBuffer (zero width, height, or channels) | Returns `make_error(Error::Category::InvalidArgument, "ImageBuffer dimensions must be positive")`. |
| `Image::create()` with mismatched data size | Returns `make_error(Error::Category::InvalidArgument, "ImageBuffer data size does not match dimensions")`. |
| `Image::load()` with non-existent path | Returns `make_error(Error::Category::IoFailed, "Failed to load image: ...")` (stb_image returns NULL). |
| `Image::load()` with invalid/corrupt PNG | Returns `make_error(Error::Category::IoFailed, "Failed to load image: ...")`. |
| `Image::save()` with unwritable path | Returns `make_error(Error::Category::IoFailed, "Failed to write image: ...")` (stb_image_write returns 0). |
| `CaptureCommand` — scenario validation fails | Prints error to stderr; returns `EXIT_FAILURE` (before creating any platform resources). |
| `CaptureCommand` — platform creation fails | Prints error to stderr; returns `EXIT_FAILURE`. |
| `CaptureCommand` — window creation fails | Prints error to stderr; returns `EXIT_FAILURE`. |
| `CaptureCommand` — render device creation fails | Prints error to stderr; returns `EXIT_FAILURE`. |
| `CaptureCommand` — `setup_cube()` fails | `setup_cube()` calls `std::exit(EXIT_FAILURE)` on failure (documented in SPEC-009). No error is propagated via `Result`. |
| `CaptureCommand` — read_pixels() fails | Prints error to stderr; returns `EXIT_FAILURE`. |
| `CaptureCommand` — Image::create() fails | Prints error to stderr; returns `EXIT_FAILURE`. |
| `CaptureCommand` — Image::save() fails | Prints error to stderr; returns `EXIT_FAILURE`. |

## Permissions and security

- The CLI binary requires no elevated privileges (root/admin) to run — except when writing to paths that require permissions (e.g., `/tmp/` is typically world-writable).
- No network access is required at runtime.
- No secrets, credentials, or environment variables are consumed.
- The architecture boundary (CONST-001) is preserved: no SDL3, OpenGL, or GLM headers are included from `src/cmd/`. All platform, graphics, and image operations go through engine abstractions.
- stb_image and stb_image_write are public-domain single-header libraries with no external dependencies. They are fetched via CMake FetchContent at build time and statically linked into `buddd_engine`.
- Image loading from disk (`Image::load()`) reads arbitrary PNG files. The caller is responsible for trusting the source of loaded images. stb_image has been widely audited and is considered safe for decoding untrusted PNGs in memory-safe contexts.

## Observability

| Signal | Source |
|---|---|
| Capture started | stderr: `"Capturing: cube\n"` |
| Capture succeeded | stdout: `"Captured: <path>\n"` |
| Capture failed (readback) | stderr: Error message from `read_pixels()` |
| Capture failed (image processing) | stderr: Error message from `Image::create()` |
| Capture failed (file write) | stderr: Error message from `Image::save()` |
| Unknown scenario | stderr: `"Unknown capture scenario: '<name>'"` + usage |
| No scenario provided | stderr: Scenario usage text |
| Extra arguments warning | stderr: `"Warning: unexpected arguments after 'capture <scenario>': ..."` |
| glReadPixels errors (debug builds) | OpenGL errors logged via `glGetError()` check in debug builds |
| Image creation success/failure | stderr: log in debug builds (optional) |
| Exit code | Shell variable `$?` after the process exits |

No additional logging or metrics infrastructure is required beyond what already exists.

## Out of scope

- `image_analyzer` subagent or image comparison/verification tools (future spec).
- Multi-frame capture, video recording, or animation capture.
- Capture scenarios beyond `"cube"` (future specs).
- Modifying existing demos (`triangle`, `cube` demos) or `demo_helpers` beyond what is already specified.
- Headless render-device capture support (returns error by design).
- Verification or validation of captured image correctness (pixel comparison, golden image testing).
- GUI/editor integration for captures.
- Texture pipeline integration (`Image::load()` exists for future use but is not wired into any texture system).
- Colour space conversion, gamma correction, or HDR capture.
- Framebuffer object (FBO) readback — the spec reads from the default framebuffer only.
- GPU-to-CPU readback performance optimisation (pixel buffer objects, async readback).
- Cross-platform image file path handling (POSIX `/tmp/` convention assumed for default paths).
- Image file format support beyond PNG (no JPEG, BMP, TIFF, etc.).

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The `RenderDevice` abstract class already has `size() -> std::pair<int, int>` returning the framebuffer width and height. `read_pixels()` uses this to determine the readback dimensions. |
| A-02 | The `Error::Category` enum is extensible. Adding `ReadbackFailed` and `IoFailed` is additive and does not break existing code, consistent with previous specs (SPEC-005 A-05). |
| A-03 | The existing OpenGL backend stores framebuffer width and height internally (matching the window dimensions). These are accessible via `size()` and can be passed to `glReadPixels`. |
| A-04 | The `demo_helpers.h` / `demo_helpers.cpp` `setup_cube()` function is stable and usable by capture code. The capture scenario does not modify it. |
| A-05 | The camera position `(0, 0, 3)` looking at the origin with 60° FOV and 800×600 aspect ratio shows the entire front face of the unit cube (size 2) with some margin. This is a suitable default for a deterministic capture. |
| A-06 | The default output path `/tmp/buddd_capture_<scenario>_<timestamp>.png` is suitable for Linux/POSIX systems. On other platforms (if added later), the default path may need adjustment. |
| A-07 | The `capture` command creates the platform, window, and render device internally (not reusing any existing state). This is consistent with the `demo` command pattern. |
| A-08 | The capture scenario renders only 1 frame. No render loop, no continuous rendering. This is intentional — the command produces a single snapshot. |
| A-09 | The stb library is fetched via CMake FetchContent from `https://github.com/nothings/stb.git`, `master` branch. The build requires internet access for the initial configure. Offline builds may use a cached CMake fetch. |
| A-10 | The `#define STB_IMAGE_IMPLEMENTATION` and `#define STB_IMAGE_WRITE_IMPLEMENTATION` macros are defined in exactly one translation unit (`image.cpp`) before including the respective headers, per stb's single-header convention. |
| A-11 | The `src/cmd/CMakeLists.txt` glob pattern is extended to include `capture/*.cpp` alongside the existing `commands/*.cpp` and `demo/*.cpp` patterns. This requires a CMake re-configure when new capture files are added. |
| A-12 | The `CaptureCommand` uses the SDL3 backend unconditionally (not the compile-time `BUDDD_HAS_DISPLAY` selection). Capturing requires a display — headless capture is explicitly unsupported. If `BUDDD_HAS_DISPLAY=OFF`, `Platform::create(SDL3)` will fail at runtime, which is an acceptable error path. |
| A-13 | The `buddd capture cube` scenario uses a fixed rotation angle of 0 radians (axis-aligned front view) for deterministic output. This is deliberate for reproducible captures. |
| A-14 | The `Image` class is not thread-safe. Callers must ensure exclusive access to `Image` instances if used from multiple threads. |
| A-15 | `Image::load()` loads PNG files with any channel count (1–4). The loaded image's `channels()` reflects the number of channels in the file. |
| A-16 | The `Image` class stores pixel data as `std::vector<std::byte>` regardless of channel count. No implicit conversion between RGB and RGBA occurs. |

## Open questions (resolved)

| ID | Question | Resolution |
|---|---|---|
| Q-01 | Should `CaptureCommand` attempt SDL3 even on headless builds? | **RESOLVED**: Use SDL3 unconditionally. On headless builds, `Platform::create(SDL3)` fails at runtime with an error message. No compile-time `#ifdef` branch. |
| Q-02 | Should the captured cube be rendered at rotation angle 0 or a slight angle? | **RESOLVED**: Angle = 0 (front view, deterministic). The capture produces a reproducible reference image. |
| Q-03 | Should `Image::load()` / `Image::save()` use `InitFailed` or a new `IoFailed` category? | **RESOLVED**: New `IoFailed` category added to `Error::Category`. More descriptive for file I/O errors. |
| Q-04 | Should the default output path be configurable via environment variable? | **RESOLVED**: No environment variable. The user always passes an explicit path or uses the `/tmp/` default. |
| Q-05 | Should `read_pixels()` be allowed before the first `begin_frame()` call? | **RESOLVED**: Allowed. The caller gets current framebuffer contents. Before any `begin_frame()`, contents are undefined (platform-dependent). This is documented as undefined behaviour if called outside `begin_frame()`/`end_frame()`. |
| Q-06 | Should `Image::save()` overwrite existing files or fail? | **RESOLVED**: Overwrite silently, consistent with standard CLI tools. |
| Q-07 | Should `capture` command print framebuffer dimensions? | **RESOLVED**: Print path only (`"Captured: <path>\n"`), matching the simplicity of other commands. |

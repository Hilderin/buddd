#pragma once

#include "error.h"                // for Result<T>
#include "image/image_buffer.h"   // for ImageBuffer

namespace buddd::engine {
class Platform;
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::capture {

/// Captures a frame of the rotating cube scene.
/// Sets up the cube via setup_cube(), renders `num_frames` frames (with
/// rotation animation if num_frames > 1), then reads back the framebuffer.
///
/// @param platform   The engine platform (for event polling).
/// @param device     The render device (for rendering and readback).
/// @param window_w   Window width in pixels.
/// @param window_h   Window height in pixels.
/// @param num_frames Number of frames to render before capturing (default: 1).
///                   The last frame (frame N) is captured. If > 1, the cube
///                   rotates 0.5 rad/s around Y, matching the cube demo timing.
/// @return An ImageBuffer containing the raw (bottom-left origin) framebuffer
///         contents, or an error.
[[nodiscard]] auto capture_cube_scene(
    buddd::engine::Platform& platform,
    buddd::engine::RenderDevice& device,
    int window_w,
    int window_h,
    int num_frames = 1
) -> buddd::engine::Result<buddd::engine::ImageBuffer>;

} // namespace buddd::cmd::capture

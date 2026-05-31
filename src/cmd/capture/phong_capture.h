#pragma once

#include "error.h"                // for Result<T>
#include "image/image_buffer.h"   // for ImageBuffer

namespace buddd::engine {
class Platform;
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::capture {

/// Captures a frame of the Phong lighting demo scene.
/// Replicates the scene setup from the interactive phong demo:
///   5 cubes with varying Phong material properties
///   5 lights (1 directional, 3 point, 1 spot)
///   Fixed camera at (6, 3.5, 8) looking at origin
///
/// Renders `num_frames` frames (minimum 2 for driver quirk workaround),
/// then reads back the last frame's framebuffer.
/// Point lights A and B orbit continuously (matching the interactive demo),
/// so --frames N produces different light positions for each frame.
///
/// @param platform   The engine platform (for event polling).
/// @param device     The render device (for rendering and readback).
/// @param window_w   Window width in pixels.
/// @param window_h   Window height in pixels.
/// @param num_frames Number of frames to render before capturing (default: 1).
///                   The last frame (frame N) is captured.
/// @return An ImageBuffer containing the raw (bottom-left origin) framebuffer
///         contents, or an error.
[[nodiscard]] auto capture_phong_scene(
    buddd::engine::Platform& platform,
    buddd::engine::RenderDevice& device,
    int window_w,
    int window_h,
    int num_frames = 1
) -> buddd::engine::Result<buddd::engine::ImageBuffer>;

} // namespace buddd::cmd::capture

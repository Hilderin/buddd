#pragma once

namespace buddd::engine {
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::demo {

/// Runs the scene-based cube demo: 120-frame render loop using
/// World + RenderSystem instead of manual camera/MVP/draw calls.
///
/// Platform is accessed via device.window().platform().
/// @param device    The render device (for rendering).
/// @param argc      Argument count (argv[0] is the demo name).
/// @param argv      Argument vector (argv[0] is the demo name).
/// @return 0 on success, non-zero on error.
[[nodiscard]] auto run_cube_scene_demo(buddd::engine::RenderDevice& device,
                                       int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo

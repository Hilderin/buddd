#pragma once

namespace buddd::engine {
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::demo {

/// Runs the free camera demo: interactive fly-through camera using WASD +
/// mouse look + Space/Control vertical movement. Press Escape to exit.
///
/// Platform is accessed via device.window().platform().
/// @param device    The render device (for rendering).
/// @param argc      Argument count (argv[0] is the demo name).
/// @param argv      Argument vector (argv[0] is the demo name).
/// @return 0 on success, non-zero on error.
[[nodiscard]] auto run_free_camera_demo(buddd::engine::RenderDevice& device,
                                        int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo

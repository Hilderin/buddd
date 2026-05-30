#pragma once

namespace buddd::engine {
class Platform;
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::demo {

/// Runs the triangle demo: 120-frame render loop with a coloured triangle.
/// @param platform  The engine platform (for event polling).
/// @param device    The render device (for rendering).
/// @param argc      Argument count (argv[0] is the demo name).
/// @param argv      Argument vector (argv[0] is the demo name).
/// @return          0 on success, non-zero on error.
[[nodiscard]] auto run_triangle_demo(buddd::engine::Platform& platform,
                                     buddd::engine::RenderDevice& device,
                                     int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo

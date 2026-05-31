#pragma once

namespace buddd::engine {
class RenderDevice;
} // namespace buddd::engine

namespace buddd::cmd::demo {

[[nodiscard]] auto run_textured_cube_demo(buddd::engine::RenderDevice& device,
                                          int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo

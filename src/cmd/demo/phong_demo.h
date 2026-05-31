#pragma once

namespace buddd::engine { class RenderDevice; }

namespace buddd::cmd::demo {

[[nodiscard]] auto run_phong_demo(buddd::engine::RenderDevice& device,
                                  int argc, const char* const* argv) -> int;

} // namespace buddd::cmd::demo

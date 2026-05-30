#pragma once

namespace buddd::cmd {

class RunCommand {
public:
    /// Opens an interactive window (1024×768, title "Buddd Engine") and clears
    /// the framebuffer each frame until the user closes the window (no draw calls).
    /// Extra arguments are silently ignored.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd

#pragma once

namespace buddd::cmd {

class TestCommand {
public:
    /// Opens a test window (800×600, title "Buddd Engine — Render Test") and
    /// renders a coloured triangle for 120 frames (~2 seconds at 60 FPS).
    /// If extra arguments follow "test", prints a warning to stderr but
    /// proceeds. If the user closes the window early, prints an abort message
    /// and exits with code 0.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd

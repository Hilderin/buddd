#pragma once

namespace buddd::cmd {

class DemoCommand {
public:
    /// Parses argv[2] as a demo name, creates a platform/window/device (800×600,
    /// title "Buddd Engine — Demo: <name>"), and dispatches to the matching
    /// per-demo function. If no name is given, prints usage to stderr and exits 1.
    /// If the name is unknown, prints an error + usage to stderr and exits 1.
    /// Extra arguments after the demo name produce a warning to stderr.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd

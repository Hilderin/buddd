#pragma once

namespace buddd::cmd {

class VersionCommand {
public:
    /// Prints the engine version string and exits with code 0.
    /// Extra arguments are silently ignored.
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd

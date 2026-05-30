#pragma once

namespace buddd::cmd {

class CaptureCommand {
public:
    /// Parses argv[2] as a scenario name, validates it BEFORE creating resources
    /// (fails fast), then creates a platform/window/device (800×600, SDL3 backend
    /// unconditionally), runs the scenario to capture one frame, writes a PNG file,
    /// and exits.
    ///
    /// Signature: buddd capture <scenario> [output_path]
    [[nodiscard]] auto run(int argc, const char* const* argv) -> int;
};

} // namespace buddd::cmd

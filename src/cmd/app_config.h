#pragma once

#include "error.h"

#include <string>
#include <vector>

namespace buddd::cmd {

struct CaptureSpec {
    int frame;          // 1-based
    std::string path;

    /// Returns the effective 1-based frame number after applying the OpenGL
    /// driver quirk. The minimum effective frame is 2 (frame 1 on some OpenGL
    /// drivers returns the clear colour, so it is silently bumped to frame 2).
    [[nodiscard]] int effective_frame() const {
        return (frame < 2) ? 2 : frame;
    }
};

struct RunningArgs {
    int frame_limit = 0;
    std::vector<CaptureSpec> captures;
};

/// Parse --frame N and --capture N:path from argv starting at index `start`.
///
/// - `--frame N`: sets RunningArgs::frame_limit. N must be >= 0 (0 = interactive/no limit).
/// - `--capture N:path`: adds CaptureSpec{N, path}. N must be >= 1.
/// - Unknown flags are silently ignored.
///
/// Returns RunningArgs on success, or an error on invalid input.
[[nodiscard]] auto parse_running_args(int argc, char* argv[], int start)
    -> engine::Result<RunningArgs>;

} // namespace buddd::cmd

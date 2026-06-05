#pragma once

#include "error.h"

#include <string>
#include <vector>

namespace buddd::cmd {

struct CaptureSpec {
    int frame;          // 1-based
    std::string path;
};

struct RunningArgs {
    int frame_limit = 0;
    std::vector<CaptureSpec> captures;
};

/// Parse --frame N and --capture N:path from argv starting at index `start`.
///
/// - `--frame N`: sets RunningArgs::frame_limit. N must be >= 1.
/// - `--capture N:path`: adds CaptureSpec{N, path}. N must be >= 1.
/// - Unknown flags are silently ignored.
///
/// Returns RunningArgs on success, or an error on invalid input.
[[nodiscard]] auto parse_running_args(int argc, char* argv[], int start)
    -> engine::Result<RunningArgs>;

} // namespace buddd::cmd

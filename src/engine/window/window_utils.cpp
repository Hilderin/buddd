#include "window_utils.h"

namespace buddd::engine {

auto window_state_to_string(WindowState state) -> std::string {
    switch (state) {
        case WindowState::Normal:    return "normal";
        case WindowState::Maximized: return "maximized";
        case WindowState::Minimized: return "minimized";
    }
    return "normal";
}

auto parse_window_state(const std::string& str) -> WindowState {
    if (str == "normal")    return WindowState::Normal;
    if (str == "maximized") return WindowState::Maximized;
    if (str == "minimized") return WindowState::Minimized;
    return WindowState::Normal;  // unknown → fallback
}

} // namespace buddd::engine

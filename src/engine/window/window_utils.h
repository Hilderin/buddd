#pragma once

#include "window.h"

#include <string>

namespace buddd::engine {

/// Convert WindowState to its string representation.
/// Normal → "normal", Maximized → "maximized", Minimized → "minimized".
auto window_state_to_string(WindowState state) -> std::string;

/// Parse a string to WindowState.
/// "normal" → Normal, "maximized" → Maximized, "minimized" → Minimized.
/// Any other string → Normal (fallback).
auto parse_window_state(const std::string& str) -> WindowState;

} // namespace buddd::engine

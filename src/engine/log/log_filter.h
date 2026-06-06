#pragma once

#include "log.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace buddd::log {

/// Internal filter class that determines whether a (level, tag) pair is enabled.
/// Not thread-safe — owned by Logger and only modified during init/reset.
class LogFilter {
public:
    /// Set the global minimum level. Messages below this level are dropped unless
    /// a tag override raises the effective level.
    void set_global_level(LogLevel level) { global_level_ = level; }

    /// Replace all tag overrides.
    void set_tag_overrides(const std::vector<std::pair<std::string, LogLevel>>& overrides) {
        tag_overrides_ = overrides;
    }

    /// Returns the global minimum level.
    [[nodiscard]] auto global_level() const -> LogLevel { return global_level_; }

    /// Returns true if a message at `level` with `tag` should be dispatched.
    /// Performs prefix matching on tag overrides (last match wins).
    /// Lock-free — only modified during init/reset.
    [[nodiscard]] auto is_enabled(LogLevel level, std::string_view tag) const -> bool {
        if (tag_overrides_.empty()) {
            return level >= global_level_;
        }

        // Check tag overrides in reverse order (last match wins)
        LogLevel effective = global_level_;
        for (auto it = tag_overrides_.rbegin(); it != tag_overrides_.rend(); ++it) {
            const auto& [pattern, override_level] = *it;
            if (tag.substr(0, pattern.length()) == pattern) {
                effective = override_level;
                break;
            }
        }
        return level >= effective;
    }

private:
    LogLevel global_level_ = LogLevel::Debug;
    std::vector<std::pair<std::string, LogLevel>> tag_overrides_;
};

} // namespace buddd::log

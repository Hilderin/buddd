#pragma once

#include <cstdio>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace buddd::log {

// ---------------------------------------------------------------------------
// LogLevel enum — ordered from most verbose (0) to least verbose (4)
// ---------------------------------------------------------------------------
enum class LogLevel {
    Trace,   // 0 — off by default in debug builds, only via --log-level=trace
    Debug,   // 1 — default minimum in debug builds (NDEBUG not defined)
    Info,    // 2
    Warn,    // 3 — default minimum in release builds (NDEBUG defined)
    Error    // 4
};

// ---------------------------------------------------------------------------
// LogMessage — structured log data passed to Sink::write()
// ---------------------------------------------------------------------------
struct LogMessage {
    LogLevel        level;
    std::string_view tag;       // source tag (e.g. "Asset:ModelLoader")
    std::string     message;    // Owned string (not string_view) because MemorySink stores LogMessage
                                // objects that outlive the caller's scope. A string_view would dangle.
    std::string_view file;      // __FILE__ value
    int             line;       // __LINE__ value
    std::string_view function;  // __FUNCTION__ value
};

// ---------------------------------------------------------------------------
// Sink — abstract interface for log output destinations
// ---------------------------------------------------------------------------
class Sink {
public:
    virtual ~Sink() = default;
    virtual void write(const LogMessage& message) = 0;
};

// ---------------------------------------------------------------------------
// LogConfig — passed to Logger::init()
// ---------------------------------------------------------------------------
struct LogConfig {
    std::vector<std::shared_ptr<Sink>> sinks;
    LogLevel global_min_level =
#ifdef NDEBUG
        LogLevel::Warn;
#else
        LogLevel::Debug;
#endif
    std::vector<std::pair<std::string, LogLevel>> tag_overrides;
};

// ---------------------------------------------------------------------------
// LogFilter (forward declaration only — full declaration in log_filter.h)
// ---------------------------------------------------------------------------
class LogFilter;

// ---------------------------------------------------------------------------
// Logger singleton
// ---------------------------------------------------------------------------
class Logger {
public:
    /// Must be called once from single-threaded startup before any log macro.
    /// Idempotent — second call is a no-op (call reset() first to reconfigure).
    static void init(LogConfig config);

    /// Flushes all sinks and releases resources. Not thread-safe.
    static void shutdown();

    /// Clears all sinks, filter state — enables clean test isolation.
    /// Call before init() when reconfiguring (e.g., between unit tests).
    static void reset();

    /// Returns the singleton reference. Thread-safe (C++11 magic static).
    static auto instance() -> Logger&;

    /// Core log method — formats message, checks filter, dispatches to sinks.
    /// Thread-safe via internal mutex.
    template <typename... Args>
    void log(LogLevel level, std::string_view tag,
             std::string_view file, int line, std::string_view function,
             std::string_view fmt, Args&&... args)
    {
        if (!is_enabled(level, tag)) return; // short-circuit (redundant if caller already checked, but safe)

        // Tag truncation (> 255 chars)
        if (tag.length() > 255) {
            std::fprintf(stderr, "[WARN] [Log] Tag truncated from %zu to 255 chars: '%s'\n",
                         tag.length(), std::string(tag.substr(0, 255)).c_str());
            tag = tag.substr(0, 255);
        }

        std::string message = std::vformat(fmt, std::make_format_args(args...));

        // Message truncation (> 32 KB)
        if (message.size() > 32 * 1024) {
            message.resize(32 * 1024);
        }

        LogMessage msg{
            .level    = level,
            .tag      = tag,
            .message  = std::move(message),
            .file     = file,
            .line     = line,
            .function = function
        };
        write_to_sinks(msg);
    }

    /// Fast level + tag check without formatting.
    /// Used by macros to short-circuit before argument evaluation.
    [[nodiscard]] auto is_enabled(LogLevel level, std::string_view tag) const -> bool;

    Logger(const Logger&) = delete;
    auto operator=(const Logger&) -> Logger& = delete;

private:
    Logger() = default;
    void write_to_sinks(const LogMessage& msg);

    // Owned via unique_ptr for pimpl (hide mutex from header)
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ConsoleSink, FileSink, and MemorySink are declared in their own headers:
//   - log/console_sink.h
//   - log/file_sink.h
//   - log/memory_sink.h

// ---------------------------------------------------------------------------
// Macros
// ---------------------------------------------------------------------------

/// Must appear at file scope (outside any namespace) in every .cpp that uses log macros.
/// Defines BUDDD_CURRENT_LOG_TAG as a static constexpr string_view.
/// Compile error if a log macro is used without declaring this first.
#define BUDDD_LOG_TAG(tag) \
    static constexpr std::string_view BUDDD_CURRENT_LOG_TAG = tag

// Internal implementation macro — not for direct use.
#define BUDDD_LOG_INTERNAL(level, tag_, fmt, ...)                         \
    do {                                                                   \
        auto& _buddd_logger = ::buddd::log::Logger::instance();           \
        if (_buddd_logger.is_enabled((level), (tag_))) {                  \
            _buddd_logger.log(                                             \
                (level), (tag_),                                           \
                __FILE__, __LINE__, __FUNCTION__,                          \
                (fmt) __VA_OPT__(,) __VA_ARGS__                           \
            );                                                             \
        }                                                                  \
    } while (false)

// Standard macros (use BUDDD_CURRENT_LOG_TAG from BUDDD_LOG_TAG)
#define BUDDD_LOG_TRACE(fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Trace, BUDDD_CURRENT_LOG_TAG, fmt __VA_OPT__(,) __VA_ARGS__)
#define BUDDD_LOG_DEBUG(fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Debug, BUDDD_CURRENT_LOG_TAG, fmt __VA_OPT__(,) __VA_ARGS__)
#define BUDDD_LOG_INFO(fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Info, BUDDD_CURRENT_LOG_TAG, fmt __VA_OPT__(,) __VA_ARGS__)
#define BUDDD_LOG_WARN(fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Warn, BUDDD_CURRENT_LOG_TAG, fmt __VA_OPT__(,) __VA_ARGS__)
#define BUDDD_LOG_ERROR(fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Error, BUDDD_CURRENT_LOG_TAG, fmt __VA_OPT__(,) __VA_ARGS__)

// Tagged macros (explicit tag override for a single call)
#define BUDDD_LOG_TAGGED_TRACE(tag_, fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Trace, tag_, fmt __VA_OPT__(,) __VA_ARGS__)
#define BUDDD_LOG_TAGGED_DEBUG(tag_, fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Debug, tag_, fmt __VA_OPT__(,) __VA_ARGS__)
#define BUDDD_LOG_TAGGED_INFO(tag_, fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Info, tag_, fmt __VA_OPT__(,) __VA_ARGS__)
#define BUDDD_LOG_TAGGED_WARN(tag_, fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Warn, tag_, fmt __VA_OPT__(,) __VA_ARGS__)
#define BUDDD_LOG_TAGGED_ERROR(tag_, fmt, ...) \
    BUDDD_LOG_INTERNAL(::buddd::log::LogLevel::Error, tag_, fmt __VA_OPT__(,) __VA_ARGS__)

} // namespace buddd::log

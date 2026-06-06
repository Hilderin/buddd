# IMPL-021 — Logging System

## Source spec

`.specs/sprint-2026-06/logging-system/spec.md`

## Goal

Implement a lightweight self-contained logging framework for the Buddd Engine with five log levels (trace/debug/info/warn/error), a macro-based API that automatically captures source location (`__FILE__`/`__LINE__/__FUNCTION__`), mandatory per-file source tag declaration (`BUDDD_LOG_TAG`), runtime level/filter control via CLI flags (`--log-level`, `--log-file`, `--log-filter`), console (stderr) and file sinks, a test-only MemorySink, `std::format`-style formatting, thread safety via mutex, and zero external dependencies beyond C++26 standard library. The CLI flag parsing infrastructure in `src/cmd/` is extended to produce a `LogConfig` used to initialise the `Logger` singleton at startup.

## Non-goals

- Do NOT migrate existing `std::cerr`/`printf`/`fprintf` calls to the new logger — deferred to a future feature.
- Do NOT add any color/ANSI output to the console sink.
- Do NOT add ImGui log sink.
- Do NOT add network/remote/JSON logging.
- Do NOT add async/non-blocking logging — all logging is synchronous and mutex-guarded.
- Do NOT add log file rotation or management beyond simple append.
- Do NOT add automatic logging of `Error`/`Result<T>` types — the logger is fully decoupled from `buddd::engine::Error` and `buddd::engine::Result<T>`.
- Do NOT introduce any new external dependencies (no fmt, no spdlog, etc.).
- Do NOT change the architecture boundaries (ADR-019).

## Relevant ADRs

| ADR | Constraint |
|---|---|
| ADR-001 | The logger must NOT depend on `buddd::engine::Error`/`Result<T>` (decoupling requirement in spec non-goals). CLI parsing functions in `app_config.h/.cpp` already use `engine::Result<T>` — this is acceptable since the CLI code is not part of the logger itself. The logger code (`src/engine/log/`) must NOT include `error.h`. |
| ADR-009 | Test files must use the plural `_tests.cpp` suffix. Test file: `logging_tests.cpp`. |
| ADR-014 | CLI infra lives in `src/cmd/`; `--log-level`, `--log-file`, `--log-filter` flags must be parsed by infrastructure in `src/cmd/` and produce a `LogConfig` passed to `Logger::init()`. |
| ADR-019 | No code outside `src/engine/` may include platform/graphics/windowing headers. The logger is entirely standard-library-only and architecture-boundary compliant by nature. |

## Files to inspect

| File | Why |
|---|---|
| `src/engine/error.h` | Confirm the logger does NOT depend on this file (decoupling requirement). |
| `src/engine/CMakeLists.txt` | Confirm `file(GLOB_RECURSE)` picks up new files under `src/engine/log/` automatically. |
| `src/cmd/app_config.h` | Understand existing CLI flag parsing pattern (`parse_running_args`, `RunningArgs`, `engine::Result<T>` return). |
| `src/cmd/app_config.cpp` | Understand arg-iteration style, error reporting, and flag-value consumption pattern. |
| `src/cmd/main.cpp` | Understand where to call `Logger::init()` and parse log flags. |
| `tests/CMakeLists.txt` | Confirm GLOB pattern `*_tests.cpp` picks up `logging_tests.cpp`; test helpers in `tests/` are hand-included, not globbed. |
| `tests/test_helpers.h` | Pattern for shared test helpers — `log_helpers.h` follows the same approach. |
| `docs/wiki/architecture/module-map.md` | Understand the existing module structure, naming conventions, and include patterns. |

## Files allowed to change

### New files

| Path | Purpose |
|---|---|
| `src/engine/log/log.h` | **Public header.** `LogLevel` enum, `LogMessage` struct, `Sink` interface, `LogConfig` struct, `BUDDD_LOG_TAG` macro, all `BUDDD_LOG_*` macros, all `BUDDD_LOG_TAGGED_*` macros, forward declaration of `Logger`. This is the sole public entry point — users `#include "log/log.h"`. |
| `src/engine/log/log_filter.h` | **Internal header.** `LogFilter` class — holds global minimum level and per-tag prefix overrides, provides `is_enabled(level, tag) -> bool`. |
| `src/engine/log/log_filter.cpp` | `LogFilter` implementation — tag override matching, prefix-based lookup. |
| `src/engine/log/logger.h` | **Internal header.** `Logger` singleton class declaration — `init(LogConfig)`, `shutdown()`, `reset()`, `instance()`, `log(...)`, `is_enabled(level, tag)`. |
| `src/engine/log/logger.cpp` | `Logger` implementation — singleton instance, mutex-guarded dispatch, format + sink iteration, `init()`/`shutdown()`/`reset()` lifecycle. |
| `src/engine/log/console_sink.h` | `ConsoleSink` class declaration (inherits `Sink`). |
| `src/engine/log/console_sink.cpp` | `ConsoleSink` implementation — writes `[LEVEL] [Tag] message\n` to stderr. |
| `src/engine/log/file_sink.h` | `FileSink` class declaration (inherits `Sink`). |
| `src/engine/log/file_sink.cpp` | `FileSink` implementation — opens file in append mode, writes `YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message\n`. On open failure, emits raw `write(2, ...)` warning to stderr and does NOT create the sink. |
| `src/engine/log/memory_sink.h` | `MemorySink` header-only class (inherits `Sink`). Conditionally compiled: entire content guarded by `#ifdef BUDDD_TESTING`. Stores messages in `std::vector<LogMessage>` with `messages() -> const std::vector<LogMessage>&` accessor. `clear()` method for test reset. |
| `tests/log_helpers.h` | `ScopedMemoryLogger` RAII helper struct in `buddd::test` namespace. Constructor creates `MemorySink`, builds `LogConfig`, calls `Logger::init()`. Destructor calls `Logger::reset()`. Exposes `sink` member for test assertions. |
| `tests/logging_tests.cpp` | Catch2 test file exercising all 20 acceptance criteria from the spec. Tagged `[logging]`. |

### Modified files

| Path | Change |
|---|---|
| `src/cmd/app_config.h` | Add `LogConfigFlags` struct (or extend `RunningArgs` — see decisions below). Add `parse_logging_args(argc, argv, start) -> LogConfig` function declaration. |
| `src/cmd/app_config.cpp` | Implement `parse_logging_args()` — handle `--log-level=<level>`, `--log-file=<path>`, `--log-filter=<pattern>=<level>`. Validate level strings. Invalid → return an `engine::Result<LogConfig>` error (existing pattern). |
| `src/cmd/main.cpp` | At the very top of `main()`, before any other logic, call `parse_logging_args()` from the available argv, and call `Logger::init(config)` on success. On parse failure, print error to stderr and return `EXIT_FAILURE`. |

### Runtime impact only (no file change)

- `src/engine/CMakeLists.txt` — No explicit change needed; the existing `file(GLOB_RECURSE)` already covers `src/engine/log/*.{h,cpp}`.
- `tests/CMakeLists.txt` — No explicit change needed; the existing `file(GLOB_RECURSE)` already covers `*_tests.cpp`. The `log_helpers.h` file is included directly by `logging_tests.cpp`.

## Files forbidden to change

- Any file under `src/engine/` outside `src/engine/log/` — no modifications to existing engine source files (the logger is additive only).
- `src/engine/error.h` — logger must NOT depend on this file.
- `src/cmd/commands/*` — no command file changes.
- `src/cmd/apps/*` — no app subclass changes.
- `tests/test_helpers.h` — shared test helpers remain unchanged.
- Any file under `docs/` — documentation updates are handled by the wiki-agent, not here.
- Any `.specs/` file outside this feature directory.
- `src/editor/` — no changes.
- `CMakeLists.txt` (root) — no changes needed.

## Existing conventions to follow

| Convention | Source | Application |
|---|---|---|
| `#pragma once` header guard | All existing headers | Use in all new `.h` files. |
| `snake_case` for source files and directories | `docs/wiki/architecture/module-map.md` | `log/log.h`, `log/console_sink.h`, `logging_tests.cpp` |
| PascalCase for classes | `business-rules.md` | `ConsoleSink`, `FileSink`, `MemorySink`, `LogFilter`, `LogConfig`, `LogMessage`, `Logger` |
| No `I` prefix for abstract interfaces | `business-rules.md` | `Sink`, not `ISink` |
| `auto` return type with trailing return type | Existing engine code | `auto foo() -> void` |
| Namespace `buddd::log` for logger types | Spec | All logger code in `src/engine/log/` |
| Namespace `buddd::test` for test helpers | Spec | `ScopedMemoryLogger` in `tests/log_helpers.h` |
| Test files: plural `_tests.cpp` suffix | ADR-009 | `logging_tests.cpp` |
| Test helper files: `_helpers.h` suffix | Existing pattern (`test_helpers.h`) | `log_helpers.h` |
| `std::string_view` for string parameters in public API | Project style | `LogMessage::tag`, `LogMessage::file`, `LogMessage::function` as `std::string_view` |
| File sink opens in append mode | Spec non-goal: "no log rotation beyond simple append" | `FileSink` opens with `std::ios::app` |
| `BUDDD_TESTING` guard for test-only code | Existing CMake definition (`target_compile_definitions(buddd_engine PRIVATE BUDDD_TESTING)`) | `MemorySink` only compiled when `BUDDD_TESTING` is defined |
| `NDEBUG` for build-type detection | Spec | Default global level: `LogLevel::Debug` when `NDEBUG` not defined, `LogLevel::Warn` when `NDEBUG` defined |
| C++26 `std::format` for message formatting | Spec | Use `std::vformat` in `Logger::log()` |
| CMake GLOB_RECURSE for auto-discovery | `src/engine/CMakeLists.txt` | New files under `src/engine/log/` are auto-discovered |
| `.clang-format` (LLVM style, 4-space indent, 100 columns) | `business-rules.md` | All new files must be formatted |

## Required implementation behavior

### 1. Public API (exact signatures)

**`src/engine/log/log.h`** — the sole public header:

```cpp
#pragma once

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
        std::string message = std::vformat(fmt, std::make_format_args(args...));
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

// ---------------------------------------------------------------------------
// ConsoleSink — writes to stderr, format: [LEVEL] [Tag] message\n
// ---------------------------------------------------------------------------
class ConsoleSink : public Sink {
public:
    void write(const LogMessage& message) override;
};

// ---------------------------------------------------------------------------
// FileSink — writes to file, format: YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message\n
// ---------------------------------------------------------------------------
class FileSink : public Sink {
public:
    /// Attempts to open file_path in append mode.
    /// On success returns a unique_ptr to the FileSink.
    /// On failure writes a raw warning to stderr via write(2) and returns nullptr.
    [[nodiscard]] static auto create(std::string_view file_path) -> std::unique_ptr<FileSink>;
    void write(const LogMessage& message) override;
private:
    explicit FileSink(std::string_view file_path); // private — constructed by create()
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ---------------------------------------------------------------------------
// MemorySink — header-only, BUDDD_TESTING only
// ---------------------------------------------------------------------------
#ifdef BUDDD_TESTING
class MemorySink : public Sink {
public:
    void write(const LogMessage& message) override { messages_.push_back(message); }
    [[nodiscard]] auto messages() const -> const std::vector<LogMessage>& { return messages_; }
    void clear() { messages_.clear(); }
private:
    std::vector<LogMessage> messages_;
};
#endif

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
```

### 2. Logger internals

**`Logger::Impl`** (in `src/engine/log/logger.cpp`):
```cpp
struct Logger::Impl {
    std::mutex mutex;
    LogFilter filter;
    std::vector<std::shared_ptr<Sink>> sinks;
    bool initialized = false;
    bool shutdown = false;
};
```

**`Logger::init(LogConfig config)`**:
- If `impl_ && impl_->initialized` → return (no-op, idempotent).
- Create new `Impl`. Set `filter.set_global_level(config.global_min_level)` and `filter.set_tag_overrides(config.tag_overrides)`.
- Move `config.sinks` into `impl_->sinks`.
- Set `impl_->initialized = true`, `impl_->shutdown = false`.
- **Thread safety**: This function is explicitly NOT thread-safe (must be called from single-threaded startup).

**`Logger::shutdown()`**:
- If `!impl_` or `!impl_->initialized` → return.
- Lock mutex, clear sinks, set `impl_->shutdown = true`, `impl_->initialized = false`.
- Not thread-safe.

**`Logger::reset()`**:
- Destroy `impl_` (reset the `unique_ptr<Impl>`), completely clearing all state.
- After reset, `instance().is_enabled()` returns `false` and `log()` drops messages silently.
- Not thread-safe.

**`Logger::write_to_sinks(const LogMessage& msg)`**:
- Lock `impl_->mutex`.
- If `impl_->shutdown` → return immediately (silent drop).
- Iterate `impl_->sinks`, call `sink->write(msg)` for each.

**`Logger::log(...)`**:
- Template inline in header (for `std::make_format_args` which requires variadic template at point of call).
- Checks `is_enabled()` first, formats if enabled, calls `write_to_sinks()`.
- Thread-safe (mutex inside `write_to_sinks`).

**`Logger::is_enabled(LogLevel level, std::string_view tag)`**:
- If `!impl_` or `!impl_->initialized` → return false (silent drop before init).
- If `impl_->shutdown` → return false.
- Returns `impl_->filter.is_enabled(level, tag)`.
- This method is intentionally NOT mutex-protected for performance (the filter is only modified during `init()`/`reset()` which are not concurrent with log calls).

### 3. LogFilter behavior

**`LogFilter::is_enabled(LogLevel level, std::string_view tag)`**:
- If `tag_overrides_` is empty: return `level >= global_level_` (fast path).
- Otherwise, check tag overrides in reverse order (last match wins):
  - For each `(pattern, override_level)`: if `tag` starts with `pattern` characters, set `effective = override_level`.
- If an override matched: return `level >= effective`.
- Otherwise: return `level >= global_level_`.

**Prefix matching**: A pattern `"Asset"` matches tag `"Asset"` and `"Asset:ModelLoader"` (starts-with). A pattern `"Asset:"` matches `"Asset:ModelLoader"` but NOT `"Asset"` (because `"Asset"` does not start with `"Asset:"`).

### 4. ConsoleSink output format

`[LEVEL] [Tag] message\n`

- LEVEL is the uppercase name: `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`.
- Tag is the source tag as-is (e.g., `Asset:ModelLoader`).
- Message is the formatted string as-is.
- No timestamp, no color, no trailing text beyond `\n`.
- Written to stderr via `std::fprintf(stderr, ...)` or `std::print` (C++23) if available — use `std::fprintf` for maximum portability since C++23 `std::print` may not be available in all C++26 implementations.

### 5. FileSink behavior

- **Construction**: `FileSink` is constructed via the static factory `FileSink::create(std::string_view file_path)`, NOT a public constructor.
- **Factory behavior**: Opens `file_path` in append mode (`std::ios::app`).
  - On success: returns `std::unique_ptr<FileSink>`.
  - On failure: writes a raw warning to stderr via `write(2, ...)` (POSIX `write` syscall, NOT the logger — the logger may not be initialised yet) and returns `nullptr`. No exception is thrown.
- **Sink registration**: The caller (e.g., `app_config.cpp` or `ScopedMemoryLogger`) MUST check the return value. Only non-null sinks are added to `LogConfig::sinks`. Nullptr results are silently skipped (the raw stderr warning already informed the user).
- **Logger::init** stores `config.sinks` as-is — all sinks in the vector are already valid (filtering happens at construction site).
- Output format per line: `YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message\n`
  - Timestamp: ISO 8601 basic format, e.g. `2026-06-06T14:30:00`. Use `std::chrono::system_clock::now()` and `std::chrono::to_time_t()` + `std::strftime` for maximum compatibility. Alternatively, use C++20 `std::chrono::zoned_time` / `std::format` with `%F` and `%T` if available (C++20 chrono formatting is available in C++26).
- Flushed after every write (no buffering — every message is immediately written to disk via `std::flush` or equivalent).
- Messages longer than 32 KB are truncated before formatting (the format string itself is not truncated; the formatted result is truncated). Although in practice, `std::format` does the allocation, so truncation happens on the resulting string.
- Tag truncation at 255 characters: if `tag.length() > 255`, log an internal warning via the logger itself (tag `Log`) with the truncated tag, then use the truncated tag.

### 6. MemorySink behavior

- Only compiled when `BUDDD_TESTING` is defined.
- `write()` copies the `LogMessage` (including the owned `message` string) into an internal `std::vector<LogMessage>`.
- `messages()` returns a const reference to the vector.
- `clear()` empties the vector.
- Not thread-safe (test-only, single-threaded test environment).

### 7. ScopedMemoryLogger (tests/log_helpers.h)

```cpp
#pragma once

#include "log/log.h"    // or "src/engine/log/log.h" depending on include path
// ... (catch2 headers if needed)

namespace buddd::test {

struct ScopedMemoryLogger {
    std::shared_ptr<buddd::log::MemorySink> sink;

    ScopedMemoryLogger() {
        sink = std::make_shared<buddd::log::MemorySink>();
        buddd::log::LogConfig config;
        config.sinks.push_back(sink);
        // Set global level to Trace to capture all messages in tests
        config.global_min_level = buddd::log::LogLevel::Trace;
        buddd::log::Logger::init(std::move(config));
    }

    ~ScopedMemoryLogger() {
        buddd::log::Logger::reset();
    }
};

} // namespace buddd::test
```

### 8. CLI flag parsing (src/cmd/app_config.h / .cpp)

Add to `app_config.h`:
```cpp
#include "log/log.h"   // for LogConfig, LogLevel

namespace buddd::cmd {

/// Parse --log-level, --log-file, --log-filter from argv starting at `start`.
/// Returns LogConfig on success, or an error on invalid values.
/// --log-filter format: pattern=level (level is optional, defaults to global_level).
/// Invalid level strings return InvalidArgument error.
[[nodiscard]] auto parse_logging_args(int argc, char* argv[], int start)
    -> engine::Result<buddd::log::LogConfig>;

} // namespace buddd::cmd
```

**Implementation in `app_config.cpp`**:
- Iterate argv from `start` to `argc - 1`.
- `--log-level=<level>`: Match with `std::string_view::substr(0, 11)` for `"--log-level="`. Extract value after `=`. Validate level string: must be exactly one of `trace`, `debug`, `info`, `warn`, `error` (case-sensitive). Set `config.global_min_level`.
- `--log-file=<path>`: Match with `std::string_view::substr(0, 10)` for `"--log-file="`. Extract path after `=`. Call `FileSink::create(path)`. If the returned `unique_ptr<FileSink>` is non-null, push it to `config.sinks`. If null, the factory already emitted a raw stderr warning; no additional error is returned.
- `--log-filter=<pattern>=<level>`: Match with `std::string_view::substr(0, 12)` for `"--log-filter="`. Extract the value after `=`. Split on LAST `=` (to handle colons in tag prefixes). Left side = pattern, right side = level. If no `=` found after the one in `--log-filter=`, level is omitted (use global level — treated as no-op override, filter entry may be stored for informational purposes but `is_enabled` will fall through to global level). Validate level string if present. Add `(pattern, level)` to `config.tag_overrides`. Last occurrence wins for duplicate patterns.
- Unknown flags are silently ignored (existing convention from `parse_running_args`).
- In `main.cpp`, call `parse_logging_args(argc, argv, 1)` before all other logic. On error, print `"Error: <msg>"` to stderr and `return EXIT_FAILURE`. On success, call `Logger::init(config)`.

Level string → LogLevel mapping:
- `"trace"` → `LogLevel::Trace`
- `"debug"` → `LogLevel::Debug`
- `"info"` → `LogLevel::Info`
- `"warn"` → `LogLevel::Warn`
- `"error"` → `LogLevel::Error`
- Anything else → error.

### 9. Thread safety contract

- `Logger::init()`, `Logger::shutdown()`, `Logger::reset()` are NOT thread-safe. They must be called from single-threaded startup/teardown code.
- `Logger::log()` and `Logger::is_enabled()` ARE safe to call from any thread. `log()` locks a mutex during sink iteration; `is_enabled()` performs a lock-free filter check (the filter is const after init).
- `ConsoleSink::write()` and `FileSink::write()` are called under the Logger mutex — they do not add their own locking.
- `MemorySink::write()` is also called under the Logger mutex, making it safe for test use even with multiple threads.

### 10. Decoupling from Error/Result<T>

- No file in `src/engine/log/` shall `#include "error.h"`.
- The `LogConfig` struct uses only standard types and `LogLevel` enum.
- The CLI parsing code in `src/cmd/app_config.cpp` already uses `engine::Result<T>` — this is the CLI layer, NOT the logger. The logger receives a fully-constructed `LogConfig`.

### 11. Logging after shutdown

If `Logger::log()` is called when `impl_->shutdown` is true (checked inside `write_to_sinks`), the message is silently dropped. No crash, no undefined behavior, no allocation.

### 12. Empty format string

If the format string is empty (or null — but `std::string_view` does not support null), `std::vformat("", {})` produces an empty string. This is acceptable. If the entire `LogMessage` has an empty message, sinks output `"[LEVEL] [Tag] \n"` (message is empty).

### 13. Very long tag (> 255 chars)

In `Logger::log()`, before constructing the LogMessage, check `tag.length() > 255`. If so, emit an internal log (tag: `"Log"`, level: Warn) with the truncated tag, and use `tag.substr(0, 255)` for the message. The internal log for truncation must not trigger infinite recursion — guard by a thread-local flag or use a raw stderr write if that's a concern. **Simplest approach**: use a raw `fprintf` to stderr for the truncation warning (it's a rare diagnostic event).

### 14. Very long message (> 32 KB)

After `std::vformat`, if `message.size() > 32 * 1024`, truncate to 32 KB by doing `message.resize(32 * 1024)`. No additional warning (the caller already chose the message content).

## Required tests

### Unit tests (`logging_tests.cpp`)

| # | Test | AC covered | Method |
|---|---|---|---|
| T-01 | LogLevel ordering | AC-001 | `static_assert(LogLevel::Trace < LogLevel::Debug < ... < LogLevel::Error)` |
| T-02 | MemorySink accumulates messages | AC-011 | `ScopedMemoryLogger`, log 3 messages, assert `sink->messages().size() == 3` |
| T-03 | MemorySink message fields match | AC-017 | Log with known file/line/func, assert `LogMessage::file`, `line`, `function` |
| T-04 | Console sink format | AC-003, AC-004 | Use `ScopedMemoryLogger` + manual `ConsoleSink`+stderr redirect, or use a test-specific sink that captures format — since stderr redirect is fragile, instead: create `ConsoleSink`, capture output by temporarily redirecting stderr to an `ostringstream`, then assert format `[LEVEL] [Tag] message` (no timestamp). |
| T-05 | File sink format | AC-005 | Create `FileSink` with temp file, log a message, read file back, assert regex `^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2} \[LEVEL\] \[Tag\] message$` |
| T-06 | Global level filtering | AC-006 | `ScopedMemoryLogger` with `global_min_level = Warn`, log info/warn/error, assert info absent, warn+error present |
| T-07 | Tag override filtering | AC-007 | `ScopedMemoryLogger` with `global_min_level=Info` and `tag_overrides = {{"TestTag", Trace}}`, log trace with `TestTag`, assert present; log trace with `OtherTag`, assert absent. |
| T-08 | File sink enabled via config | AC-008 | `ScopedMemoryLogger` + a temp-file `FileSink`, verify file exists; without `FileSink` in config, verify file does not exist |
| T-09 | File sink failure is non-fatal | AC-009 | Create `FileSink` with invalid path (e.g. `/nonexistent/dir/file.log`), verify no exception, logger continues (can still log to MemorySink). Verify a raw warning appears on stderr (capture stderr during FileSink creation). |
| T-10 | Debug build default threshold | AC-012 | Compile-only check or runtime: `#ifndef NDEBUG` then assert default `LogConfig::global_min_level == Debug`; or test with actual NDEBUG state in a separate compilation unit. **Simplest**: verify the `k_default_log_level` constant has the correct value for the current build type. |
| T-11 | Release build default threshold | AC-013 | Same as T-10 but for `#ifdef NDEBUG`. |
| T-12 | No side effects for disabled levels | AC-014 | Create `ScopedMemoryLogger` with `global_min_level = Error`. `int counter = 0`; `BUDDD_LOG_INFO("{}", ++counter)`. Assert `counter == 0`. |
| T-13 | `BUDDD_LOG_TAGGED_INFO` overrides tag | AC-015 | `ScopedMemoryLogger` with file tag `"FileTag"`; `BUDDD_LOG_TAGGED_INFO("CustomTag", "msg")`. Assert captured message has tag `"CustomTag"`. |
| T-14 | `std::format`-style works | AC-016 | `BUDDD_LOG_INFO("int {} str {}", 42, "hello")`, assert message contains `"int 42 str hello"`. |
| T-15 | No external dependencies | AC-018 | Code review: grep `#include` in `src/engine/log/*.h` and `src/engine/log/*.cpp` — no third-party headers. |
| T-16 | Logger compiled in engine | AC-019 | Build system: verify `src/engine/log/logger.cpp` is part of `buddd_engine` (it's auto-globbed). |
| T-17 | Logger decoupled from Error/Result | AC-020 | Code review: `grep -rn 'error.h\|Error\|Result' src/engine/log/` returns no matches. |
| T-18 | Thread safety stress test | AC-010 | 4 threads, each logging 1000 messages via `BUDDD_LOG_INFO` while main thread reads MemorySink (or use a thread-safe test sink that records messages). After join, verify no interleaved content: each message in MemorySink should contain exactly one level indicator and one tag (the full format string should appear intact). **Note**: The MemorySink is not thread-safe for reads while writes are in progress, so collect messages after threads join. |
| T-19 | Missing BUDDD_LOG_TAG compile error | AC-002 | A separate source file (e.g., `tests/compile_fail/no_tag_test.cpp` or a CMake `try_compile` test) that uses `BUDDD_LOG_INFO` without declaring `BUDDD_LOG_TAG`. Must fail to compile. Use CMake `try_compile` or a separate build target. |
| T-20 | Logger singleton init idempotent | Edge case 6 | Call `Logger::init()` twice with different configs; second call must be no-op (does not overwrite first config). Verify with MemorySink. |
| T-21 | Logging after shutdown is safe | Edge case 11 | `ScopedMemoryLogger` → `Logger::reset()` → log a message → no crash, no UB. |
| T-22 | Logging before init is safe | Edge case "not init" | Before any `Logger::init()`, call `BUDDD_LOG_INFO("test")` → no crash, silent drop. |
| T-23 | Empty format string | Error case | `BUDDD_LOG_INFO("")` → no crash; verify message is empty string. |
| T-24 | File sink append mode | W-07 from spec-critic | Create FileSink with existing temp file containing content, log a message, read file back, verify the original content is preserved (appended, not truncated). |
| T-25 | Empty source tag | W-08 from spec-critic | Declare `BUDDD_LOG_TAG("")`, log a message, verify output contains `[]` (empty brackets). |

### Compile-time test

- A dedicated source file `tests/compile_fail/missing_tag.cpp` containing only:
  ```cpp
  #include "log/log.h"
  // Intentionally no BUDDD_LOG_TAG
  void test() { BUDDD_LOG_INFO("test"); }
  ```
- This file is compiled via CMake `try_compile` or a custom target that expects failure. Add a CMake function in `tests/CMakeLists.txt` (or a new `tests/compile_fail/CMakeLists.txt`) that verifies compilation fails with the expected error (reference to undefined `BUDDD_CURRENT_LOG_TAG`).

### Integration test (CLI flags)

- **New test in `logging_tests.cpp`** or extend `cmd_tests.cpp`:
  - Use `run_buddd()` helper to invoke the buddd binary with `--log-level=error --log-file=/tmp/test.log run triangle --frame 2`.
  - Verify exit code 0.
  - Verify `/tmp/test.log` exists and contains timestamped log entries.
  - Clean up the temp file.
- Acceptable to skip process-level integration if CI restricts filesystem; in that case, test via `ScopedMemoryLogger` + manually constructing `LogConfig` as per Unit Tests.

## Edge cases

(All from spec, carried forward into test requirements above)

1. **File sink path contains spaces**: The `--log-file` value is a raw string; the OS handles quoting. `FileSink` passes the path as-is to `std::ofstream`.
2. **File sink path is a directory**: e.g. `--log-file=/tmp/` — `std::ofstream` fails to open → handled by FileSink failure path (raw stderr warning, no file sink).
3. **Very long source tag (> 255 chars)**: Truncated with `warn`-level internal log (raw stderr).
4. **Empty source tag**: `BUDDD_LOG_TAG("")` compiles; output shows empty `[]` brackets.
5. **Very long log message (> 32 KB)**: Truncated at 32 KB before dispatch to sinks.
6. **Logger::init() called twice**: Idempotent — second call is no-op.
7. **`--log-filter` with non-matching tag**: Stored but never matched — harmless.
8. **`--log-filter` with duplicate pattern**: Last match wins (reverse iteration in `is_enabled`).
9. **`--log-level` invalid level**: CLI parser returns `Result<LogConfig>` error; main.cpp prints error and exits with code 1.
10. **Unicode/UTF-8 in messages**: Pass through as-is.
11. **Logging after shutdown**: Silently dropped (no crash).
12. **Init/shutdown not thread-safe**: Documented; calling concurrently with log operations is UB.
13. **Prefix matching boundary**: `Asset` matches `Asset` and `Asset:ModelLoader`; `Asset:` matches `Asset:ModelLoader` but NOT `Asset`.
14. **Logger not initialized before first log call**: Silently dropped (no crash).
15. **Null or empty format string**: No crash; empty message logged.
16. **Memory exhaustion during formatting**: `std::format` may throw `std::bad_alloc`; allowed to propagate.

## Security impact

- No elevated permissions required.
- File sink writes only to the explicitly-provided path. No automatic path expansion or `~` tilde expansion (shell handles this).
- No logging of sensitive data by the framework itself. Callers are responsible for not logging sensitive information.
- No network access.
- No injection risk: format strings are compile-time literals under macro control; arguments are typed via `std::format`.

## Data and migration impact

None. This feature adds new files and does not modify any existing source code outside the CLI parsing layer and main.cpp entrypoint. There is no schema change, no data migration, and no seed data.

## API compatibility impact

- **Public API addition**: `src/engine/log/log.h` — new public header. Including it in existing translation units is additive (no breakage from non-inclusion).
- **CLI flag addition**: `--log-level`, `--log-file`, `--log-filter` — these are new optional flags. They do not break existing CLI usage.
- **CLI error handling**: Unknown flags are silently ignored (matching existing convention). `--log-level` with an invalid value exits with error (new behavior, but only affects typographical errors).
- **No deprecation**: No existing API is changed or removed.
- **No ABI impact**: The new code is compiled into `buddd_engine` static library. No existing symbol is altered.

## Documentation impact

- **Wiki page**: A new wiki page `docs/wiki/domain/logging.md` must be created (by wiki-agent) documenting API reference, tag naming conventions, macro usage guidelines, and CLI flag reference.
- **Wiki business-rules.md**: Must be updated (by wiki-agent) to document the new CLI flags and their behavior.
- **ADR**: A new ADR (ADR-021 or similar) must be authored (by governance-reviewer or human) documenting the architectural decisions made in this implementation (sink interface, singleton lifecycle, thread safety strategy, tag enforcement mechanism).
- **Spec coordination**: The `.specs/sprint-2026-06/asset-manager/` spec and implementation-contract may reference outdated `std::cerr` conventions; these should be updated in a future migration feature, not now.

## ADR impact

This implementation does not yet warrant a new ADR; the spec-level design already captures the architectural decisions. However, after implementation, the project should consider creating an ADR documenting:
- Why a custom logger was chosen over a third-party library (spdlog, etc.).
- Why `BUDDD_LOG_TAG` uses `static constexpr string_view` (zero-cost, compile-time enforcement).
- Why the singleton pattern with mutex was chosen over async logging.

This is deferred to governance-reviewer or human validation.

## Done criteria

The implementation is complete when all of the following are true:

- [ ] **File existence**: All 11 new files listed under `Files allowed to change > New files` exist at the correct paths.
- [ ] **Build integration**: `src/engine/log/` files are compiled as part of `buddd_engine` (verify via `ninja -t targets` or build log).
- [ ] **CLI integration**: `src/cmd/main.cpp` calls `Logger::init()` at startup; `--log-level`, `--log-file`, `--log-filter` are parsed.
- [ ] **Test file exists**: `tests/logging_tests.cpp` exists and contains at least 20 Catch2 `TEST_CASE` entries.
- [ ] **Test helper exists**: `tests/log_helpers.h` exists with `ScopedMemoryLogger`.
- [ ] **All unit tests pass**: `cmake --build --preset debug && ctest -R logging --output-on-failure` reports 100% pass rate.
- [ ] **Compile-time test**: A separate compile-failure target exists and fails with the expected error when `BUDDD_LOG_TAG` is missing.
- [ ] **MemorySink conditional**: `grep BUDDD_TESTING src/engine/log/memory_sink.h` confirms the `#ifdef` guard.
- [ ] **No Error/Result dependency**: `grep -rn 'error.h\|Error\|Result' src/engine/log/` returns no matches.
- [ ] **No external dependencies**: `grep -rn '^#include' src/engine/log/*.h src/engine/log/*.cpp` confirms only C++26 standard library headers.
- [ ] **FileSink factory**: `src/engine/log/file_sink.h` declares `static auto FileSink::create(std::string_view) -> std::unique_ptr<FileSink>` and has no public constructor (constructor is private).
- [ ] **File sink append mode**: `src/engine/log/file_sink.cpp` uses `std::ios::app`.
- [ ] **Build-type default**: `src/engine/log/log.h` contains `#ifdef NDEBUG` logic for `LogConfig::global_min_level` default.
- [ ] **Tag truncation**: `src/engine/log/logger.cpp` truncates tags > 255 chars with a raw stderr warning.
- [ ] **Message truncation**: `src/engine/log/logger.cpp` truncates formatted messages > 32 KB.
- [ ] **Null safety**: Calling log macros before `Logger::init()` or after `Logger::reset()` produces no crash (messages silently dropped).
- [ ] **Formatter alignment**: All new files have been formatted with `cmake --build --preset debug --target format`.

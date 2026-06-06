# SPEC-021 — Logging System

## Problem

The Buddd Engine currently uses ad-hoc `std::cerr`, `std::printf`, and `std::fprintf` calls scattered across ~25 source files. This approach has several pain points:

- **No level filtering** — All output is mixed together; debug diagnostics and critical errors appear on the same stream.
- **No structured control** — There is no way to enable or disable logging at runtime without editing code.
- **No source attribution** — Messages do not carry a consistent module/sub-component tag, making it hard to identify the origin of a log line.
- **No thread safety** — Concurrent writes from different threads can interleave output.
- **No file sink** — There is no built-in mechanism to capture log output to a file for post-mortem analysis.
- **No testability** — Log output cannot be captured or asserted in unit tests.

These problems make debugging, runtime diagnostics, and development velocity harder than necessary. A structured logging framework is required before the engine grows more complex.

## Goals

- Provide a lightweight, self-contained logging framework with five log levels (`trace`, `debug`, `info`, `warn`, `error`).
- Provide C++ macros as the primary API surface, capturing caller location (`__FILE__`, `__LINE__`, `__FUNCTION__`) automatically.
- Enforce per-file source tag declaration via `BUDDD_LOG_TAG`; missing tags produce a compile error.
- Support `std::format`-style formatting (`{}` placeholders) in all log macros.
- Support runtime level control (global minimum level, per-tag overrides) via CLI flags.
- Support two sinks: console (stderr, always active) and file (optional, enabled via `--log-file`).
- Be thread-safe (mutex-based) from day one.
- Provide a `MemorySink` for unit test assertions.
- Have zero external dependencies beyond the C++26 standard library.
- Decouple the logger from the existing `Error`/`Result<T>` types — no automatic error logging.

## Non-goals

- Migrating existing `std::cerr`/`printf` calls to the new logger. This is explicitly deferred to a future feature.
- In-editor (ImGui) log sink. This is deferred to a future feature.
- Color output in the console sink.
- Network/remote logging.
- Structured JSON logging.
- Asynchronous/non-blocking logging (all logging is synchronous and mutex-guarded).
- Log rotation or log file management beyond simple append.
- Automatic logging of `Error`/`Result<T>` types.

## Key entities

| Entity | Role |
|---|---|---|
| `LogLevel` | Enum: `Trace`, `Debug`, `Info`, `Warn`, `Error` (ordered most to least verbose). |
| `LogMessage` | Struct carrying level, tag, message, and source location (file, line, function). Passed to `Sink::write()`. |
| `Sink` | Abstract interface for log output destinations. Defines a single `write(const LogMessage&)` method. |
| `ConsoleSink` | Writes to stderr. Format: `[LEVEL] [Tag] message\n`. Always present, created at startup. |
| `FileSink` | Writes to a file. Format: `YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message\n`. Created on demand if `--log-file` is specified. |
| `MemorySink` | Accumulates messages in a `std::vector<LogMessage>`. Used only in unit test builds. Exposes `const std::vector<LogMessage>& messages() const` for test assertions. |
| `LogFilter` | Holds the global minimum level and per-tag overrides using prefix matching (e.g., `Asset` matches all tags starting with `Asset:`). Determines whether a given (tag, level) pair is enabled. |
| `LogConfig` | Struct with fields: `std::vector<std::shared_ptr<Sink>> sinks`, minimum level, per-tag overrides. Passed to `Logger::init()`. |
| `Logger` | Singleton that owns the filter, sink list, and mutex. Configured via `Logger::init(LogConfig)`. Provides `log(level, tag, file, line, func, fmt, args...)` dispatch. Supports `Logger::reset()` for test isolation. |

## Actors

| Actor | Interaction |
|---|---|
| **Engine developer** | Writes log macros in engine code. Declares `BUDDD_LOG_TAG` in each `.cpp` file. |
| **Game/editor developer** | Consumes log output for debugging. Uses CLI flags to control verbosity. |
| **Test author** | Writes Catch2 tests that exercise `MemorySink` to assert logged messages. |
| **End-user (CLI operator)** | Runs engine tools/editor with `--log-level`, `--log-file`, `--log-filter` flags. |

## User-visible behavior

1. Every `.cpp` file that uses log macros MUST declare a source tag at file scope via `BUDDD_LOG_TAG("Module:Sub")`. If a file uses any `BUDDD_LOG_*` macro without first declaring `BUDDD_LOG_TAG`, compilation fails.
2. All log output to console uses format: `[LEVEL] [Tag] message\n` — no timestamp prefix.
3. File log output uses format: `YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message\n` — ISO 8601 timestamp prefix.
4. Messages below the effective minimum level are silently discarded with zero formatting cost.
5. Log macros are valid in any thread; interleaving is prevented by an internal mutex.

## CLI flags

| Flag | Type | Repeatable | Description |
|---|---|---|---|
| `--log-level=<level>` | one of: `trace`, `debug`, `info`, `warn`, `error` | No | Set the global minimum log level. Overrides the build-type default. |
| `--log-file=<path>` | file path string | No | Enable the file sink and write to the given path. |
| `--log-filter=<source-pattern>=<level>` | `source-pattern` is a tag prefix pattern, `level` is one of the above (optional) | Yes | Override the minimum level for all tags matching the given prefix. Uses `=` as separator. Level is optional — if omitted, the source uses the global level (no override). Supports prefix matching: `Asset` matches `Asset`, `Asset:ModelLoader`, `Asset:Texture`, etc. Example: `--log-filter=Asset:ModelLoader=trace`. |

**Default thresholds (no flags)**:
- Debug build (NDEBUG not defined): `debug` level minimum; `trace` is never active by default.
- Release build (NDEBUG defined): `warn` level minimum.

## API reference

### Per-file tag declaration

```cpp
// Must appear at top of every .cpp that uses log macros, outside any namespace.
// Only one declaration per file; repeated declarations are an error.
BUDDD_LOG_TAG("Asset:ModelLoader");
```

### Log macros (primary API)

| Macro | Semantics |
|---|---|
| `BUDDD_LOG_TRACE("fmt", args...)` | Emit trace-level message. |
| `BUDDD_LOG_DEBUG("fmt", args...)` | Emit debug-level message. |
| `BUDDD_LOG_INFO("fmt", args...)` | Emit info-level message. |
| `BUDDD_LOG_WARN("fmt", args...)` | Emit warning-level message. |
| `BUDDD_LOG_ERROR("fmt", args...)` | Emit error-level message. |

Each macro automatically captures `__FILE__`, `__LINE__`, `__FUNCTION__` and uses the tag declared by `BUDDD_LOG_TAG`.

### Tag override (explicit)

```cpp
// For blocks needing a different tag than the file's declared BUDDD_LOG_TAG.
BUDDD_LOG_TAGGED_INFO("Other:Tag", "format {}", arg);
BUDDD_LOG_TAGGED_WARN("Other:Tag", "format {}", arg);
// ... similar for all five levels.
```

### Formatting style

All macros accept `std::format`-style format strings with `{}` placeholders:

```cpp
BUDDD_LOG_INFO("loaded mesh '{}' ({} vertices)", name, count);
```

### Underlying functions (for advanced use, e.g., custom wrappers)

```cpp
namespace buddd::log {

void log(LogLevel level, std::string_view tag,
         std::string_view fmt, auto&&... args,
         std::source_location loc = std::source_location::current());

}
```

(Not part of the normal API — the macros are the intended path.)

### LogMessage struct

The `LogMessage` struct carries structured log data from the logger to all sinks:

```cpp
struct LogMessage {
    LogLevel level;              // message severity
    std::string_view tag;        // source tag (e.g., "Asset:ModelLoader")
    std::string_view message;    // formatted message string
    std::string_view file;       // source file (__FILE__)
    int line;                    // source line (__LINE__)
    std::string_view function;   // enclosing function (__FUNCTION__)
};
```

### Test helper (tests/log_helpers.h)

A convenience RAII helper for unit tests that need to capture log output:

```cpp
namespace buddd::test {

struct ScopedMemoryLogger {
    std::shared_ptr<MemorySink> sink;
    ScopedMemoryLogger();   // creates a MemorySink and calls Logger::init()
    ~ScopedMemoryLogger();  // calls Logger::reset() for test isolation
};

}
```

## User stories

### Story 1 — Per-file source tag (Priority: P1)

As an engine developer, I want each `.cpp` file to declare its logging source tag once, so that every log message from that file is automatically attributed.

**Given** a `.cpp` file that uses `BUDDD_LOG_INFO`  
**When** the file is compiled without a `BUDDD_LOG_TAG` declaration  
**Then** compilation fails with an "undefined identifier" error for `BUDDD_CURRENT_LOG_TAG` (or equivalent).

**Given** a `.cpp` file with `BUDDD_LOG_TAG("Render:OpenGL")`  
**When** `BUDDD_LOG_INFO("hello")` is called  
**Then** the logged message includes `[Render:OpenGL]` in the output.

### Story 2 — Log levels and filtering (Priority: P1)

As a CLI operator, I want to control verbosity so that I see only the messages I care about.

**Given** a debug build with no flags  
**When** the engine runs  
**Then** `trace` messages are suppressed and `debug`+ messages are visible.

**Given** `--log-level=error`  
**When** the engine runs  
**Then** all output at `warn`, `info`, `debug`, `trace` is suppressed; only `error` messages appear.

**Given** `--log-level=info --log-filter=Asset:ModelLoader=trace`  
**When** the engine runs  
**Then** most tags output from `info+`, but `Asset:ModelLoader` outputs from `trace+`.

### Story 3 — Console sink (Priority: P1)

As a developer, I want clear, readable log output on the terminal.

**Given** the engine is running with default settings  
**When** a message is logged at `warn` level with tag `Engine`  
**Then** `[WARN] [Engine] <message>\n` appears on stderr.

**Given** a tag contains hierarchical segments like `Asset:ModelLoader`  
**When** a message is logged  
**Then** `[INFO] [Asset:ModelLoader] <message>\n` appears on stderr.

### Story 4 — File sink (Priority: P1)

As an operator running a headless capture, I want logs saved to a file for later review.

**Given** `--log-file=/tmp/engine.log`  
**When** the engine logs messages  
**Then** each line in `/tmp/engine.log` has the format `YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message`.

**Given** the engine starts without `--log-file`  
**When** messages are logged  
**Then** no file is created and no file I/O occurs.

### Story 5 — Thread safety (Priority: P2)

As a developer working with a multithreaded engine, I want log calls from any thread to not corrupt each other.

**Given** two threads calling `BUDDD_LOG_INFO` concurrently  
**When** both threads emit messages  
**Then** each message appears atomically (not interleaved with partial lines from the other thread).

### Story 6 — MemorySink for tests (Priority: P2)

As a test author, I want to assert that specific log messages are emitted during certain operations.

**Given** a test that registers a `MemorySink` with the logger  
**When** an operation that logs is executed  
**Then** the test can retrieve and assert against the captured messages.

### Story 7 — `BUDDD_LOG_TAGGED_*` macros (Priority: P3)

As a developer, I want to occasionally log with a different tag without restructuring code.

**Given** a file declares `BUDDD_LOG_TAG("Asset:ModelLoader")`  
**When** a block of code uses `BUDDD_LOG_TAGGED_WARN("Asset:Texture", "failed")`  
**Then** the output shows `[Asset:Texture]` instead of `[Asset:ModelLoader]`.

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | Five log levels exist (trace, debug, info, warn, error) in order of increasing severity. | Unit test asserts `LogLevel::Trace < LogLevel::Debug < LogLevel::Info < LogLevel::Warn < LogLevel::Error`. |
| AC-002 | `BUDDD_LOG_TAG` must be declared before any log macro in a translation unit, or compilation fails. | A compile-only test file with log macros but no `BUDDD_LOG_TAG` must fail to compile. |
| AC-003 | Log macros emit to console sink by default. | Unit test captures stderr output and verifies format `[LEVEL] [Tag] message`. |
| AC-004 | Console output format is `[LEVEL] [Tag] message\n` with no timestamp. | Unit test checks absence of ISO 8601 timestamp pattern. |
| AC-005 | File output format is `YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message\n` with ISO 8601 timestamp. | Integration test writes to a temp file, reads it back, and asserts timestamp prefix via regex. |
| AC-006 | `--log-level` correctly sets global minimum; messages below threshold are dropped. | Test with `MemorySink` verifies that at `--log-level=warn`, `info` messages are absent and `warn`/`error` messages are present. |
| AC-007 | `--log-filter` overrides per-tag level. | Test with `MemorySink` and filter `Asset:ModelLoader=trace` verifies trace messages appear for that tag only. |
| AC-008 | `--log-file` enables file sink; without it no file sink exists. | Test with `--log-file` creates file; test without creates no file (verify file does not exist). |
| AC-009 | File sink failure (permission denied, invalid path) does not crash; continues without file sink. | Test writes to `/dev/null/fail` (invalid path) and verifies engine continues + raw stderr warning. |
| AC-010 | Thread safety: concurrent log calls produce non-interleaved lines. | Stress test spawns 4 threads each logging 1000 messages; verify no line contains interleaved content. |
| AC-011 | `MemorySink` exists and accumulates messages in a `std::vector<LogMessage>`. | Unit test attaches `MemorySink`, logs 3 messages, retrieves vector, asserts size == 3 and `LogMessage` fields match expected values. |
| AC-012 | Debug build default threshold is `debug`; `trace` is off by default. | Build test binary in debug mode, log a `trace` message, verify it is dropped. |
| AC-013 | Release build default threshold is `warn`. | Build test binary in release mode, log `info` and `warn`, verify `info` is dropped and `warn` passes. |
| AC-014 | Macros check level before evaluating arguments (no side effects for disabled messages). | Test with an argument expression that increments a counter; verify counter unchanged when level is below threshold. |
| AC-015 | `BUDDD_LOG_TAGGED_*` macros override tag for a single call. | Unit test uses `BUDDD_LOG_TAGGED_INFO("Custom:Tag", "msg")` and verifies `[Custom:Tag]` in output. |
| AC-016 | `std::format`-style formatting works in all macros. | Unit test logs `BUDDD_LOG_INFO("int {} str {}", 42, "hello")` and asserts output contains `int 42 str hello`. |
| AC-017 | Source location (`__FILE__`, `__LINE__`, `__FUNCTION__`) is captured and available. | Unit test uses `MemorySink` and verifies the `LogMessage::file`, `LogMessage::line`, and `LogMessage::function` fields match the expected values. |
| AC-018 | No external dependencies — only C++26 standard library used. | Code review of `#include` directives in all new files confirms no third-party headers. |
| AC-019 | Logger backend `.cpp` is compiled as part of `src/engine/`. | Build system inspection confirms the file is listed in `src/engine/CMakeLists.txt`. |
| AC-020 | Logger is decoupled from `Error`/`Result<T>` — no automatic logging of errors. | Code review confirms no include of `Error`/`Result` headers in logger source files. |

## E2E Verification

This feature will be verified through:

- **Unit test suite** (`logging_tests.cpp` or similar) covering all acceptance criteria with `MemorySink`, stderr capture, and compile-time tests.
- **Integration smoke test**: A small CLI test app that exercises `--log-level`, `--log-file`, `--log-filter` and validates output on disk and stderr.
- **Compile-time test**: A standalone file in the test directory that intentionally omits `BUDDD_LOG_TAG` and is verified to fail compilation (via CMake `try_compile` or a dedicated build target).
- **Thread safety stress test**: Multi-threaded test with 4+ threads logging concurrently, validated programmatically.

## Success criteria

| ID | Metric |
|---|---|
| SC-001 | All 20 acceptance criteria pass in CI on every commit. |
| SC-002 | No measurable increase in link-time or compile-time for translation units that do not include logging headers. |
| SC-003 | A `trace`-level log call in a hot loop (10M iterations) with `--log-level=error` adds < 5 ns overhead per iteration (short-circuit check only). |
| SC-004 | File sink throughput: writing 10K messages to a local SSD file completes in under 500 ms. |

## Architecture / Design sketch

```
┌────────────────────────────────────────────────────────────────┐
│                        Macros (buddd/log.h)                    │
│  BUDDD_LOG_TAG, BUDDD_LOG_INFO(...), BUDDD_LOG_TAGGED_INFO    │
│  Captures: __FILE__, __LINE__, __FUNCTION__                    │
└───────────────────────────┬────────────────────────────────────┘
                            │ expands to
                            ▼
┌────────────────────────────────────────────────────────────────┐
│                     Logger (buddd/log/logger.h/.cpp)           │
│  - Singleton                                                 │
│  - log(level, tag, file, line, func, fmt, args...)            │
│  - Thread safety via std::mutex                               │
│  - Owns: LogFilter, std::vector<unique_ptr<Sink>>             │
└──────┬─────────────────────────┬──────────────────────────────┘
       │                         │
       ▼                         ▼
┌──────────────┐       ┌──────────────────┐
│  LogFilter   │       │  Sink interface  │
│  - global    │       │  write(const     │
│    min level │       │   LogMessage&)   │
│  - per-tag   │       │  = 0             │
│    overrides │       └──┬────┬────┬─────┘
└──────────────┘          │    │    │
                  ┌───────┘    │    └──────────┐
                  ▼            ▼               ▼
          ┌──────────┐ ┌──────────┐ ┌──────────────┐
          │Console   │ │ FileSink │ │ MemorySink   │
          │Sink      │ │          │ │ (tests only) │
          │(stderr)  │ │ (file)   │ │              │
          └──────────┘ └──────────┘ └──────────────┘
```

### Components

1. **`include/buddd/log.h`** (public header) — Macro definitions, `LogLevel` enum, `BUDDD_LOG_TAG` declaration macro, forward declarations. This is the sole public entry point.

2. **`src/engine/log/logger.h`** and **`src/engine/log/logger.cpp`** — `Logger` class, `Sink` interface, `LogFilter`, initialization from CLI args, shutdown.

3. **`src/engine/log/console_sink.h/.cpp`** — `ConsoleSink` implementation.

4. **`src/engine/log/file_sink.h/.cpp`** — `FileSink` implementation.

5. **`src/engine/log/memory_sink.h`** — `MemorySink` implementation (header-only, test-only; may live in a `test/` directory or be conditionally compiled).

### Initialization flow

1. CLI flags (e.g., `--log-level`, `--log-file`, `--log-filter`) are parsed by the CLI parsing infrastructure (`src/cmd/`) into a `LogConfig` struct containing the minimum level, per-tag overrides, and a sink list.
2. At program startup (before any thread spawns or any log macro is used), `Logger::init(LogConfig)` is called with the parsed config.
3. `Logger::init()` configures the `LogFilter`, creates `ConsoleSink` (always), and optionally creates `FileSink` based on the config.
4. After init, all `BUDDD_LOG_*` macros call through `Logger::log()`.
5. On shutdown (or test teardown), `Logger::shutdown()` flushes all sinks and releases resources.
6. `Logger::reset()` clears all sinks, filter state, and mutex — enabling clean test isolation between test cases.

## Testing strategy

| Layer | Approach | Tools |
|---|---|---|
| Unit tests | Test each macro, level filter, tag system, format string, MemorySink | Catch2 v3, `MemorySink`, `ScopedMemoryLogger` helper (`tests/log_helpers.h`), stderr redirect |
| Compile-time tests | Verify missing `BUDDD_LOG_TAG` fails to compile | CMake `try_compile` on a dedicated source file |
| Integration | Test CLI flags by launching a test binary with args and checking output | Bash script or Catch2 `main()` with argv injection |
| Thread safety | Stress test with 4 threads logging 1000 messages each, verify output integrity | Catch2 + `std::thread` |

## Edge cases

1. **File sink path contains spaces**: The `--log-file` argument is a raw string; the OS handles path quoting as per normal CLI conventions. The logger should not modify the path.
2. **File sink path is a directory**: `--log-file=/tmp/` (a directory). The `FileSink` should fail to open, log a raw warning to stderr, and continue without file sink.
3. **Very long source tag**: Tags longer than 255 characters are truncated (with a `warn`-level internal log).
4. **Empty source tag**: `BUDDD_LOG_TAG("")` should compile but produce empty `[]` brackets. (Documented as allowed but discouraged.)
5. **Very long log message ( > 32 KB )**: Message is truncated at a reasonable limit (e.g., 32 KB) before dispatching to sinks.
6. **Logger::init() called twice**: `Logger::init()` is idempotent — the second call is a no-op. Use `Logger::reset()` before `init()` when the configuration must change (e.g., between unit tests).
7. **`--log-filter` with tag that has no corresponding log calls**: Filter entry is stored but never matched — harmless.
8. **`--log-filter` with duplicate tag**: Last occurrence wins.
9. **`--log-level` with invalid level string**: The CLI parser validates the level string and returns a `Result<RunningArgs>` containing the `LogConfig`. Invalid values are caught during parsing, an error message is printed to stderr, and the process exits with a non-zero exit code — before `Logger::init()` is ever called.
10. **Unicode/UTF-8 characters in messages**: Passed through as-is; no encoding transformation.
11. **Logging after shutdown**: If `BUDDD_LOG_*` macros are called after `Logger::shutdown()`, the message is silently dropped (no crash, no undefined behavior).
12. **Logger::init() / Logger::shutdown() not thread-safe**: These methods must be called from single-threaded startup/teardown code. Calling them concurrently with log operations from other threads is undefined behavior.
13. **`--log-filter` prefix matching boundary**: A filter pattern `Asset=trace` matches the tag `Asset` itself as well as all tags starting with `Asset:` (e.g., `Asset:ModelLoader`, `Asset:Texture`). A pattern `Asset:` (trailing colon) matches `Asset:ModelLoader` but does NOT match the bare tag `Asset`.

## Error cases

| Scenario | Behavior |
|---|---|
| `--log-file` path cannot be opened (permission denied, invalid path, out of space) | Write a warning directly to stderr via low-level `write(2, ...)` (no logger dependency), skip file sink creation, continue execution. |
| `BUDDD_LOG_TAG` missing in a `.cpp` that uses log macros | Compilation fails with "undefined identifier" error. |
| `BUDDD_LOG_TAG` declared twice in the same file | Compilation error (redefinition). |
| Logger not initialized before first log call | Log call is silently dropped (no crash). |
| Logger::log() called with a null or empty format string | No crash; a placeholder message is logged instead. |
| Memory exhaustion during formatting | `std::format` may throw `std::bad_alloc`; this is allowed to propagate (the caller is responsible for the format arguments). |

## Documentation updates

The following existing documents must be updated when this feature is implemented:

- **`src/cmd/main.cpp`** — CLI help text must document `--log-level`, `--log-file`, and `--log-filter` flags.
- **CLI parsing infrastructure** (`src/cmd/` or `src/engine/app_config.h`) — Must parse log CLI flags into a `LogConfig` struct (including sinks, minimum level, per-tag overrides) for `Logger::init()`.
- **New ADR** required — Architectural decision record for the logging system (design decisions, sink interface, lifecycle).
- **New wiki page** required — API reference, conventions (tag naming, macro usage), and CLI usage guide.
- **`docs/wiki/domain/business-rules.md`** — May need an observability note about structured log output stream replacing raw `std::cerr`.
- **`.specs/sprint-2026-06/asset-manager/spec.md` and `implementation-contract.md`** — May reference old `std::cerr` conventions that are superseded by the logging system.

## Permissions and security

- No elevated permissions are required for the logging system itself.
- File sink writes only to the path explicitly provided via `--log-file`. No automatic path expansion or `~` tilde expansion is performed (the shell handles this).
- No logging of sensitive data (passwords, tokens) is performed by the framework. Callers are responsible for not logging sensitive information.
- No network access.

## Observability

- The logger itself should log internal warnings (e.g., tag truncation) via the same logging API (tag: `[Log]`).
- Internal errors during logger init (file sink failure) use a raw `write(2, ...)` to stderr since the logger may not yet be initialized.
- Unit tests create a `LogConfig` with a `MemorySink` and call `Logger::init(LogConfig)` / `Logger::reset()` for test isolation. A `ScopedMemoryLogger` test helper (in `tests/log_helpers.h`) automates this: constructs a `MemorySink`, calls `Logger::init()`, and calls `Logger::reset()` on destruction.

## Out of scope

- Migration of existing `std::cerr`/`printf`/`fprintf` calls to the new logger.
- In-editor (ImGui) log viewer or sink.
- Color/ANSI output in the console sink.
- Asynchronous / non-blocking / lock-free logging.
- JSON or structured log output.
- Log file rotation, compression, or archival.
- Remote logging (syslog, network socket).
- Automatic logging of `Error`/`Result<T>` types.

## Assumptions

1. The program's `main()` (or equivalent entry point) parses CLI args into a `LogConfig` struct and calls `Logger::init(LogConfig)` before any thread spawns or any log macro is used.
2. A single `Logger` singleton is sufficient — there is no need for multiple independent logger instances.
3. `std::format` is available (C++26 standard library). The `fmt` library is not needed.
4. Mutex-based synchronization is adequate for the expected log volume (< 10K messages/second peak). If this becomes a bottleneck in the future, async logging can be introduced later.
5. The `BUDDD_LOG_TAG` macro expands to a file-scope `static constexpr` string or equivalent, incurring zero runtime cost.
6. CLI args are parsed by the existing CLI infrastructure in `src/cmd/` into a `LogConfig` struct which is then passed to `Logger::init(LogConfig)`.
7. Thread safety testing is best-effort — a single stress test is sufficient to demonstrate correctness.
8. The MemorySink is only compiled into test builds (via `#ifdef` or separate compilation unit), not into release builds.

## Open questions

None. All architectural decisions were confirmed during the grill-me session with the human.

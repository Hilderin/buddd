# ADR-020-custom-logging-system - Custom Logging System

## Status

Accepted

## Context

The Buddd Engine historically used ad-hoc `std::cerr`, `std::printf`, and `std::fprintf` calls scattered across ~25 source files. This approach had several unsustainable properties:

- **No level filtering** — Debug diagnostics and critical errors appeared on the same stream with no way to separate them at runtime.
- **No structured control** — There was no mechanism to enable, disable, or redirect logging without editing and recompiling source code.
- **No source attribution** — Messages did not carry a consistent module or sub-component tag, making it hard to identify the origin of a log line in a multi-module codebase.
- **No thread safety** — Concurrent writes from different threads could interleave output, producing corrupted lines.
- **No file sink** — There was no built-in mechanism to capture log output to a file for post-mortem analysis.
- **No testability** — Log output could not be captured or asserted in unit tests.

A structured logging framework was required before the engine grew more complex. Three approaches were evaluated: an existing third-party library (spdlog, fmtlog), a wrapper around `std::format` with minimal abstraction, and a custom lightweight logger built from scratch.

## Decision

A **custom lightweight logger** is implemented in `src/engine/log/` with the following design decisions:

### 1. Custom logger over third-party libraries

spdlog and fmtlog were evaluated and rejected:

- **spdlog** is a mature, widely-used library but pulls in significant dependencies (fmt library, header-only complexity) and has a large API surface that exceeds the project's needs. Its async logging, logger registry, and sink registry add complexity with no immediate benefit.
- **fmtlog** is faster (lock-free) but uses a non-standard formatting DSL and has a smaller community. Its lock-free design adds complexity that is not yet justified by the engine's log volume (< 10K messages/second peak).
- **Custom logger** is ~500 lines of C++26, has zero external dependencies, exactly matches the project's needs (5 levels, simple sinks, thread safety via mutex), and avoids maintaining a dependency on a third-party logging library. If performance becomes a bottleneck, async logging can be introduced later without changing the macro API.

### 2. Five log levels

`Trace < Debug < Info < Warn < Error`, ordered most to least verbose. Default thresholds vary by build type:
- Debug build (`NDEBUG` not defined): `Debug` minimum; `Trace` is off by default, enabled only via `--log-level=trace`.
- Release build (`NDEBUG` defined): `Warn` minimum.

### 3. Macro-based API with mandatory per-file source tag

The primary API surface is C++ macros (`BUDDD_LOG_INFO`, `BUDDD_LOG_WARN`, etc.) rather than direct function calls. Every `.cpp` file that uses log macros **must** declare a source tag at file scope via `BUDDD_LOG_TAG("Module:Sub")`. Missing tags produce a compile-time error (reference to undefined `BUDDD_CURRENT_LOG_TAG`). This enforces consistent attribution with zero runtime cost — the tag is a `static constexpr std::string_view`.

`BUDDD_LOG_TAGGED_*` macros allow explicit tag override for a single call without changing the file's declared tag.

### 4. Singleton Logger with LogConfig-based initialization

A single `Logger` singleton is configured via `Logger::init(LogConfig)`, where `LogConfig` carries the sink list, minimum level, and per-tag overrides. This provides:
- **Idempotent init**: calling `init()` twice is a no-op (use `reset()` first to reconfigure).
- **Test isolation**: `Logger::reset()` destroys all state, enabling clean setup/teardown between test cases.

### 5. Sink interface with three implementations

An abstract `Sink` interface with a single `write(const LogMessage&)` method supports:

- **ConsoleSink** — Always present, writes `[LEVEL] [Tag] message\n` to stderr. No timestamp, no color.
- **FileSink** — Created on demand via a static factory `FileSink::create(path)`. Returns `nullptr` on failure (writes a raw warning to stderr via POSIX `write(2)`, since the logger may not be initialised yet). Uses append mode (`std::ios::app`). Output format: `YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message\n` with ISO 8601 timestamp.
- **MemorySink** — Header-only, compiled only in test builds (`#ifdef BUDDD_TESTING`). Accumulates messages in a `std::vector<LogMessage>` for test assertions. A `ScopedMemoryLogger` RAII helper automates its lifecycle.

### 6. Thread safety via std::mutex

- `Logger::log()` and `Logger::is_enabled()` are safe to call from any thread.
- `log()` locks an internal `std::mutex` during sink iteration; `is_enabled()` performs a lock-free filter check (the filter is const after init).
- `init()`, `shutdown()`, and `reset()` are NOT thread-safe — they must be called from single-threaded startup/teardown.

### 7. Source location capture

All log macros automatically capture `__FILE__`, `__LINE__`, and `__FUNCTION__` via the preprocessor, storing them in the `LogMessage` struct for diagnostic use.

### 8. std::format formatting

All macros accept `std::format`-style format strings with `{}` placeholders. The `Logger::log()` template uses `std::vformat` for type-safe formatting. There is no dependency on the `fmt` library.

### 9. CLI control

Three CLI flags control the logger at runtime:
- `--log-level=<level>` — Set global minimum level.
- `--log-file=<path>` — Enable the file sink.
- `--log-filter=<pattern>=<level>` — Override level for tags matching a prefix (repeatable).

These are parsed in `src/cmd/app_config.cpp` before all other startup logic, and the resulting `LogConfig` is passed to `Logger::init()` in `src/cmd/main.cpp`.

### 10. Zero external dependencies

The logger uses only the C++26 standard library. No spdlog, fmt, or any third-party headers. The sole exception is POSIX `write(2)` used in `FileSink::create()` for raw stderr output before the logger is initialised — this is mandated by the contract for reliability during startup failure paths.

### 11. Decoupled from Error/Result\<T\>

The logger is fully independent of the engine's `Error` and `Result<T>` types. No file in `src/engine/log/` includes `error.h`. The CLI parsing layer (`src/cmd/app_config.cpp`) uses `Result<T>` for its own parsing, but this is separate from the logger itself.

## Alternatives considered

- **No structured logging** — Continue with ad-hoc `std::cerr`/`printf`. Rejected because it lacks level filtering, source attribution, thread safety, file output, and testability — all needed before the engine grows more complex.

- **spdlog** — Rejected because it pulls in the `fmt` library as a mandatory dependency, has a large API surface (logger registry, async logging, multiple sink types not needed yet), and adds a dependency to maintain. The project's log volume does not justify spdlog's feature set at this stage.

- **fmtlog** — Rejected because it uses a non-standard formatting DSL, is less widely used, and its lock-free design adds complexity without proven need (< 10K messages/second peak). If async logging becomes necessary in the future, it can be introduced behind the same macro API.

- **Non-macro API** — Direct function calls without preprocessor capture. Rejected because macros are the only portable way to automatically capture `__FILE__`, `__LINE__`, and `__FUNCTION__` without boilerplate at every call site.

- **Per-instance logger (non-singleton)** — Rejected because a single logger is sufficient for the engine's architecture. Multiple instances would complicate CLI flag handling and test setup without benefit.

- **Async / lock-free logging** — Deferred. The mutex-based approach is adequate for the expected volume. Async logging can be introduced later as a new sink wrapper without changing the macro API.

- **Color / ANSI console output** — Deferred. Console output is intentionally plain to keep the initial implementation simple and avoid terminal compatibility issues.

## Consequences

### Positive

- All new code uses a consistent logging API with level filtering, source attribution, and thread safety from day one.
- No spdlog or fmt dependency to maintain — one fewer external dependency in the project.
- Testable logging via `MemorySink` — log output can be captured and asserted in unit tests without stderr redirection hacks.
- Compile-time enforcement of source tags prevents untagged log messages from entering the codebase.
- Runtime CLI control (`--log-level`, `--log-file`, `--log-filter`) gives operators full control over verbosity without recompilation.
- The sink interface is extensible — new sink types (in-editor ImGui sink, async wrapper, network sink) can be added later without changing the macro API or logger core.
- Zero-cost abstraction for disabled log levels: the `is_enabled()` short-circuit check prevents argument evaluation and formatting for filtered messages.

### Negative

- Existing `std::cerr`/`printf` calls (~25 files) are not migrated — this is deferred to a future feature, creating a transitional period with two logging approaches in the codebase.
- Custom logger development and maintenance effort replaces using an off-the-shelf solution (though the logger is small, ~500 lines).
- Mutex-based synchronization blocks sink writes, which could become a bottleneck under very high log volume (> 100K messages/second). If this becomes a problem, async logging can be introduced behind the sink abstraction.
- No color output in the console sink, which some developers may find less readable during development. This is a deliberate simplification that can be added later.
- POSIX `write(2)` dependency in `FileSink::create()` for raw stderr warnings — this is a minor portability concern if the project ever targets a non-POSIX platform (Windows). On Windows, `_write()` from `<io.h>` would be an alternative.

## Related documents

- SPEC-021 — Logging System specification (`.specs/sprint-2026-06/logging-system/spec.md`)
- IMPL-021 — Logging System implementation contract (`.specs/sprint-2026-06/logging-system/implementation-contract.md`)
- ADR-001 — The logger is decoupled from the `Error`/`Result<T>` pattern defined in ADR-001.
- ADR-009 — Test file naming convention followed (`logging_tests.cpp`).
- ADR-014 — CLI flag parsing infrastructure lives in `src/cmd/` as defined by ADR-014; log flags are parsed there.
- ADR-019 — The logger is architecture-boundary compliant (stdlib only, no platform/graphics dependencies).

---

*Derived from SPEC-021 and the accepted implementation contract IMPL-021.*

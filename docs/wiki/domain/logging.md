# Logging System

> **API reference, conventions, and CLI usage guide.**
>
> Reference: [SPEC-021](/.specs/sprint-2026-06/logging-system/spec.md), [IMPL-021](/.specs/sprint-2026-06/logging-system/implementation-contract.md), [ADR-020](/docs/adr/ADR-020-custom-logging-system.md)

## Quick start

```cpp
// 1. Include the public header
#include "log/log.h"

// 2. Declare a per-file source tag at file scope (outside any namespace)
BUDDD_LOG_TAG("Engine");

// 3. Use log macros anywhere in the file
BUDDD_LOG_INFO("hello world");
BUDDD_LOG_INFO("loaded mesh '{}' ({} vertices)", name, count);
BUDDD_LOG_WARN("texture not found, using fallback");
BUDDD_LOG_ERROR("failed to open file: {}", path);
```

Every `.cpp` file that uses any `BUDDD_LOG_*` macro **must** declare a source tag via `BUDDD_LOG_TAG` at file scope. If the tag is missing, compilation fails with an "undefined identifier" error for `BUDDD_CURRENT_LOG_TAG`.

## Log levels

Five ordered levels, from most verbose (0) to least verbose (4):

| Level | Enum value | When to use | Enabled by default |
|-------|-----------|-------------|-------------------|
| Trace | `LogLevel::Trace` (0) | Detailed step-by-step diagnostics; everything executed | Off (debug & release) |
| Debug | `LogLevel::Debug` (1) | Development-time information; state dumps, intermediate values | On (debug build) / Off (release) |
| Info | `LogLevel::Info` (2) | Normal operational messages; lifecycle events, load completion | On (both) |
| Warn | `LogLevel::Warn` (3) | Recoverable issues; fallback paths, degraded behaviour | On (both) |
| Error | `LogLevel::Error` (4) | Unrecoverable errors; failures that abort the current operation | On (both) |

### Default thresholds (no CLI flags)

- **Debug build** (`NDEBUG` not defined): minimum level is `Debug`. `Trace` messages are never active by default.
- **Release build** (`NDEBUG` defined): minimum level is `Warn`. `Debug` and `Info` are suppressed by default.

### When to use each level

| Level | Typical use cases |
|-------|-------------------|
| `BUDDD_LOG_TRACE` | Entering/leaving functions, loop iterations, granular state transitions. Use sparingly — high volume expected. |
| `BUDDD_LOG_DEBUG` | Variable dumps, intermediate computation results, development-only diagnostics. Not shown in release builds by default. |
| `BUDDD_LOG_INFO` | Engine startup, scene loaded, asset cached, window created. Operator-visible signals. |
| `BUDDD_LOG_WARN` | Texture not found (fallback used), deprecated API called, minor resource exhaustion. Operation continues. |
| `BUDDD_LOG_ERROR` | File I/O failure, shader compilation error, device lost. The current operation cannot complete. |

## Source tags

### Naming convention

Source tags use a hierarchical `Module:SubComponent` format:

- **`Engine`** — general engine startup/shutdown messages
- **`Log`** — internal logger diagnostics (tag truncation warnings, etc.)
- **`Render:OpenGL`** — OpenGL backend operations
- **`Render:Vulkan`** — future Vulkan backend operations
- **`Asset:ModelLoader`** — model loading from glTF files
- **`Asset:Texture`** — texture loading and caching
- **`Asset:FileWatcher`** — file system monitoring events
- **`Scene`** — scene graph operations (World, Entity, Component)
- **`Input`** — input system events
- **`Platform:SDL3`** — SDL3 platform operations
- **`Cmd`** — CLI command execution

The format is `Module:Sub` where `Module` is the engine submodule name and `Sub` is the specific component within it. Tags use PascalCase segments joined by colons.

### Prefix matching

`--log-filter` uses prefix matching: a pattern `Asset` matches all tags starting with `Asset:` (e.g., `Asset:ModelLoader`, `Asset:Texture`) as well as the exact tag `Asset`. A pattern `Asset:` (trailing colon) matches `Asset:ModelLoader` but does **not** match the bare `Asset`.

### Empty tag

`BUDDD_LOG_TAG("")` compiles but produces empty `[]` brackets in output. Allowed but discouraged — always use a descriptive tag.

### Tag truncation

Tags longer than 255 characters are truncated. An internal warning is emitted to stderr when truncation occurs.

## Macro reference

### Per-file tag declaration

```cpp
BUDDD_LOG_TAG("Module:SubComponent");
```
- Must appear at file scope (outside any namespace) before any `BUDDD_LOG_*` usage.
- Exactly one declaration per file; repeated declarations are a compile error.
- Expands to `static constexpr std::string_view BUDDD_CURRENT_LOG_TAG = ...` — zero runtime cost.

### Standard log macros

| Macro | Level |
|-------|-------|
| `BUDDD_LOG_TRACE("fmt", args...)` | Trace |
| `BUDDD_LOG_DEBUG("fmt", args...)` | Debug |
| `BUDDD_LOG_INFO("fmt", args...)` | Info |
| `BUDDD_LOG_WARN("fmt", args...)` | Warn |
| `BUDDD_LOG_ERROR("fmt", args...)` | Error |

Each macro:
- Automatically captures `__FILE__`, `__LINE__`, `__FUNCTION__`.
- Uses the tag declared by `BUDDD_LOG_TAG`.
- Accepts `std::format`-style format strings with `{}` placeholders.
- Checks the level filter before evaluating arguments — no side effects for disabled messages.

### Tag override macros (explicit tag)

```cpp
BUDDD_LOG_TAGGED_INFO("Other:Tag", "format {}", arg);
BUDDD_LOG_TAGGED_WARN("Other:Tag", "format {}", arg);
```

Available for all five levels. Overrides the file's declared tag for a single call. Useful for logging with a different tag without restructuring code.

### Formatting

All macros use `std::format`-style formatting with `{}` placeholders:

```cpp
BUDDD_LOG_INFO("int {} str {}", 42, "hello");
// Output: [INFO] [Tag] int 42 str hello
```

### Underlying function (advanced use)

```cpp
namespace buddd::log {
    void log(LogLevel level, std::string_view tag,
             std::string_view fmt, auto&&... args,
             std::source_location loc = std::source_location::current());
}
```

The macros are the intended API path. Use the underlying function only for custom wrappers.

### LogMessage struct

```cpp
struct LogMessage {
    LogLevel        level;
    std::string_view tag;       // source tag (e.g. "Asset:ModelLoader")
    std::string     message;    // formatted message (owned string)
    std::string_view file;      // __FILE__ value
    int             line;       // __LINE__ value
    std::string_view function;  // __FUNCTION__ value
};
```

## CLI flags

| Flag | Type | Repeatable | Description |
|------|------|-----------|-------------|
| `--log-level=<level>` | one of: `trace`, `debug`, `info`, `warn`, `error` | No | Set the global minimum log level. Overrides the build-type default. |
| `--log-file=<path>` | file path | No | Enable file sink and write to the given path (append mode, ISO 8601 timestamps). |
| `--log-filter=<pattern>=<level>` | `<pattern>` = tag prefix, `<level>` = one of the above (optional) | Yes | Override the minimum level for all tags matching the given prefix. |

### Examples

```bash
# Set global level to error only
buddd run triangle --log-level=error

# Log to a file with debug-level detail
buddd run phong --log-level=debug --log-file=/tmp/phong.log

# Only see Asset:ModelLoader at trace level, everything else at info+
buddd run gltf-demo --log-level=info --log-filter=Asset:ModelLoader=trace

# Multiple filter overrides
buddd run cube --log-level=warn --log-filter=Engine=debug --log-filter=Render:OpenGL=info
```

### Behaviour notes

- `--log-level` with an invalid level string causes the engine to print an error and exit with code 1 before any initialisation.
- `--log-file` failure (e.g., permission denied, path is a directory) writes a raw warning to stderr and continues without a file sink — the engine does not crash.
- `--log-filter` uses `=` as separator. The level is optional — if omitted, the source uses the global level (no effective override).
- Unknown flags are silently ignored (existing CLI convention).

## Sinks

### Console sink (always present)

Format: `[LEVEL] [Tag] message\n` — written to stderr. No timestamp, no colour.

```
[INFO] [Engine] hello world
[WARN] [Asset:Texture] texture not found, using fallback
[ERROR] [Render:OpenGL] shader compilation failed
```

### File sink (optional, via `--log-file`)

Format: `YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message\n` — ISO 8601 timestamp prefix, append mode.

```
2026-06-06T14:30:00 [INFO] [Engine] hello world
2026-06-06T14:30:01 [WARN] [Asset:Texture] texture not found, using fallback
```

Each message is flushed immediately. Messages longer than 32 KB are truncated before dispatch.

### Memory sink (test only)

Compiled only in test builds (`#ifdef BUDDD_TESTING`). Accumulates messages in a `std::vector<LogMessage>` for unit test assertions. Used via the `ScopedMemoryLogger` RAII helper in `tests/log_helpers.h`.

## Thread safety

- `BUDDD_LOG_*` macros are safe to call from any thread.
- Internally, a `std::mutex` protects sink writes — messages from concurrent threads are never interleaved.
- `Logger::init()`, `Logger::shutdown()`, and `Logger::reset()` are **NOT** thread-safe — they must be called from single-threaded startup/teardown.
- `Logger::is_enabled()` performs a lock-free filter check (the filter is const after init).

## Best practices

1. **Declare `BUDDD_LOG_TAG` in every `.cpp` file** — this is enforced by the compiler, so it's not optional. Use a descriptive `Module:Sub` format.

2. **Use `BUDDD_LOG_*` macros, not `std::cerr` or `printf`** — new code should always use the structured logger. A future feature will migrate existing ad-hoc output.

3. **Choose the right level** — `info` for normal operational messages, `warn` for recoverable issues, `error` for failures. Use `debug`/`trace` sparingly (high volume).

4. **Avoid `BUDDD_LOG_TAGGED_*` in normal code** — these are for exceptional cases where a single call needs a different attribution. Prefer declaring a separate `.cpp` file with its own `BUDDD_LOG_TAG` for logically distinct components.

5. **Do not log sensitive data** — the framework does not filter message content. Passwords, tokens, and personal data should never appear in log output.

6. **Check the level filter before expensive argument formatting** — the macros already short-circuit on the filter, but avoid putting expensive computations inside macro arguments for disabled levels (the `is_enabled()` check prevents argument evaluation, so this is safe by default).

7. **Use `--log-filter` for debugging specific subsystems** — rather than enabling `--log-level=trace` globally (which floods the output), target specific tags: `--log-filter=Asset:ModelLoader=trace`.

8. **File sink for headless/stress runs** — always use `--log-file` when running headless or batch tests to capture diagnostic output for post-mortem analysis.

## Test helpers

```cpp
#include "log/log.h"

// In a test:
{
    buddd::test::ScopedMemoryLogger logger;  // creates MemorySink, calls Logger::init()
    BUDDD_LOG_INFO("hello {}", 42);
    BUDDD_LOG_WARN("warning");
    CHECK(logger.sink->messages().size() == 2);
    CHECK(logger.sink->messages()[0].tag == "MyTestTag");
    CHECK(logger.sink->messages()[0].message == "hello 42");
}   // ~ScopedMemoryLogger calls Logger::reset() for isolation
```

## Reference

- Spec: [SPEC-021](/.specs/sprint-2026-06/logging-system/spec.md) — Full specification, user stories, acceptance criteria, edge cases
- Implementation contract: [IMPL-021](/.specs/sprint-2026-06/logging-system/implementation-contract.md) — Required implementation behaviour, test requirements, API signatures
- ADR: [ADR-020](/docs/adr/ADR-020-custom-logging-system.md) — Architectural decisions (custom logger, macro API, singleton, sink interface, thread safety)

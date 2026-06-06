# Code Review — Logging System (SPEC-021 / IMPL-021)

## Review summary

The implementation is **approved** — all acceptance criteria are implemented and tested, all API signatures match the contract, the build integrates cleanly, and 395/395 tests pass (including 28 logging test cases with 8077 assertions and a compile-failure test).

## What was checked

### Files reviewed

**New files (11):**
- `src/engine/log/log.h` — Public header: LogLevel, LogMessage, Sink, LogConfig, Logger, macros
- `src/engine/log/log_filter.h` — LogFilter class (header-inline)
- `src/engine/log/log_filter.cpp` — LogFilter implementation file (empty, for build consistency)
- `src/engine/log/logger.h` — Logger internal header (includes log.h)
- `src/engine/log/logger.cpp` — Logger::Impl, init/shutdown/reset, is_enabled, write_to_sinks
- `src/engine/log/console_sink.h` — ConsoleSink declaration
- `src/engine/log/console_sink.cpp` — ConsoleSink implementation (stderr, `[LEVEL] [Tag] message\n`)
- `src/engine/log/file_sink.h` — FileSink declaration with `static create()` factory, private constructor
- `src/engine/log/file_sink.cpp` — FileSink implementation (append mode, ISO 8601 timestamp)
- `src/engine/log/memory_sink.h` — MemorySink (header-only, `#ifdef BUDDD_TESTING` guarded)
- `tests/log_helpers.h` — ScopedMemoryLogger RAII helper

**Modified files (5):**
- `src/cmd/app_config.h` — Added `parse_logging_args()` declaration
- `src/cmd/app_config.cpp` — Implemented `parse_logging_args()` for `--log-level`, `--log-file`, `--log-filter`
- `src/cmd/main.cpp` — Calls `parse_logging_args()` at startup, then `Logger::init()`
- `tests/CMakeLists.txt` — Added `src/engine` include path, compile-fail test target for missing_tag
- `experiments-spec-driven-dev.md` — Minor documentation notes (not logging-related)

**Test files (2):**
- `tests/logging_tests.cpp` — 28 Catch2 `TEST_CASE` entries tagged `[logging]`
- `tests/compile_fail/missing_tag.cpp` — Compile-time test: log macro without BUDDD_LOG_TAG

### Criteria checked

| # | Check | Result |
|---|-------|--------|
| 1 | All 11 new files exist at correct paths | ✅ |
| 2 | API signatures match contract exactly | ✅ |
| 3 | All 20 acceptance criteria (AC-001 to AC-020) implemented and tested | ✅ |
| 4 | Configuration/build integration correct | ✅ |
| 5 | No external dependencies (C++26 std only) | ✅ (see W-01) |
| 6 | No Error/Result<T> dependency in `src/engine/log/` | ✅ |
| 7 | Thread safety correctly implemented | ✅ |
| 8 | Edge cases handled | ✅ |
| 9 | MemorySink guarded by `BUDDD_TESTING` | ✅ |
| 10 | FileSink::create() returns `unique_ptr` (nullptr on failure), constructor private | ✅ |
| 11 | Default levels use `NDEBUG` correctly | ✅ |
| 12 | All tests pass (395/395) | ✅ |
| 13 | Compile-fail test for missing BUDDD_LOG_TAG | ✅ |
| 14 | Console sink: no timestamp, `[LEVEL] [Tag] message\n` format | ✅ |
| 15 | File sink: ISO 8601 timestamp prefix, append mode | ✅ |
| 16 | Tag truncation (>255 chars) with stderr warning | ✅ |
| 17 | Message truncation (>32 KB) | ✅ |
| 18 | Macros use `__VA_OPT__` for zero-arg format strings | ✅ |
| 19 | CLI parsing: `--log-level`, `--log-file`, `--log-filter` parsed before subsystem init | ✅ |
| 20 | Visual/rendering check: N/A (logging is text output, not rendered visual) | N/A |

### AC-to-test mapping

| AC | Test(s) | Status |
|----|---------|--------|
| AC-001 | T-01: LogLevel ordering | ✅ |
| AC-002 | T-19 / compile_fail/missing_tag.cpp | ✅ |
| AC-003 | T-04: Console sink format (stderr redirect) | ✅ |
| AC-004 | T-04: No ISO 8601 timestamp in console | ✅ |
| AC-005 | T-05: File sink format with timestamp regex | ✅ |
| AC-006 | T-06: Global level filtering (Warn, check info absent) | ✅ |
| AC-007 | T-07: Tag override filtering | ✅ |
| AC-008 | T-08: File sink enabled/disabled via config | ✅ |
| AC-009 | T-09: File sink failure (nullptr, logger continues) | ✅ |
| AC-010 | T-18: Thread safety stress test (4 threads, 1000 msgs each) | ✅ |
| AC-011 | T-02: MemorySink accumulates 3 messages | ✅ |
| AC-012 | T-10: Debug build default = Debug | ✅ |
| AC-013 | T-10: Release build default = Warn | ✅ |
| AC-014 | T-12: No side effects for disabled levels | ✅ |
| AC-015 | T-13: BUDDD_LOG_TAGGED_INFO overrides tag | ✅ |
| AC-016 | T-14: `std::format`-style works (int {} str {}) | ✅ |
| AC-017 | T-03: Source location fields (file, line, function) | ✅ |
| AC-018 | T-15 + code review: no third-party includes | ✅ |
| AC-019 | T-16 + build system: logger compiled in engine | ✅ |
| AC-020 | T-17 + grep: no Error/Result in logger | ✅ |

Additional edge case tests: T-20 (init idempotent), T-21 (after shutdown safe), T-22 (before init safe), T-23 (empty format string), T-24 (file append mode), T-25 (empty source tag), tag truncation, long message truncation, prefix matching boundary, last-match-wins.

---

## Blocking issues

None.

The implementation is complete, correct, and all acceptance criteria are met.

---

## Warnings

- **W-01 (`<unistd.h>` in `file_sink.cpp`)**: `file_sink.cpp` includes `<unistd.h>` (POSIX) for the `::write(STDERR_FILENO, ...)` raw stderr warning on file-open failure. This is beyond C++26 standard library (AC-018), but **explicitly mandated by the implementation contract** (§5) which requires POSIX `write(2)` for low-level output when the logger may not yet be initialized. Acceptable given the Linux/POSIX target and clear contract justification.

- **W-02 (FileSink opens file twice)**: `FileSink::create()` creates a temporary `Impl` to verify the file is writable, then the private constructor creates another `Impl`. This opens the file twice on success. A minor inefficiency — could be refactored to construct the FileSink first and open once. Works correctly in practice.

- **W-03 (Contract off-by-one in prefix lengths)**: The contract specifies `substr(0, 11)` for `--log-level=` and `substr(0, 12)` for `--log-filter=`. The actual lengths are 12 and 13 respectively. The **implementation correctly uses the real lengths** (12 and 13). This is a contract typo, not an implementation issue.

- **W-04 (No CLI integration test)**: The contract explicitly notes that process-level CLI integration tests (invoking the buddd binary with `--log-level` etc.) are acceptable to skip. The unit tests cover all functionality through MemorySink and manual FileSink creation. Consider adding an integration test for end-to-end CLI validation.

- **W-05 (MemorySink read while threads write not guarded)**: The thread safety stress test (T-18) correctly joins threads before reading the collector. The MemorySink itself (used in other tests) is documented as not thread-safe which is acceptable for test-only single-threaded use. No action needed.

---

## Required changes

None.

---

## Suggested improvements

- Consider adding an integration smoke test that runs `buddd run triangle --frame 2 --log-level=warn --log-file=/tmp/test.log` and verifies the output file format, but this is explicitly deferred per contract.
- `FileSink::create()` could be refactored to construct the FileSink before opening, avoiding the double-open, but this is cosmetic.
- The `LogFilter::is_enabled()` inline could mark the `break` in the override loop with a specific comment that it's the "last match wins" behavior (it already has `break` but no "last match wins" comment).

---

## Questions for human

None.

---

## Recommendation

**Approved.** The implementation is complete, all acceptance criteria are covered, all 395 tests pass (including the compile-fail test), code review confirms proper decoupling from Error/Result, no third-party dependencies (beyond the POSIX `write(2)` explicitly required by contract), correct thread safety, proper MemorySink guarding, and correct CLI integration. No blocking issues found.

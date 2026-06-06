# Spec Review — Logging System (SPEC-021) — Re-review

## Review summary

This is a re-review after the spec-author addressed all 6 blocking issues from the previous review cycle. 

**All 6 blocking issues are resolved.** The spec:
1. Disambiguated `--log-filter` syntax by adopting `=` as separator (`--log-filter=Asset:ModelLoader=trace`) with prefix matching and optional level.
2. Defined MemorySink injection via `LogConfig` sinks vector, `Logger::init(LogConfig)`, and `Logger::reset()`, with a `ScopedMemoryLogger` RAII test helper.
3. Extended the `Sink` interface to receive `const LogMessage&` (which includes `file`, `line`, `function` fields), making AC-017 testable.
4. Specified build-type detection via `#ifndef NDEBUG` (Debug) / `NDEBUG` defined (Release).
5. Added a `## Documentation updates` section listing all affected files (CLI help, CLI parsing, ADR, wiki, business-rules, asset-manager spec).
6. Clarified the Logger lifecycle: `init()`/`shutdown()`/`reset()` contract, test isolation, thread-safety warnings (edge cases 11-12), and idempotent `init()` behavior.

The spec now satisfies all Definition of Ready criteria. No remaining blocking issues.

**Verdict: accepted** (ready for implementation contracting)

---

## Definition of Ready checklist

| # | Criterion | Status | Notes |
|---|---|---|---|
| **Clarity & Completeness** | | | |
| 1 | Scope clearly defined | ✅ | Goals (L16–28), Non-goals (L30–38), Out of scope (L404–413) provide clear boundaries. |
| 2 | Dependencies identified | ✅ | C++26 std library, CLI parsing infrastructure (Assumption 6, L421–422), NDEBUG for build-type detection (L80–81). |
| 3 | Edge cases and error conditions described | ✅ | 13 edge cases (L353–367) + 9 error cases (L369–378). Thorough coverage. |
| 4 | Behavior unambiguous and testable | ✅ | All AC items have clear verification methods; source location is testable via MemorySink + LogMessage struct. |
| **Verification** | | | |
| 5 | E2E verification defined | ✅ | E2E section (L270–277), testing strategy table (L344–351), 20 AC items with verification methods. |
| 6 | AC specific, measurable, verifiable | ✅ | All 20 AC items have concrete verification (unit tests, compile-time tests, integration, stress test). |
| 7 | Success and failure states described | ✅ | SC-001–004 (L279–286) + comprehensive error cases (L369–378). |
| **Documentation** | | | |
| 8 | Interface changes documented | ✅ | CLI flags table (L73–77), API reference (L85–164), macro signatures (L95–112), LogLevel enum, Sink interface, LogMessage struct. |
| 9 | Existing documentation updates listed | ✅ | `## Documentation updates` section (L380–389) lists 6 affected documents (CLI help, CLI parsing, ADR, wiki, business-rules, asset-manager spec). |
| **Technical** | | | |
| 10 | Technical constraints identified | ✅ | C++26 only, mutex-based sync, `std::format`, NDEBUG-based build-type detection. |
| 11 | Risks or unknowns surfaced | ✅ | Edge cases cover 13 risk scenarios; error cases cover 9 failure modes; open questions section is present (L426). |
| 12 | Performance implications noted | ✅ | SC-003 (< 5 ns overhead for disabled levels) and SC-004 (10K msgs < 500 ms) are quantified. |

---

## Blocking issues

Items that must be resolved before the artifact can be accepted.

**No blocking issues remain.** All 6 previously identified issues have been resolved:

- [x] **BLOCKING-1** (previous): `--log-filter` syntax ambiguity when tags contain colons — **RESOLVED**. Syntax changed to `--log-filter=<source-pattern>=<level>` with `=` separator (L77). Example: `--log-filter=Asset:ModelLoader=trace`. Prefix matching semantics defined (edge case 13, L367). Level is optional.

- [x] **BLOCKING-2** (previous): MemorySink injection API is undefined — **RESOLVED**. `LogConfig` now has `std::vector<std::shared_ptr<Sink>> sinks` (L51). `Logger::init(LogConfig)` accepts the config (L52). `Logger::reset()` provides test isolation (L52–53, L341). `ScopedMemoryLogger` test helper defined in `tests/log_helpers.h` (L151–165).

- [x] **BLOCKING-3** (previous): AC-017 (source location) is untestable — **RESOLVED**. `Sink::write()` now receives `const LogMessage&` (L46). `LogMessage` struct includes `file`, `line`, `function` fields (L140–149). MemorySink can expose these for test assertions.

- [x] **BLOCKING-4** (previous): Build-type detection mechanism not specified — **RESOLVED**. The spec now explicitly states: "Debug build (NDEBUG not defined): `debug` level minimum" and "Release build (NDEBUG defined): `warn` level minimum" (L79–81).

- [x] **BLOCKING-5** (previous): Documentation updates not listed — **RESOLVED**. `## Documentation updates` section (L380–389) lists 6 files: `src/cmd/main.cpp`, CLI parsing infrastructure, new ADR, new wiki page, `docs/wiki/domain/business-rules.md`, and `.specs/sprint-2026-06/asset-manager/spec.md` + implementation-contract.

- [x] **BLOCKING-6** (previous): Logger singleton test isolation ambiguous — **RESOLVED**. Initialization flow (L336–342) defines `init()` → `shutdown()` → `reset()` lifecycle. Edge case 6 (L360): `init()` is idempotent; use `reset()` before `init()` to change config. Edge case 11 (L365): logging after `shutdown()` is silently dropped. Edge case 12 (L366): `init()`/`shutdown()` are not thread-safe. `ScopedMemoryLogger` (L151–165) automates test setup/teardown.

---

## Warnings

Non-blocking concerns for awareness:

- **W-03 (AC-010 thread safety test reliability):** AC-010 requires spawning 4 threads logging 1000 messages each and verifying no interleaved output. This is timing-dependent — a buggy implementation may still pass if interleaving happens not to occur during the test run. Consider adding a stress test with atomic barriers or using a test that explicitly verifies the mutex is held per-message.

- **W-04 (`--log-level` with invalid level string — process exit coupling):** Edge case 9 (L363) says the engine should "print an error message to stderr and exit with a non-zero exit code". This couples the logging subsystem to process lifecycle management. Since `Logger::init()` is called from main, a more natural approach would be for `Logger::init()` to return a `Result<void>` on failure, letting the caller decide whether to exit.

- **W-06 (MemorySink thread safety):** The spec does not explicitly state whether `MemorySink` is thread-safe. Since it's used only in tests (typically single-threaded), this is acceptable, but documenting it as "not thread-safe, test-only" would avoid confusion.

- **W-07 (File sink append vs truncate):** Non-goal mentions "no log rotation or log file management beyond simple append" (L37), which implies append mode, but the file sink open mode is not explicitly stated in the file sink behavior section. Consider stating explicitly whether the file sink appends to or truncates an existing file.

- **W-08 (Empty source tag brackets):** Edge case 4 (L358) says `BUDDD_LOG_TAG("")` should produce empty `[]` brackets, but there is no corresponding AC item. Consider adding an AC for this edge case.

---

## Required changes

No changes required — all blocking issues are resolved. The spec is ready for implementation contracting.

## Suggested improvements

Optional ideas (not required):

- Consider adding a dedicated AC for empty source tag behavior (W-08).
- Consider defining a `Logger::init()` return type (`engine::Result<void>`) for graceful error handling (W-04).
- Document the file sink open mode explicitly (append vs truncate) (W-07).
- Document that `MemorySink` is not thread-safe (W-06).

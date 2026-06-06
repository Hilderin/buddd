# Workflow Coordination: logging-system

## Orchestrator

**Feature**: `logging-system`
**Status**: completed
**Current step**: done
**Initial instructions**: `Better logging system for the Buddd engine — replace ad-hoc std::cerr/printf/fprintf with a lightweight custom logger supporting levels, sinks (console, file, future in-editor), hierarchical source tags, CLI control (--log-level, --log-file, --log-filter), thread safety, and a test-friendly MemorySink. Framework only — no migration of existing code.`
**Notes**:
- Grill-me completed 2026-06-06.
- 5 log levels: trace (off by default, explicit via --log-level=trace), debug, info, warn, error.
- Debug build default: debug+. Release build default: warn+.
- Console format: `[LEVEL] [Source:Sub] message` (no timestamp).
- File format: timestamp ISO + `[LEVEL] [Source:Sub] message`.
- API: macros (BUDDD_LOG_INFO, etc.) capturing __FILE__/__LINE__/__FUNCTION__, with implicit per-file tag via BUDDD_LOG_TAG("Module:Sub").
- Tag mandatory per-file — compile error if missing.
- Thread safety via mutex.
- Header-only API, compiled backend (.cpp).
- std::format for formatting.
- No external dependencies (std only).
- MemorySink for unit tests.
- File sink failure → warn on raw stderr + continue without file.
- Decoupled from Error/Result<T>.
- ADR + wiki page required.
- Future feature: migrate existing cerr/printf calls.

## spec-author

**Status**: completed
**Summary**: Fixed all 6 blocking issues from spec-critic: (1) disambiguated `--log-filter` syntax to use `=` separator with prefix matching; (2) defined MemorySink injection via `LogConfig` struct + `Logger::init(LogConfig)` + `Logger::reset()` with `ScopedMemoryLogger` test helper; (3) extended `Sink` interface to receive `const LogMessage&` (includes file/line/function); (4) specified `NDEBUG`-based build-type detection for default thresholds; (5) added `## Documentation updates` section listing affected files; (6) documented init/shutdown lifecycle, test isolation contract, and thread-safety warnings. Also moved `source_location` to end of parameter list per W-05.
**Artifacts**:
- `.specs/sprint-2026-06/logging-system/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**: Re-review confirms all 6 blocking issues from the previous cycle are resolved. The spec now uses `=` separator for `--log-filter`, defines MemorySink injection via LogConfig + Logger::init()/reset(), extends Sink to receive LogMessage (with file/line/function), uses NDEBUG for build-type detection, lists documentation updates, and clarifies the Logger init/shutdown/reset lifecycle with test isolation. All Definition of Ready criteria are satisfied. No remaining blocking issues.
**Artifacts**:
- `.specs/sprint-2026-06/logging-system/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- **W-03**: AC-010 (thread safety stress test) is timing-dependent — consider a more deterministic test.
- **W-04**: Logger coupling to process exit for invalid `--log-level` is inappropriate; consider `Result<void>` return from `init()`.
- **W-06**: MemorySink thread safety not explicitly documented (acceptable for test-only).
- **W-07**: File sink append vs truncate mode not explicitly stated.
- **W-08**: No AC for empty source tag behavior (edge case 4).
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**: Fixed blocking issue B-01: replaced public `FileSink` constructor with `static auto FileSink::create(std::string_view) -> std::unique_ptr<FileSink>` factory (returns nullptr on failure). Updated section 5 (factory semantics, caller must check return value, Logger::init stores valid sinks only), section 1 (FileSink class declaration), and section 8 (conditional addition in app_config.cpp). Also addressed non-blocking warnings: documented why LogMessage::message is `std::string` (W-01), moved `LogFilter` to forward-declaration in log.h (W-03), and inlined MemorySink method bodies (W-05).
**Artifacts**:
- `.specs/sprint-2026-06/logging-system/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**: Re-review confirms B-01 is resolved. FileSink now uses `static auto create() -> std::unique_ptr<FileSink>` factory (returns nullptr on failure), private constructor, conditional addition in app_config.cpp. Logger::init() stores pre-validated sinks. W-01, W-03, W-05 were addressed; W-04 (hard-coded /tmp/test.log) and W-06 (flag position flexibility undocumented) remain as non-blocking warnings.
**Artifacts**:
- `.specs/sprint-2026-06/logging-system/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- W-04: Integration test still uses hard-coded `/tmp/test.log` instead of `temp_filename()` helper, risking collisions in parallel test execution.
- W-06: Logging flag position flexibility relative to subcommands is not documented in section 8.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-06 10:42:52
**Warnings**:
<none>
**Notes**: All specs, contracts, and reviews approved. Proceed to implementation.

## code-implementer

**Status**: completed
**Summary**: Implemented the Buddd Engine logging system (SPEC-021 / IMPL-021). Created all 11 new files under src/engine/log/ (log.h, log_filter.h/.cpp, logger.h/.cpp, console_sink.h/.cpp, file_sink.h/.cpp, memory_sink.h), added CLI parsing for --log-level, --log-file, --log-filter flags in src/cmd/app_config.h/.cpp, wired Logger::init() into src/cmd/main.cpp, created tests/log_helpers.h (ScopedMemoryLogger) and tests/logging_tests.cpp (28 test cases covering all 25 required tests), and added compile-fail test tests/compile_fail/missing_tag.cpp with CMake try_compile. All 395 tests pass including logging tests and compile-fail verification.
**Artifacts**:
- src/engine/log/log.h (new)
- src/engine/log/log_filter.h (new)
- src/engine/log/log_filter.cpp (new)
- src/engine/log/logger.h (new)
- src/engine/log/logger.cpp (new)
- src/engine/log/console_sink.h (new)
- src/engine/log/console_sink.cpp (new)
- src/engine/log/file_sink.h (new)
- src/engine/log/file_sink.cpp (new)
- src/engine/log/memory_sink.h (new)
- tests/log_helpers.h (new)
- tests/logging_tests.cpp (new)
- tests/compile_fail/missing_tag.cpp (new)
- src/cmd/app_config.h (modified)
- src/cmd/app_config.cpp (modified)
- src/cmd/main.cpp (modified)
- tests/CMakeLists.txt (modified)
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**: Implementation approved. All 20 acceptance criteria are implemented and tested. 395/395 tests pass (28 logging test cases + 1 compile-fail test). Code review confirms correct API signatures, thread safety, MemorySink guarding, FileSink factory pattern, NDEBUG defaults, no Error/Result dependency, and no third-party includes (beyond POSIX `unistd.h` for `write(2)` mandated by contract).
**Artifacts**:
- `.specs/sprint-2026-06/logging-system/code-review.md`
**Questions for human**:
none
**Warnings**:
- W-01: `<unistd.h>` in file_sink.cpp is POSIX, not C++26 stdlib, but mandated by contract for raw `write(2)` stderr warning.
- W-02: FileSink opens file twice during create (test Impl + constructor Impl). Minor inefficiency.
- W-03: Contract has off-by-one typos in prefix lengths, implementation uses correct lengths.
- W-04: No process-level CLI integration test (explicitly acceptable to skip per contract).
- W-05: MemorySink not thread-safe (documented, test-only).
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**: Created ADR-020 documenting the logging system architectural decisions: custom logger over spdlog, macro API with BUDDD_LOG_TAG, singleton Logger, Sink interface, thread safety strategy, zero external dependencies, and decoupling from Error/Result<T>.
**Artifacts**:
- `docs/adr/ADR-020-custom-logging-system.md`
**Questions for human**:
<none>
**Warnings**:
<none>
**Blocking issues**:
<none>

## wiki-agent

**Status**: completed
**Summary**: Created comprehensive logging system wiki page (`docs/wiki/domain/logging.md`) covering quick start, log levels, source tag conventions, macro API reference, CLI flag reference, sink behaviour, thread safety guarantees, and best practices. Updated `docs/wiki/domain/business-rules.md` to document the new structured logger, CLI flags (`--log-level`, `--log-file`, `--log-filter`), and console/file output formats in the observability section. Updated `docs/wiki/architecture/module-map.md` to add the `log/` submodule with all 10 files under `src/engine/log/`.
**Artifacts**:
- `docs/wiki/domain/logging.md` (created)
- `docs/wiki/domain/business-rules.md` (modified)
- `docs/wiki/architecture/module-map.md` (modified)
**Changes made**:
- Created `docs/wiki/domain/logging.md` — full API reference, tag naming conventions, macro usage guidelines, CLI flag reference with examples, sink behaviour details, thread safety documentation, and best practices
- Added "Structured logging (new system as of SPEC-021)" subsection to `docs/wiki/domain/business-rules.md` under "Observability messages" — documents the structured logger, CLI flags, and output formats
- Added `Log submodule (log/)` section to `docs/wiki/architecture/module-map.md` in the `buddd_engine` section — lists all 10 files in `src/engine/log/` with their roles
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**: Cross-document governance validation completed. All artifacts are coherent: spec matches human intent, contract matches spec, code matches contract, tests prove acceptance criteria. ADR-020 exists and is consistent. Wiki pages reflect current state. All workflow gates completed successfully with no blocking issues. Verdict: approved.
**Artifacts**:
- `.specs/sprint-2026-06/logging-system/governance-review.md`
**Questions for human**:
none
**Warnings**:
- W-03 (spec-critic): AC-010 thread safety stress test is timing-dependent.
- W-04 (spec-critic): `--log-level` invalid value causes process exit — lifecycle coupling.
- W-04 (contract-critic): Integration test uses hard-coded `/tmp/test.log` instead of `temp_filename()`.
- W-06 (contract-critic): Logging flag position flexibility relative to subcommands undocumented.
- W-02 (contract-critic): Tag truncation uses raw `fprintf(stderr)` instead of the logging API (spec deviation, justified by recursion avoidance).
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.

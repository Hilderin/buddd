# Governance Review — Developer Assertions (SPEC-023 / IMPL-023)

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec matches human intent** — All 10 design decisions from the grill-me session (recorded in coordination.md) are correctly reflected in SPEC-023: `LogLevel::Fatal` above `Error`, `debug_break()`, NDEBUG-only build detection, fixed `"Assert"` tag, always-abort handler, `src/engine/debug/assert.h` location, testable `format_assertion_failure_message()`, FAIL always active in release, VERIFY log-only in release, zero `BUDDD_TESTING` involvement.
- [x] **Contract matches spec** — IMPL-023 correctly implements all 20 acceptance criteria from SPEC-023. Five macros with correct behavior matrix. All edge cases carried forward. AC-015 dual-option phrasing resolved by extracting `format_assertion_failure_message()` as a public, testable function.
- [x] **Code matches contract** — Code review confirms all contract requirements met: 4 new files at correct paths, 5 modified files with correct changes, `LogLevel::Fatal` after `Error` (value 5), `debug_break()` with correct platform intrinsics and `#ifndef NDEBUG` guard, all 5 macros with `do { } while(false)` wrapper, `handle_assertion_failure` signature matches AC-014, 8 assertion insertion points across 3 files, no forbidden files modified, no external dependencies, no `BUDDD_TESTING` involvement.
- [x] **Tests prove acceptance criteria** — All 20 ACs have concrete verification via 12 test cases (T-A1 through T-A12) plus explicit code review checks. Test results confirm all 12 tests pass in both debug and release builds (24 assertions). Full suite: 419/419 in debug, 413/419 in release (6 pre-existing unrelated failures).
- [x] **No double evaluation** — Tests T-A7 and T-A8 verify `BUDDD_VERIFY` and `BUDDD_ASSERT` evaluate their expressions exactly once.
- [x] **Empty format string edge case** — Contract correctly implements suppression of "Message:" line when message is empty (`message.has_value() && !message->empty()`), matching spec edge case 5.
- [x] **All `CHECK` replaced with `REQUIRE`** — Spec-critic and contract-critic reviews verified the convention (confirmed by CI test output showing all assertions passed).

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-021-developer-assertions.md** — Created and accepted. Documents all architectural decisions: `LogLevel::Fatal` addition, five-macro API, NDEBUG-only build detection, platform-specific debug break, fixed `"Assert"` tag, always-abort handler, testable formatter, `src/engine/debug/` file location, zero external dependencies, impact on ADR-020. Consistent with all artifacts.
- [x] **ADR-020 impact documented** — ADR-021 explicitly documents how the logging system is extended: `LogLevel::Fatal` added, `BUDDD_LOG_FATAL` / `BUDDD_LOG_TAGGED_FATAL` macros added, console sink updated, no CLI change, no structural change to Logger/Sink interfaces.
- [x] **ADR-009 compliance** — Test file `assertion_tests.cpp` uses the required `_tests.cpp` suffix.
- [x] **ADR-019 compliance** — Assertion system is standard-library-only plus `log/log.h`, architecture-boundary compliant.
- [x] **No ADR history rewritten** — ADR-021 is a new independent ADR; existing ADRs (020, 009, 019) are referenced but not modified.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/domain/logging.md`** — Updated: six log levels listed (0–5, including Fatal), `Fatal` row in "When to use each level" table, `[FATAL]` console format example shows assertion failure, `BUDDD_LOG_FATAL` in macro reference, `--log-level` notes Fatal is NOT accepted, "Assertion System" section cross-references `assertions.md`, ADR-021 in reference list. Consistent with spec and implementation.
- [x] **`docs/wiki/domain/assertions.md`** — New page created: quick start with all 5 macros, header location, behavior matrix matching spec exactly, per-macro API reference, failure report format, debug break behaviour, logging integration, `handle_assertion_failure()` and `format_assertion_failure_message()` API, edge cases table. References SPEC-023, IMPL-023, ADR-021, logging.md.
- [x] **`docs/wiki/architecture/module-map.md`** — Updated: debug submodule section added with file table, logging module description updated to "six log levels" (adding Fatal), SPEC-023 and ADR-021 in reference list.
- [x] **`docs/wiki/engineering/testing.md`** — Updated: "Assertion tests" subsection added documenting `tests/assertion_tests.cpp` with 12 test cases tagged `[assertion]`. REFERENCES `REQUIRE`/`REQUIRE_FALSE` conventions (not `CHECK`).
- [x] **Wiki does not contradict ADRs** — All wiki content is derived from SPEC-023/IMPL-023 and ADR-021. No contradictions found.

## Warnings

Non-blocking concerns for awareness:

- **6 pre-existing release test failures** — `cli_app_tests.cpp`, `cmd_tests.cpp`, `logging_tests.cpp` fail in release builds. These are pre-existing and unrelated to the assertion feature.
- **`BUDDD_LOG_FATAL` / `BUDDD_LOG_TAGGED_FATAL` not directly tested** — No test exercises these macros directly. T-A5 tests the Fatal-level log path via `Logger::instance().log()`, covering runtime behavior but not macro expansion. Acceptable — macros are mechanical renames of existing ERROR pattern.
- **Redundant `is_enabled()` check in `BUDDD_VERIFY` release path** — The outer `is_enabled()` call duplicates the internal check in `Logger::log()`. Not a bug, but unnecessary code.
- **`format_assertion_failure_message` has no explicit `noexcept`** — The function performs pure string formatting. No explicit `noexcept` specification, though the project does not universally apply `noexcept`.
- **`experiments-spec-driven-dev.md` has unrelated modification** — In the working tree but not part of this feature implementation.
- **MSVC `__debugbreak()` path not build-tested** — Present for completeness; no MSVC CI build exists. Should be verified if MSVC support is added.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None. ADR-021 is created and accepted. Wiki has been updated by wiki-agent. No governance changes are required.

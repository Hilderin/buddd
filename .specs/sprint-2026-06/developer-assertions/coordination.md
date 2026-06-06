# Workflow Coordination: developer-assertions

## Orchestrator

**Feature**: `developer-assertions`
**Status**: completed
**Current step**: completed
**Initial instructions**: Implement a lightweight assertion system for the Buddd Engine that checks engine invariants in debug/development builds, logs failures through the existing logging system, can trigger a debugger break, can be compiled out in release builds, and provides clear macros (BUDDD_ASSERT, BUDDD_ASSERT_MSG, BUDDD_VERIFY, BUDDD_FAIL, BUDDD_FAIL_MSG).
**Notes**:
- Design decisions settled during grill-me (2026-06-06):
  - **Fatal log level**: Add `LogLevel::Fatal` above `Error` (log.h enum extension)
  - **Debug break function**: `buddd::engine::debug_break()` in `src/engine/debug/debug_break.h`
  - **Build detection**: `NDEBUG` only (no extra CMake flags)
  - **Assertion tag**: Fixed `"Assert"` tag hardcoded in assertion macros
  - **Abort**: Always abort in debug builds after log + break
  - **Header location**: `src/engine/debug/assert.h` (+ `debug_break.h`)
  - **Testing**: Test internal handle_assertion_failure() formatting, BUDDD_VERIFY evaluation, non-double-evaluation of BUDDD_ASSERT. No BUDDD_TESTING involvement in assert behavior.
  - **FAIL in release**: Always active — log Fatal + abort
  - **VERIFY in release**: Log Fatal but no break/abort

## spec-author

**Status**: completed
**Summary**:
Created SPEC-023 spec.md for developer assertions. Defines the assertion system API (BUDDD_ASSERT, BUDDD_ASSERT_MSG, BUDDD_VERIFY, BUDDD_FAIL, BUDDD_FAIL_MSG), their debug/release behavior matrix, the addition of LogLevel::Fatal to the logging system, the debug_break() function, and the handle_assertion_failure() formatting and reporting function. Covers 20 acceptance criteria, user stories, edge cases, error cases, and documentation update requirements. All design decisions from the grill-me session are incorporated.
**Artifacts**:
- `.specs/sprint-2026-06/developer-assertions/spec.md`
**Questions for human**:
none
**Warnings**:
- The spec references `docs/wiki/domain/logging.md` for documentation updates; that page currently lists 5 log levels and will need a `Fatal` row added.
- No blocking issues identified — spec is complete and self-validated against Definition of Ready criteria.
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
All 12 Definition of Ready criteria are satisfied. The spec is clear, complete, and unambiguous. No blocking issues found. The five-macro API, NDEBUG-only behavior matrix, LogLevel::Fatal addition, testing strategy, and documentation requirements are all well-defined. The spec is accepted and ready for implementation contract authoring.
**Artifacts**:
- `.specs/sprint-2026-06/developer-assertions/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- AC-015 has dual-option phrasing ("test a string-building helper instead, or invoke in a test-safe path") that the implementation-contract-author should resolve to a single approach.
- MSVC `__debugbreak()` path (AC-005) is present for completeness but not build-tested; should be re-verified if MSVC support is added.
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Loop 1: Fixed 2 blocking issues from contract-critic review. (1) Updated `format_assertion_failure_message` to suppress the "Message:" line when the formatted message is empty (`message.has_value() && !message->empty()`), matching spec edge case 5. Updated edge case 5 description to reflect correct behavior. (2) Replaced all 20 `CHECK(...)` with `REQUIRE(...)` in the test file code block. Updated the convention table row to reference the wiki convention (`docs/wiki/engineering/testing.md`) with `REQUIRE` (not `CHECK`). Added a note that `src/engine/debug/` directory must be created by the code agent.
**Artifacts**:
- `.specs/sprint-2026-06/developer-assertions/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
- The `format_assertion_failure_message` format intentionally omits a trailing `\n` in the message body (the console sink appends it). Any test that checks the exact string must account for this.
- The `Fatal` level is NOT added to the CLI `--log-level` parser. Users cannot specify `--log-level=fatal`. This is consistent with the spec's non-goals (no external exposure of the Fatal level as a CLI option).
- AC-015's dual-option phrasing is resolved by extracting `format_assertion_failure_message()` as a public, testable function. Tests call this directly and never invoke `handle_assertion_failure()` (which aborts).
- The `__debugbreak()` MSVC path (AC-005) is present for completeness. No MSVC CI build exists to verify it; should be re-verified if MSVC support is added.
- `src/engine/debug/` directory does not exist yet — must be created by the code agent.
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Both blocking issues resolved and verified. (1) Empty format string edge case: `format_assertion_failure_message` now suppresses the "Message:" line when message is empty via `!message->empty()` guard. (2) All `CHECK` replaced with `REQUIRE` in test file; convention table corrected. No new issues found. Contract accepted.
**Artifacts**:
- `.specs/sprint-2026-06/developer-assertions/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- `src/engine/debug/` directory does not exist yet — must be created by the code agent.
- Redundant `is_enabled()` check in `BUDDD_VERIFY` release path (outer check duplicates `Logger::log()` internal check).
- No test for `BUDDD_LOG_FATAL` / `BUDDD_LOG_TAGGED_FATAL` macros (compile-coverage would be beneficial).
- `format_assertion_failure_message` has no explicit `noexcept` specification.
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Hilderin
**Date**: 2026-06-06 15:42:07
**Warnings**:
<none>
**Notes**: All specs, contracts, and reviews approved. Proceed to implementation.

## code-implementer

**Status**: completed
**Summary**:
Implemented the full Developer Assertions system per IMPL-023 contract: created `src/engine/debug/` with `debug_break.h`, `assert.h`, and `assert.cpp`; added `LogLevel::Fatal` and FATAL log macros to `src/engine/log/log.h`; added `[FATAL]` rendering to `console_sink.cpp`; inserted 8 assertion points across `engine_service.cpp` (4), `entity.cpp` (1), and `render_device_opengl.cpp` (2 default cases with BUDDD_FAIL_MSG). Created `tests/assertion_tests.cpp` with 12 test cases covering all acceptance criteria. All 419 tests pass in debug, assertion tests pass in release.
**Artifacts**:
- Created: `src/engine/debug/debug_break.h`, `src/engine/debug/assert.h`, `src/engine/debug/assert.cpp`, `tests/assertion_tests.cpp`
- Modified: `src/engine/log/log.h`, `src/engine/log/console_sink.cpp`, `src/engine/engine_service.cpp`, `src/engine/scene/entity.cpp`, `src/engine/render/render_device_opengl.cpp`
**Questions for human**:
none
**Warnings**:
- Release build shows 6 pre-existing test failures in `cli_app_tests.cpp`, `cmd_tests.cpp`, and `logging_tests.cpp` (display-related and file-permission tests) — unrelated to assertion changes.
- `tests/log_helpers.h` requires `#include "log/memory_sink.h"` to be included before it (by the consumer) since it references `buddd::log::MemorySink` without including it — same pattern as `logging_tests.cpp`.
**Blocking issues**:
none

## code-reviewer

**Status**: completed
**Summary**:
Code review complete. All 20 acceptance criteria satisfied. Feature builds cleanly in debug and release. All 12 assertion test cases pass in both builds (24 assertions). Full debug suite passes 419/419. Release has 6 pre-existing unrelated failures. No blocking issues found. Eight assertion insertion points added across 3 files. Code matches spec, contract, and project conventions.
**Artifacts**:
- `.specs/sprint-2026-06/developer-assertions/code-review.md`
**Questions for human**:
none
**Warnings**:
- 6 pre-existing release test failures in `cli_app_tests.cpp`, `cmd_tests.cpp`, `logging_tests.cpp` — unrelated to assertion changes.
- `BUDDD_LOG_FATAL` / `BUDDD_LOG_TAGGED_FATAL` macros not directly tested (only the log path via T-A5). Acceptable — mechanical renames of existing ERROR pattern.
- `experiments-spec-driven-dev.md` has an unrelated modification in the working tree.
**Blocking issues**:
none

## adr-agent

**Status**: completed
**Summary**:
Created ADR-021-developer-assertions.md documenting all architectural decisions of the assertion system: LogLevel::Fatal addition, five-macro API (BUDDD_ASSERT, BUDDD_ASSERT_MSG, BUDDD_VERIFY, BUDDD_FAIL, BUDDD_FAIL_MSG), NDEBUG-only build detection, platform-specific debug break via __builtin_trap()/__debugbreak(), fixed "Assert" tag, always-abort handler, testable format_assertion_failure_message(), file location in src/engine/debug/, zero external dependencies, and impact on ADR-020.
**Artifacts**:
- `docs/adr/ADR-021-developer-assertions.md`
**Questions for human**:
none
**Warnings**:
- The logging wiki page (docs/wiki/domain/logging.md) still lists 5 log levels — it will need a `Fatal` row added when the wiki-agent updates it.
- ADR-020 (logging system) is extended by this ADR with LogLevel::Fatal; no structural changes to ADR-020's design are required.
**Blocking issues**:
none

## wiki-agent

**Status**: completed
**Summary**:
Updated the operational wiki to reflect the Developer Assertions feature (SPEC-023/IMPL-023). Modified `docs/wiki/domain/logging.md` to add `LogLevel::Fatal` to the log levels table, console format, macro reference, and CLI documentation. Created `docs/wiki/domain/assertions.md` as a new page documenting the full assertion system API. Added `debug/` submodule to `docs/wiki/architecture/module-map.md`. Added assertion test file reference to `docs/wiki/engineering/testing.md`.
**Artifacts**:
- Created: `docs/wiki/domain/assertions.md`
- Modified: `docs/wiki/domain/logging.md`, `docs/wiki/architecture/module-map.md`, `docs/wiki/engineering/testing.md`
**Changes made**:
- **`logging.md`**: Added Fatal (level 5) to log levels table with description; added Fatal row to "When to use each level" table; added `BUDDD_LOG_FATAL` to macro reference; updated tag override section to say "six levels"; added `[FATAL]` example to console sink format; updated `--log-level` CLI flag docs to note Fatal is not accepted; added note that Fatal is always enabled and cannot be suppressed; added "Assertion System" section cross-referencing the new assertions page; added ADR-021 to reference list.
- **`assertions.md` (new)**: Full documentation covering quick start, header location (`#include "debug/assert.h"`), behavior matrix for all 5 macros (debug vs release), API reference for each macro, failure report format, debug break behaviour, logging integration, `handle_assertion_failure()` and `format_assertion_failure_message()` API, testing notes, edge cases table, and references to SPEC-023/IMPL-023/ADR-021.
- **`module-map.md`**: Updated logging module description from "five log levels" to "six log levels" (adding Fatal); added new "Debug submodule (debug/)" section with file table for `debug_break.h`, `assert.h`, `assert.cpp`; added SPEC-023 and ADR-021 to the reference list.
- **`testing.md`**: Added "Assertion tests" subsection documenting `tests/assertion_tests.cpp` tagged `[assertion]` with 12 test cases.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: completed
**Summary**:
Cross-document governance validation complete. Spec matches human intent (all 10 grill-me decisions reflected). Contract matches spec (20 ACs fully covered). Code matches contract (all files, signatures, macros, insertion points verified). Tests prove all acceptance criteria (12 test cases, all pass in debug and release). ADR-021 is created and consistent with all artifacts. Wiki updated to reflect current state. No blocking issues found. Feature passes governance review.
**Artifacts**:
- `.specs/sprint-2026-06/developer-assertions/governance-review.md`
**Questions for human**:
none
**Warnings**:
- 6 pre-existing release test failures in `cli_app_tests.cpp`, `cmd_tests.cpp`, `logging_tests.cpp` — unrelated to assertion changes.
- `BUDDD_LOG_FATAL` / `BUDDD_LOG_TAGGED_FATAL` macros not directly tested (only the log path via T-A5). Acceptable — mechanical renames of existing ERROR pattern.
- Redundant `is_enabled()` check in `BUDDD_VERIFY` release path (duplicates `Logger::log()` internal check).
- `format_assertion_failure_message` has no explicit `noexcept` specification.
- `experiments-spec-driven-dev.md` has unrelated modification in working tree.
- MSVC `__debugbreak()` path present for completeness but not build-tested.
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

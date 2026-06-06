# Implementation Contract Review — IMPL-023 Developer Assertions

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Contradiction with spec edge case 5 (empty format string)**: **RESOLVED** — The `format_assertion_failure_message` implementation now guards with `if (message.has_value() && !message->empty())`, suppressing the "Message:" line when the message is empty. Edge case 5 description in the contract now matches the spec: "the `"Message:"` line is suppressed entirely."

- [x] **Wiki convention violated: `CHECK` used instead of `REQUIRE`/`REQUIRE_FALSE`**: **RESOLVED** — All `CHECK(...)` have been replaced with `REQUIRE(...)` in the test file. The convention table now correctly references `docs/wiki/engineering/testing.md` with `TEST_CASE`, `REQUIRE`, `SECTION` (not `CHECK`). Verified by grep — zero `CHECK(` calls remain in the contract.

## Warnings

Non-blocking concerns for awareness:

- **`src/engine/debug/` directory does not exist**: The contract assumes the `src/engine/debug/` directory exists but it does not. The code agent will need to create it when writing new files. The contract should explicitly mention this.

- **Redundant `is_enabled()` check in `BUDDD_VERIFY` release path**: The release path for `BUDDD_VERIFY` (§257–271) checks `is_enabled()` before calling `Logger::instance().log()`, but `Logger::log()` internally also calls `is_enabled()` (line 90 of `log.h`), making the outer check redundant. This is not a bug — just a minor double-check that adds no value. Consider removing the outer check or keeping it as a documentation hint.

- **No test for `BUDDD_LOG_FATAL` / `BUDDD_LOG_TAGGED_FATAL` macros**: The contract adds `BUDDD_LOG_FATAL` and `BUDDD_LOG_TAGGED_FATAL` to `log.h` but no test exercises these macros directly. T-A5 tests `Logger::instance().log()` directly at Fatal level, which covers the log path but not the macro expansion. Acceptable since the macros are mechanical renames of existing patterns, but a compile-coverage test would be beneficial.

- **`format_assertion_failure_message` has no explicit `noexcept` specification**: The function does pure string formatting with no side effects. Given the project's style (some functions use `noexcept`), consider whether this should be `noexcept`. Not blocking — the existing project does not universally apply `noexcept`.

## Required changes — all resolved ✓

All four requested changes from the previous review have been implemented and verified:

1. **✓** Empty format string edge case fixed — `format_assertion_failure_message` now suppresses "Message:" line when message is empty.
2. **✓** All `CHECK` replaced with `REQUIRE` in test file (verified by grep — zero `CHECK(` calls).
3. **✓** Convention table updated — references `docs/wiki/engineering/testing.md` with `REQUIRE` (not `CHECK`).
4. **✓** Contract now includes a note that `src/engine/debug/` directory does not exist and must be created by the Code Agent.

## Suggested improvements

Optional ideas (not required):

- Consider adding a test for `BUDDD_LOG_FATAL` macro compilation (simple compile check).
- Consider adding `assert.cpp` as an explicit dependency in a comment in `src/engine/CMakeLists.txt` if `file(GLOB_RECURSE)` ever changes — currently auto-discovery is relied upon, which is brittle.

# Spec Review — SPEC-023 Developer Assertions

## Review summary

The spec is well-structured, complete, and unambiguous. It defines a clear assertion API (five macros), a precise debug/release behavior matrix governed by NDEBUG only, the addition of `LogLevel::Fatal` to the logging system, and a testable `handle_assertion_failure()` function. All 12 Definition of Ready criteria are satisfied. No blocking issues found. The spec is ready for implementation contract authoring.

---

## Definition of Ready assessment

### Clarity & Completeness

- [x] **Scope is clearly defined** — Included macros and components are listed; non-goals (§26–36) and out-of-scope (§243–255) sections draw clear boundaries.
- [x] **Dependencies are identified** — Depends on `buddd::log::Logger` (SPEC-021), C++26 standard library, compiler intrinsics (`__builtin_trap`/`__debugbreak`). LogLevel::Fatal is an additive change to the existing enum.
- [x] **Edge cases and error conditions are described** — 8 edge cases (§208–217) cover uninitialized logger, noexcept functions, static init ordering, empty format strings, exception-in-expression, etc. Error cases table (§219–227) covers all 5 failure scenarios.
- [x] **Expected behavior is unambiguous and testable** — Behavior matrix per macro × build type is explicit. Every AC has a concrete verification approach. Testing strategy (§185–197) explains how abort-incompatible paths are tested indirectly.

### Verification

- [x] **E2E verification is defined** — Unit test suite, compile-time release verification, ScopedMemoryLogger integration are all specified (§185–197).
- [x] **Acceptance criteria are specific, measurable, and verifiable** — 20 ACs (AC-001 through AC-020) each have a clear verification method (unit test, code review, compile check).
- [x] **Success and failure states are described** — Success criteria (§199–206) define 4 measurable metrics (CI pass, zero branch instructions, dead-stripping, <1ms abort path). Error cases (§219–227) describe failure behavior for every macro.

### Documentation

- [x] **Interface changes are documented** — `LogLevel::Fatal` enum addition, `handle_assertion_failure()` full signature (AC-014), five macro APIs, `debug_break()` declaration, all fully specified.
- [x] **Existing docs to update are listed** — `src/engine/log/log.h`, `docs/wiki/domain/logging.md`, new ADR, new wiki page/section (§258–265).

### Technical

- [x] **Technical constraints are identified** — NDEBUG-only build detection, compiler intrinsics for debug break, `do { } while(false)` macro safety, C++26-only dependencies, `std::optional<std::string>` (C++17+).
- [x] **Risks or unknowns are surfaced** — Logger unavailability during static init (edge case 8), noexcept interaction (edge case 7), open questions confirms all settled. No unresolved risks.
- [x] **Performance implications are noted** — Zero overhead in release for ASSERT/ASSERT_MSG, VERIFY preserves expression evaluation cost, FAIL always active. SC-002/SC-003 explicitly track codegen quality.

---

## Blocking issues

No blocking issues found.

---

## Warnings

- **AC-015 precision**: The acceptance criterion says "test a string-building helper instead, or invoke in a test-safe path where abort is replaced." This describes two alternative approaches. The implementation contract will need to pick one unambiguous strategy. The spec itself is clear enough — the formatting logic must be testable without abort — but the dual-option phrasing in the AC may lead to ambiguity during implementation. The implementation-contract-author should resolve this.
- **MSVC debug_break path (AC-005)**: The spec correctly notes no MSVC build is available and the branch is "present for completeness." This is fine, but the MSVC intrinsic `__debugbreak()` produces a different codegen pattern than `__builtin_trap()`. If MSVC support is ever added, the behavior should be re-verified.

---

## Required changes

None.

---

## Suggested improvements

- Consider adding a brief example of how `ScopedMemoryLogger` is used to capture Fatal-level output in the test section (§185–197), for clarity to future test authors.
- The `handle_assertion_failure()` signature in AC-014 uses `std::optional<std::string>` — consider whether `std::string_view` could be used for the optional message to avoid a heap allocation in the failure path (minor; not blocking).

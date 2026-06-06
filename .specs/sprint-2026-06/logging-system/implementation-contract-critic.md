# Implementation Contract Review — Logging System (Re-review)

## Review summary

The implementation contract (IMPL-021) was updated to fix the one blocking issue (B-01) from the previous review. The `FileSink` factory pattern is now properly defined: `FileSink::create()` returns `std::unique_ptr<FileSink>` (nullptr on failure), the constructor is private, and `app_config.cpp` conditionally adds only non-null sinks. The contract is now consistent across sections 1, 5, and 8 regarding failure handling. Of the six non-blocking warnings, four were addressed (W-01, W-03, W-05, and W-02 carries forward as a minor note), while two remain unaddressed (W-04 — integration test still uses hard-coded `/tmp/test.log` instead of `temp_filename()`; W-06 — logging flag position flexibility undocumented). These are non-blocking and do not prevent workflow progression.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **B-01 — FileSink failure communication is undefined.** *RESOLVED.* The contract now defines `FileSink::create(std::string_view) -> std::unique_ptr<FileSink>` returning nullptr on failure, a private constructor, a raw stderr warning on open failure, conditional addition in `app_config.cpp` (only if `create()` succeeds), and `Logger::init()` storing pre-validated sinks unconditionally. Sections 1, 5, and 8 are now consistent.

## Warnings

Non-blocking concerns for awareness:

- [x] **W-01 — `LogMessage::message` type documented.** *RESOLVED.* The field is `std::string` with an inline comment explaining why (MemorySink ownership).

- [ ] **W-02 — Tag truncation warning uses raw `fprintf(stderr)`.** *Carried forward.* The contract (section 13) uses `fprintf` to stderr for the tag truncation warning instead of the logging API. The spec expected logging-API usage with tag `[Log]`. The contract justifies this as recursion avoidance. This is pragmatically sound but is a spec deviation.

- [x] **W-03 — `LogFilter` forward-declared in public `log.h`.** *RESOLVED.* `LogFilter` is now forward-declared in `log.h` (line 176) with the full declaration in `log_filter.h`.

- [ ] **W-04 — Integration test uses hard-coded `/tmp/test.log`.** *NOT resolved.* The integration test section (lines 548–551) still uses `/tmp/test.log`. The existing `temp_filename()` helper (in `tests/test_helpers.h`) should be used instead to avoid collisions in parallel test execution. Not blocking (the test can be skipped if CI restricts filesystem, as noted).

- [x] **W-05 — MemorySink methods are inline.** *RESOLVED.* All `MemorySink` methods now have in-class definitions, avoiding ODR issues.

- [ ] **W-06 — Logging flag position flexibility undocumented.** *NOT resolved.* The contract still says `parse_logging_args(argc, argv, 1)` at "the very top of `main()`" without documenting that logging flags may appear anywhere in argv (before, after, or interspersed with subcommands). This works because unknown flags are silently ignored, but the flexibility is subtle and worth documenting. Not blocking.

## Required changes

No remaining blocking issues. All previous blocking issues are resolved.

## Suggested improvements

Optional ideas (not required):

- Replace `/tmp/test.log` with `temp_filename("buddd_log_")` in the integration test section.
- Add a brief note in section 8 documenting that logging flags are extracted position-independently.
- In section 13, explicitly call out the spec expectation and the recursion-avoidance justification for using `fprintf` instead of the logging API.

## Questions for the human

none

## Recommendation

**Approved.** The single blocking issue (B-01) is resolved. The two remaining unaddressed warnings (W-04, W-06) are non-blocking and the contract is ready for implementation.

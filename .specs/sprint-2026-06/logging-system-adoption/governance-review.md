# Governance Review — Logging System Adoption (SPEC-022)

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec claims `main.cpp` already has `BUDDD_LOG_TAG` (spec line 163) → contract correctly notes it does NOT.** Spec inaccuracy: `main.cpp` and all other 31 files required new `BUDDD_LOG_TAG` declarations. The contract correctly handled this by adding tags to all files. This is a minor spec defect that did not affect implementation correctness.

- [x] **Contract specifies `BUDDD_LOG_TAG("Asset")` in `asset_manager.tpp`** → implementer correctly reversed to `asset_manager.cpp` with `BUDDD_LOG_TAGGED_DEBUG` in `.tpp`. The contract's placement would have caused ODR violations when `asset_manager.h` (which includes `.tpp`) is `#include`d by command files that declare their own tags. The implementer's correction is architecturally correct. Documented in code review as a necessary contract correction.

- [x] **Contract only lists `tests/cmd_tests.cpp` as allowed test file** → implementer also modified `tests/cli_app_tests.cpp` and `tests/scene_rendering_tests.cpp`. These changes were necessary and correct: `cli_app_tests.cpp` had 4 assertion updates for `[ERROR] [App]` format, and `scene_rendering_tests.cpp` replaced `std::cerr.rdbuf()` capture with `MemorySink`. Without these changes, 407 tests would not pass. The contract was incomplete, and the implementer correctly identified the gap.

No unresolved cross-document contradictions. The three deviations above are documented, explained, and represent pragmatically correct corrections to spec/contract flaws.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-020 (Custom Logging System)** — Fully respected. Migration uses the `BUDDD_LOG_*` macro API, mandatory per-file `BUDDD_LOG_TAG` convention, `std::format`-style `{}` placeholders, and `ConsoleSink` output format `[LEVEL] [Tag] message\n`. No logging system implementation files were modified. ADR-020's "Consequences — Negative" has been updated by the wiki-agent to reflect migration completion ("The transitional period with two logging approaches is resolved").

- [x] **ADR-014 (CLI App System)** — Fully respected. CLI flag parsing in `src/cmd/` is preserved. `main.cpp` pre-init bootstrap error (line 37) remains as `fprintf(stderr)` — it cannot use the logger because `Logger::init()` has not been called yet. Usage/help text blocks in `main.cpp` remain as `fprintf(stderr)` (user-facing instructional text, not diagnostic).

- [x] **ADR-001 (Result/Error Pattern)** — Fully respected. The logging system is decoupled from `Error`/`Result<T>`. All migrated calls use `BUDDD_LOG_*` macros directly, not through error types.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/domain/logging.md`** — Updated to past tense for best practice #2 ("has been migrated"). Tag list accurately reflects all tags from the spec's tag mapping table. Migration note about stdout→stderr lifecycle messages added. ✅

- [x] **`docs/wiki/domain/business-rules.md`** — Observability messages table updated to show `BUDDD_LOG_INFO`/`BUDDD_LOG_ERROR` mechanisms instead of raw `std::cerr`/`fprintf`. stdout→stderr migration for window lifecycle messages documented. ✅

- [x] **`docs/wiki/architecture/data-flow.md`** — CLI data flow flowchart updated: unknown command/scene branches now show `BUDDD_LOG_ERROR` with `fprintf(stderr)` usage text. Output table updated to reflect new format. ✅

- [x] **`docs/adr/ADR-020-custom-logging-system.md`** — Consequences — Negative updated to past tense ("have been migrated"). Migration completion note added at bottom. ✅

- [x] **`docs/wiki/architecture/module-map.md`** — Verified already up-to-date by wiki-agent; no changes needed. ✅

## Warnings

Non-blocking concerns for awareness:

- **Contract scope vs. actual modifications**: The contract's "Files allowed to change" list was incomplete. It missed `tests/cli_app_tests.cpp`, `tests/scene_rendering_tests.cpp`, and incorrectly specified `asset_manager.tpp` as the tag declaration location. These gaps were caught and correctly handled by the implementer and code reviewer. Future contracts should be more thorough in auditing all files that may need adaptation.

- **Minor visual formatting change**: The original `fprintf(stderr, "Unknown command: '%s'\n\n", ...)` had a double newline producing a blank line separator between error text and usage help. After migration, `BUDDD_LOG_ERROR` (with single `\n` from ConsoleSink) eliminates the blank line. This is consistent with the spec's trailing `\n` removal rule. Test assertions use `find()` substring matching and still pass.

- **"Warning: unexpected arguments" migration**: The contract's instruction to collapse three `fprintf` calls into a single `BUDDD_LOG_WARN` with `"Warning: unexpected arguments after '{}': {} ..."` left the argument concatenation strategy vague. The implementer had to determine how to build the single string from the loop arguments. The resulting implementation is correct, but future contracts should be more explicit about such transformations.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- **ADR-020** ✅ Already updated by wiki-agent to reflect migration completion. No further changes needed.
- **Wiki files** ✅ All four wiki files (logging.md, business-rules.md, data-flow.md, module-map.md) already updated or verified. No further changes needed.
- **Spec file** ⚠️ Minor inaccuracy on line 163 ("main.cpp already has a log tag declaration") should be corrected if the spec is ever revised, though as a historical snapshot (read-only after workflow completion) this is acceptable.

## Summary

**Verdict: Accepted.** All workflow gates were followed in the correct order. Human validation (Hilderin, 2026-06-06 11:54:03) was obtained before implementation. The spec, contract, and implementation are consistent in intent and content. Two minor corrections (asset_manager.tpp tag placement, additional test file modifications) were necessary deviations from contract scope, both documented and correct. All three relevant ADRs (ADR-020, ADR-014, ADR-001) are respected. Wiki files have been updated to reflect the current state. All 20 acceptance criteria pass. Build succeeds with zero errors/warnings. All 407 tests pass. No unresolved blocking issues.

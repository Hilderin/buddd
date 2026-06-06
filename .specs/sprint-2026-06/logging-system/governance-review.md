# Governance Review — Logging System (SPEC-021 / IMPL-021)

## Review summary

**Verdict: Approved** — the feature is coherent across all artifacts and ready to proceed. All workflow gates completed successfully with no blocking issues. A few minor cross-document inconsistencies exist (stale spec path reference, ADR number mismatch in contract, `LogMessage::message` type change) but each was reviewed and accepted through the critic process or is a harmless spec artifact.

## Documents checked

| Artifact | Path | Status |
|---|---|---|
| Spec | `.specs/sprint-2026-06/logging-system/spec.md` | ✅ |
| Spec-critic | `.specs/sprint-2026-06/logging-system/spec-critic.md` | ✅ |
| Implementation contract | `.specs/sprint-2026-06/logging-system/implementation-contract.md` | ✅ |
| Implementation-contract-critic | `.specs/sprint-2026-06/logging-system/implementation-contract-critic.md` | ✅ |
| Code review | `.specs/sprint-2026-06/logging-system/code-review.md` | ✅ |
| ADR-020 | `docs/adr/ADR-020-custom-logging-system.md` | ✅ |
| ADR-019 | `docs/adr/ADR-019-architecture-boundaries.md` | ✅ |
| Wiki — logging | `docs/wiki/domain/logging.md` | ✅ |
| Wiki — business rules | `docs/wiki/domain/business-rules.md` | ✅ |
| Wiki — module map | `docs/wiki/architecture/module-map.md` | ✅ |
| Coordination | `.specs/sprint-2026-06/logging-system/coordination.md` | ✅ |

---

## Cross-document coherence

Items that are contradictory or inconsistent across documents:

- [x] **Spec design sketch path** (`spec.md` line 325) references `include/buddd/log.h` — the actual file is `src/engine/log/log.h`. This is a design-sketch artifact; the implementation contract, wiki, and code all use the correct path `src/engine/log/log.h`. Non-blocking — specs are historical snapshots of intent at time of writing.

- [x] **`LogMessage::message` type** (`spec.md` line 144) shows `std::string_view`, but the implementation contract deliberately changes it to `std::string` (section 1, with inline rationale: MemorySink ownership). The code implements `std::string` per contract. This deviation was reviewed and accepted through the critic process (W-01 resolved in implementation-contract-critic). Non-blocking.

- [x] **Implementation contract references ADR-021** (`implementation-contract.md` line 599) but the actual ADR created is **ADR-020**. The contract wrote "ADR-021 or similar" as a placeholder; the adr-agent numbered it ADR-020 following the existing sequence. The content is consistent. Non-blocking.

- [x] **Contract prefix lengths off-by-one** (`implementation-contract.md` section 8) specifies `substr(0, 11)` for `--log-level=` and `substr(0, 12)` for `--log-filter=` — the actual prefix lengths are 12 and 13. The code review confirms the implementation correctly uses the real lengths. This is a contract typo, not a governance issue. The spec does not specify exact prefix-match lengths.

- [x] **Tag truncation warning mechanism** — spec (line 357) implies the loggers' own API (tag `[Log]`) should be used, but the implementation contract (section 13) specifies raw `fprintf(stderr)` to avoid recursion. Flagged as W-02 in contract-critic, accepted as a pragmatic deviation. Non-blocking.

---

## ADR alignment

Required ADRs exist and are consistent:

- [x] **ADR-020** exists and documents all architectural decisions: custom logger over spdlog, five log levels, macro API with BUDDD_LOG_TAG, singleton Logger with LogConfig, Sink interface with three implementations, mutex-based thread safety, source-location capture, std::format, CLI control flags, zero external dependencies, and decoupling from Error/Result<T>.
- [x] **ADR-001** constraint respected: the logger does NOT depend on `buddd::engine::Error`/`Result<T>`. AC-020 verifies this. CLI parsing uses `Result<T>` but that is in `src/cmd/`, not the logger.
- [x] **ADR-009** test naming convention followed: `logging_tests.cpp`.
- [x] **ADR-014** CLI infra respected: log flags are parsed in `src/cmd/app_config.cpp`.
- [x] **ADR-019** architecture boundary respected: the logger is standard-library-only (except POSIX `write(2)` for raw stderr, mandated by contract for pre-init failure paths). No platform/graphics/input headers included outside `src/engine/`.

---

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/domain/logging.md`** — Created with full API reference, tag naming conventions, macro usage, CLI flag reference, sink behavior, thread safety, and best practices. Consistent with spec and ADR-020.
- [x] **`docs/wiki/domain/business-rules.md`** — Updated with "Structured logging (new system as of SPEC-021)" section under "Observability messages", documenting CLI flags and output formats.
- [x] **`docs/wiki/architecture/module-map.md`** — Updated with "Log submodule (log/)" section listing all 10 files under `src/engine/log/` with their roles.
- [x] Wiki does NOT contradict ADR-020 or any other ADR.

---

## Workflow gate check

All gates completed with no remaining blocking issues:

| Gate | Status | Blocking issues |
|---|---|---|
| spec-author | completed | none |
| spec-critic | completed | none |
| implementation-contract-author | completed | none |
| implementation-contract-critic | completed | none |
| Human Validation | approved | none |
| code-implementer | completed | none |
| code-reviewer | completed | none |
| adr-agent | completed | none |
| wiki-agent | completed | none |
| governance-reviewer | this review | none |

---

## Governance rule validation

- [x] **Authority order** respected: ADR > Spec > Wiki > Code.
- [x] **No code implemented directly from raw request** — proper spec → contract → code flow followed.
- [x] **No code implemented directly from spec** — implementation contract was used as intermediary.
- [x] **No silent architecture change** — ADR-020 documents all decisions.
- [x] **No ADR history rewritten** — no existing ADRs modified.
- [x] **Wiki does not contradict ADRs** — all wiki content consistent with ADR-020.

---

## Warnings

Non-blocking concerns for awareness (carried forward from earlier reviews):

- **W-03 (from spec-critic)** — AC-010 thread safety stress test is timing-dependent; consider a more deterministic test in the future.
- **W-04 (from spec-critic)** — `--log-level` with invalid level string causes process exit; coupling logger init to process lifecycle.
- **W-04 (from contract-critic)** — Integration test uses hard-coded `/tmp/test.log` instead of `temp_filename()` helper, risking parallel test collisions.
- **W-06 (from contract-critic)** — Logging flag position flexibility relative to subcommands is undocumented in the contract.
- **W-02 (from contract-critic)** — Tag truncation warning uses raw `fprintf(stderr)` instead of the logging API (spec deviation, justified by recursion avoidance).

---

## Questions for human

None.

---

## Recommendation

**Approved.** All artifacts are internally consistent and cross-document coherence is maintained. All workflow gates completed successfully. The minor inconsistencies identified (stale path in design sketch, ADR number placeholder, `LogMessage::message` type change) were reviewed and accepted through the critic process. Proceed to finalization.

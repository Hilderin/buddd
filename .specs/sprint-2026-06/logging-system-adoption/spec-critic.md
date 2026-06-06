# Spec Review — SPEC-022 Logging System Adoption (Ad-hoc stdout/stderr Migration)

**Re-review (Loop #2):** All previously raised issues have been addressed. The spec now satisfies all Definition of Ready criteria. No new blocking issues found.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Missing documentation updates list** (Definition of Ready — Documentation: "Existing documentation that must be updated is listed"). ~~The spec does not identify which existing wiki files, ADRs, or other docs need updating after the migration.~~ **RESOLVED**: A dedicated `## Documentation to update` section has been added (lines 350–358), listing 5 documents (`logging.md`, `module-map.md`, `business-rules.md`, `data-flow.md`, `ADR-020`) with specific update guidance.

## Warnings

Non-blocking concerns for awareness:

- **Behavior change vs. non-goals contradiction**: ~~Non-goals state "Do NOT change behavior or message content — only the output mechanism changes," but User Story 4 (and AC-016) remove `std::exit()`/`std::terminate()` after "FATAL" log calls, changing program control flow.~~ **RESOLVED**: Non-goals section (line 38) now explicitly states: "Exception: `std::exit()`/`std::terminate()` calls following fatal error logging are replaced with normal control flow...".

- **Fragile line-number references**: ~~Multiple acceptance criteria reference specific line numbers in `main.cpp`~~ **RESOLVED**: AC-006 through AC-009 now use context descriptions (function names, code behavior anchors) instead of line numbers.

- **`#ifndef NDEBUG` removal not discussed for performance**: ~~The spec does not note binary size / compile-time implication.~~ **RESOLVED**: Edge case 9 (line 319) now acknowledges: "The `Logger::is_enabled()` check runs before any formatting, so the runtime cost is negligible when debug logging is disabled (no measurable impact)."

- **Tag `FileWatcher` deviates from wiki convention**: ~~The spec assigns bare `FileWatcher`.~~ **RESOLVED**: Tag renamed to `Asset:FileWatcher` in both the tag mapping table (line 144) and the files-to-modify table (line 216).

## Required changes

Concrete, actionable changes requested:

1. ~~Add a "Documentation updates" section listing the wiki files~~ **RESOLVED**: Section added (lines 350–358).
2. ~~Either qualify the non-goal "Do NOT change behavior"~~ **RESOLVED**: Exception qualified on line 38.

## Suggested improvements

Optional ideas (not required):

- Consider adding a `grep -rn 'std::cerr\|fprintf.*stderr\|printf(\|write(STDERR_FILENO' src/engine/ src/cmd/` verification step to the E2E section (it is mentioned in Observability but could be an explicit AC). *(Still a valid suggestion, not required.)*
- The `Level mapping` table's "Example" column contains generic descriptions rather than concrete example strings in a few rows (e.g., `#ifndef NDEBUG std::cerr` → "Cache hit, FileWatcher start/stop"). Adding concrete example strings would improve clarity. *(Still a valid suggestion, not required.)*
- Tag `FileWatcher` might be better named `Asset:FileWatcher` to align with the wiki's hierarchical naming convention. **RESOLVED**: Tag has been renamed to `Asset:FileWatcher`.

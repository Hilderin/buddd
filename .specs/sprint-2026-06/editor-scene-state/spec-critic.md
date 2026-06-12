# Spec Review — Editor Scene State (SPEC-029)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **DoD — Missing list of documentation to update**: The Definition of Ready criterion under **Documentation** states: *"Existing documentation that must be updated is listed (README, wiki, ADRs, other specs)."* The spec did not identify which existing documentation files need updating. **Resolved (re-review)**: The updated spec now includes a "File changes" section with a "Modified" table listing 6 files: 3 source files and 3 wiki files (`module-map.md`, `editor-panels.md`, `scene-management.md`). This satisfies the DoD criterion.

## Warnings

Non-blocking concerns for awareness:

- [x] **Mixed scope in spec title**: The spec was titled "Editor Scene State" but included [[nodiscard]] fixes. **Resolved (re-review)**: Title updated to "Editor Scene State + [[nodiscard]] Fixes".

- [x] **AC-009 setup-failure test ambiguity**: The spec referenced World creation within setup() relative to failure points. **Resolved (re-review)**: World is now created in the constructor (not setup), so the ordering ambiguity is eliminated. AC-009 now tests that `world()` still returns a valid reference when `setup()` fails, with ASan-clean destruction. Clear and testable.

- [x] **AC-008 UB testing limitation**: The original spec used assertion/UB patterns for pre-setup `world()` access. **Resolved (re-review)**: AC-008 now requires no assertions or guards. World is constructed in the constructor, so `world()` is always safe. No UB testing needed.

- [x] **Q-01 unresolved (`[NEEDS CLARIFICATION]`)**: The assertion message format for `world()` was marked as needing clarification. **Resolved (re-review)**: No assertion is needed — World is created in constructor, always valid. Q-01 removed from spec.

- [x] **Assumption A-04 unverified**: The spec assumed exactly 11 [[nodiscard]] warning sites at specific file locations. **Resolved (re-review)**: Verified by code inspection — 3 sites in `editor.cpp` (lines 93, 96, 99), 2 sites in `menu_bar.h` (lines 43, 46), and 6+ sites in `editor_tests.cpp` (wrapped in `REQUIRE()`/`REQUIRE_FALSE()` assertions). Assumption A-04 updated to reflect verified state.

## Required changes

None. All previous blocking issue and warnings have been resolved.

## Suggested improvements

Optional ideas (not required):

- Add a brief "Documentation to update" subsection listing files such as:
  - `docs/wiki/architecture/module-map.md` — add `world_` member / `world()` accessor to Editor class entry
  - `docs/wiki/editor/editor-panels.md` — update to reflect `Editor` now owns a `World` via `unique_ptr`
- Clarify the ordering of World creation within `Editor::setup()` relative to the ImGui init check (does World creation happen before or after the `engine_imgui::is_initialized()` check?).
- Consider adding a section on multi-instance safety: if two `buddd edit` instances run from the same directory (as mentioned in editor-foundation EC), the editor World is per-instance (stack-local), so no conflict. Worth calling out explicitly.
- Consider adding an AC for verifying the `#include` path (`"scene/world.h"`) in `editor.h` matches the project convention, as stated in Assumption A-09.

---

## Re-review summary

This is the **second review** of SPEC-029 (Editor Scene State + [[nodiscard]] Fixes), after the spec-author addressed all previous blocking issues and warnings.

**What the spec covers:**
1. `Editor` class gains a `std::unique_ptr<World>` member, created in the **constructor** (not setup), destroyed in the destructor via `unique_ptr`.
2. `world()` accessor returning `World&` — always valid, no assertions or guards needed.
3. 11 `[[nodiscard]]` warning fixes from editor-foundation's `CommandStack::undo()` / `redo()` call sites (verified by code inspection, already applied).
4. 10 acceptance criteria covering lifecycle, accessor, empty-state, shutdown idempotency, and build cleanliness.
5. Comprehensive edge cases (8), error cases (2), and assumptions (10).

**Consistency checks:**
- ✅ **With editor-foundation (SPEC-028)**: The Editor lifecycle, CommandStack, EditorPanel, EditorMenu patterns are consistent. Extends the existing class without breaking changes. The `Editor() = default` from ADR-027 is no longer default (constructor creates World) — this is an expected extension, not a contradiction.
- ✅ **With ADR-027 (Editor Architecture)**: Direct member variable pattern (Decision 4) preserved — `unique_ptr<World>` is a new direct member. Architecture boundary (Decision 6) respected — no SDL3/OpenGL/GLM. App lifecycle (Decision 2) unchanged.
- ✅ **With ADR-029 (Editor UX Decisions)**: Editor World is the foundation for later panels, Play mode cloning (Decision 5). The spec correctly defers `World::clone()` to a future feature.
- ✅ **With north-star UX spec**: The UX spec's Key Entities section lists "Editor" as owning the editor World. SPEC-029 implements exactly that. Consistent.
- ✅ **With wiki files**: No contradictions found. The file changes correctly identify 3 wiki files that need updates to reflect the new World ownership.

**Previous blocking issue resolution:**
- ✅ **Documentation-to-update list**: Added "File changes" section with Modified table listing 3 wiki files. Satisfies DoD criterion.

**Previous warning resolution:**
- ✅ **Mixed scope in title**: Title updated to "Editor Scene State + [[nodiscard]] Fixes"
- ✅ **AC-009 setup-failure test ambiguity**: World created in constructor — ordering ambiguity eliminated
- ✅ **AC-008 UB testing limitation**: No assertion/UB required — World always valid
- ✅ **Q-01 unresolved**: Removed from spec entirely
- ✅ **A-04 unverified**: Verified by code inspection (3+2+6 sites confirmed)

**New issues in re-review:** None found. The constructor-creation pattern is fully consistent with all lifecycle points documented. No new contradictions, ambiguities, or gaps identified.

**DoD assessment: 12 of 12 criteria satisfied.**

## Definition of Ready assessment

### Clarity & Completeness

| Criterion | Verdict |
|---|---|
| Scope is clearly defined (what is included and what is explicitly excluded) | ✅ Pass |
| Dependencies on other features, modules, or external systems are identified | ✅ Pass |
| Edge cases and error conditions are described | ✅ Pass |
| The expected behavior is unambiguous and testable | ✅ Pass |

**Notes**: Scope is well-bounded with 9 non-goals. Edge case table (8 entries) and error case table (2 entries) are comprehensive. Minor ambiguity in AC-008/AC-009 noted in warnings but not blocking.

### Verification

| Criterion | Verdict |
|---|---|
| The spec defines how the feature will be verified end-to-end | ✅ Pass |
| Acceptance criteria are specific, measurable, and verifiable | ✅ Pass |
| Success and failure states are described | ✅ Pass |

**Notes**: 10 ACs with explicit verification methods (unit test, build check, manual). E2E section covers CI and manual paths. 4 success criteria with metrics.

### Documentation

| Criterion | Verdict |
|---|---|
| Interface changes (CLI flags, API signatures, config keys) are documented | ✅ Pass |
| Existing documentation that must be updated is listed | ✅ Pass |

**Notes**: Interface changes are documented with exact C++ signatures. The "File changes" section includes a "Modified" table listing 3 wiki files (`module-map.md`, `editor-panels.md`, `scene-management.md`) and 3 source files. Unchanged files are listed with their rationale. This now satisfies the DoD criterion.

### Technical

| Criterion | Verdict |
|---|---|
| Technical constraints are identified (system APIs, libraries, build changes) | ✅ Pass |
| Risks or unknowns are surfaced | ✅ Pass |
| Performance or resource implications, if any, are noted | ✅ Pass |

**Notes**: No new libraries or system APIs. One `unique_ptr<World>` member — negligible memory. ASan/Valgrind testing for leak detection is documented. OOM risk from `make_unique<World>()` is documented in error cases.

### Overall verdict

**ACCEPTED** — All blocking issues resolved. All 12 Definition of Ready criteria are satisfied. The spec is clear, testable, internally consistent, and aligned with existing ADRs and wiki. No new issues introduced by the constructor-creation pattern change.

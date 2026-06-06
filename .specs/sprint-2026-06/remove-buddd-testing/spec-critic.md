# Spec Review — Remove BUDDD_TESTING

## Blocking issues

No blocking issues found.

The spec satisfies all Definition of Ready criteria:

- **Clarity & Completeness**: Scope is clearly defined with explicit goals and non-goals. All edge cases, error conditions, and re-entrancy risks are described.
- **Verification**: Acceptance criteria are specific and verifiable (grep, compile, test suite). E2E verification path is documented.
- **Documentation**: All API signature changes are documented with before/after code blocks. Documentation changes for 5 wiki/ADR files plus a new ADR are listed.
- **Technical**: CMake build changes are precisely specified (exact lines to remove). Risks (re-entrancy, binary size) are surfaced.

## Warnings

Non-blocking concerns for awareness:

- **AC-008 and AC-009 reference unstable test case numbers**. The spec says "same test pattern as current Test 27" and "same test pattern as current Test 26". Test case numbering in a file can shift when tests are added/reordered. The descriptions are still implementable from the AC text alone, but the "Test N" references are fragile and could become stale. Consider replacing with specific `TEST_CASE` names from the test file.

- **Missing file in documentation update list**: `docs/wiki/domain/assertions.md` line 40 references `BUDDD_TESTING` ("no additional CMake flags or BUDDD_TESTING involvement"). While the statement remains factually correct after the removal (it becomes even more true), the reference to a now-removed define could confuse future readers. Consider updating or removing the `BUDDD_TESTING` mention from this file for clarity.

- **Ambiguous wiki target**: The spec lists `docs/wiki/domain/asset-manager.md (or equivalent)` for documenting the new public API methods. The wiki groups AssetManager rules under `docs/wiki/domain/business-rules.md` — no standalone `asset-manager.md` exists. The "or equivalent" hedge covers this, but the spec should pin the exact target file for clarity.

- **No comprehensive "all files to modify" table**: Source file changes are distributed across sections 1–5 (API changes), section "Build changes" (2 CMake files), "Test changes" (table), and "Documentation changes" (table). A single unified file list would reduce the risk of a file being overlooked during implementation.

## Required changes

None (no blocking issues).

## Suggested improvements

- Replace "Test 26" / "Test 27" references in AC-008 and AC-009 with the actual `TEST_CASE` names (or add a cross-reference to those names).
- Pin `docs/wiki/domain/business-rules.md` as the target for public API documentation instead of the ambiguous `asset-manager.md (or equivalent)`.
- Add a unified "Files to modify" summary section.
- Update `docs/wiki/domain/assertions.md` line 40 to remove the `BUDDD_TESTING` mention since the define will no longer exist.

## Verdict

**Status**: completed (no blocking issues)

The spec is thorough and well-structured. All API changes are precisely described with before/after code blocks. The build changes are exact (line numbers in CMakeLists.txt). Edge cases and error scenarios are systematically covered (empty path, unknown path, pre-init call, re-entrancy risk). The acceptance criteria are measurable (grep checks, compile checks, test suite results). Documentation updates for wiki and ADR files are explicitly listed.

The spec is ready to proceed to implementation contract authoring.

# Spec Review — SPEC-035: CLI `buddd edit [<scene>]` — open editor with scene path

## Summary

**Re-review: PASSED** — All 3 previous blocking issues have been resolved, and the spec now satisfies all Definition of Ready criteria.

The spec-author addressed every issue:
1. **Directory path handling** — Now consistently uses `is_regular_file()` throughout. Directories are always rejected pre-opening (exit 1, no window). Updated user-visible behavior, edge-case table, error cases, permissions, and assumptions.
2. **Empty string (`buddd edit ''`)** — Now unambiguously treated as "no argument" (editor opens empty). Explicit check in dispatch logic step 1.
3. **Dispatch logic** — New 4-step dispatch section clearly distinguishes flags (starts with `-`) from unknown positional args.

Additionally, the spec-author added: wiki documentation pages requiring update (data-flow.md, module-map.md, business-rules.md) and window title verification to AC-002. No new issues were found during re-review.

## Definition of Ready checks

- **Scope** ✅ — Clear goals (G-01–G-07) and explicit non-goals (NG-01–NG-07).
- **Dependencies** ✅ — Dependencies on `Editor::open_scene()`, `parse_running_args()`, `EditorApp`, `run_app()` identified. Assumptions section documents 10 key dependencies.
- **Edge cases/errors** ✅ — 13 edge cases and 7 error cases, all unambiguous and consistent.
- **Unambiguous and testable** ✅ — 4-step dispatch logic clearly defined; all inputs described.
- **E2E verification** ✅ — Integration tests tagged `[cli][app]`, headless mode, capture file validation.
- **AC specificity** ✅ — All 10 ACs have clear, verifiable verification steps.
- **Success/failure states** ✅ — Exit codes, stderr output, window-created detection all specified.
- **Interface changes documented** ✅ — CLI syntax and help text with exact before/after.
- **Existing docs listed** ✅ — `k_usage_text` identified AND wiki pages listed: data-flow.md, module-map.md, business-rules.md.
- **Technical constraints** ✅ — `std::filesystem`, `parse_running_args()`, `Editor::open_scene()` identified.
- **Risks surfaced** ✅ — Open questions (Q-01–Q-04) resolved, assumptions explicit.
- **Performance implications** ✅ — Lightweight change, no significant impact.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **Directory path handling contradiction** — **RESOLVED**: Spec now uses `is_regular_file()` consistently throughout. User-visible behavior (line 52), edge-case table (line 248), error cases (line 255), and permissions (line 264) all agree: directories are rejected pre-opening with exit 1, no window.

- [x] **Empty string (`buddd edit ''`) edge case** — **RESOLVED**: Empty string is now unambiguously treated as "no argument" (editor opens empty). Dispatch logic step 1 (line 121) explicitly checks for empty string. Edge-case entry (line 237) is clear and consistent. Example (line 131) shows `buddd edit ''` → empty editor.

- [x] **Dispatch logic for non-YAML positional args** — **RESOLVED**: New "Dispatch logic for `edit` subcommand" section (lines 117–135) defines 4-step flow: (1) no arg/empty → empty editor, (2) YAML extension → `is_regular_file()` check, (3) starts with `-` → flags-only path, (4) otherwise → unknown arg error + exit 1. Examples cover all cases including `--capture` (step 3) vs `somearg` (step 4).

## Warnings

All previous warnings have been resolved by the spec-author:

- ~~**Wiki documentation updates not listed**~~ ✅ — Data-flow.md, module-map.md, business-rules.md now listed at lines 138-143 with specific changes required.
- ~~**Empty string edge case wording is confusing**~~ ✅ — Completely rewritten for clarity. Brevity and unambiguousness achieved.
- ~~**AC-002 does not verify window title**~~ ✅ — Window title verification added to AC-002 (line 206): "Verify log output contains `'demo.yaml — Buddd Editor'`."

No new warnings identified during re-review.

## Required changes

All 3 required changes from the previous review cycle have been implemented by the spec-author:

1. ✅ **Directory path contradiction** — Switched from `exists()` to `is_regular_file()` throughout. Directories now consistently exit 1, no window.
2. ✅ **Empty string ambiguity** — Rewritten to unambiguously treat empty string as "no argument" (editor opens empty).
3. ✅ **Dispatch logic** — New 4-step dispatch section clearly distinguishes flags (starts with `-`) from unknown positional args.

No further changes required.

## Suggested improvements

Optional ideas (not required) — remain valid from first review:

- Consider adding an AC for the `edit` command's behavior when the editor is run headless without a display (current behavior per module-map.md line 359: "errors out if `BUDDD_HAS_DISPLAY=OFF`").
- Consider documenting that `buddd_editor.ini` persistence is unaffected by this change (ini file still written at CWD, no change to ini behavior).

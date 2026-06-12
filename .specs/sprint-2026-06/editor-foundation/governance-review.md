# Governance Review — Editor Foundation

**Re-validation (12-Jun-2026)**: All 4 previous blocking issues are confirmed resolved. ADR-027 has an amendment acknowledging the backward-compatible extension. The spec's Observability section was removed per human decision. A unit test for AC-043 (edge-triggered shortcut) was added. The implementation contract's ShortcutRegistry signature now matches the actual `process(EngineContext const&, bool)`. No new blocking issues found. One pre-existing behavioral difference (modifier-key shortcuts bypassing WantCaptureKeyboard) remains as an accepted design choice—see Warnings. **Overall verdict: ACCEPTED.**

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **ADR-027 Decision 2 contradicted by spec** — **RESOLVED (12-Jun-2026)**: ADR-027 now has an Amendment section (lines 183–201) acknowledging that Decision 2 has been backward-compatibly extended by SPEC-028. The amendment documents the `App::update()` addition, `run_app()` modification, backward-compatibility with all 14 existing subclasses, and partial supersession of Decision 2. The `adr-agent` created this amendment in response to the governance review.

- [x] **Observability logging required by spec not implemented** — **RESOLVED (12-Jun-2026)**: The spec's Observability section was removed entirely (human decision per coordination.md spec-author update). The logging requirements are no longer spec requirements, so the absence of `BUDDD_LOG_DEBUG`/`BUDDD_LOG_TRACE` is no longer a gap. The wiki and code-review accurately reflect that only ini-file path logging exists.

- [x] **AC-043 verification method mismatch** — **RESOLVED (12-Jun-2026)**: A dedicated unit test was added to `tests/editor_tests.cpp` (lines 251–302) that creates a `ShortcutRegistry`, binds a Space key, injects a key-down event via SDL3 offscreen driver, calls `process()` twice, and verifies the action fires exactly once. The test is guarded by `#ifdef BUDDD_HAS_DISPLAY` and passes as part of the 508-test suite. This matches the spec's AC-043 requirement exactly.

- [x] **ShortcutRegistry::process() signature differs from contract** — **RESOLVED (12-Jun-2026)**: The implementation contract was updated (Step 6, line 415) to specify `process(buddd::engine::EngineContext const& ctx, bool want_capture) -> void`, matching the actual implementation in `src/editor/shortcut_registry.h`. The declaration, inline definition body (input extraction from ctx), and all call sites were updated by `implementation-contract-author`.

- [ ] **Spec NG-05 still contains ambiguous phrasing**: NG-05 starts with "No changes to engine core (`src/engine/`)" which is correct, then lists changes to `src/cmd/`. This is clearer than the original wording but the lead-in "No changes to engine core" followed immediately by change descriptions can be read as contradictory on first glance. **Minor textual ambiguity.**

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-027 (Editor Architecture)**: Exists and accepted. Covers editor as static library, `buddd::editor` namespace, Editor class with direct members, architecture boundary, CLI command, ImGui init fatal in display mode. **Amendment added (12-Jun-2026)**: Decision 2 ("no changes to App base class or run_app()") now has an amendment note documenting the backward-compatible extension by SPEC-028 — `App::update()` and `app.update(ctx)` call. The core architecture decision (editor reuses App lifecycle) is unchanged. ✅ Consistent.
- [x] **ADR-026 (ImGui Integration)**: Exists and accepted. Provides ImGui docking branch, frame lifecycle automation, backend isolation. The editor's `io.IniFilename` and `WantCaptureKeyboard` usage is consistent.
- [x] **ADR-019 (Architecture Boundaries)**: Exists and accepted. Enforced across `src/editor/` and `src/cmd/apps/`. Verified by grep — zero matches.
- [x] **ADR-014 (CLI App System)**: Exists and accepted. The feature extends it with a new `App::update()` lifecycle method. Compatible extension.
- [x] **ADR-011 (Ownership/Nullability)**: Exists and accepted. `[[nodiscard]]` conventions followed (verified by code-review — all 11 warnings resolved).
- [x] **ADR-012 (Navigable Object Graph)**: Exists and accepted. Editor accesses `InputSystem` via `ctx.services.platform().input_system()` — consistent pattern.
- [x] **ADR-029 (Editor UX Decisions)**: Exists and accepted. North-star UX design for tabs, Play mode, layouts. The v1 foundation implemented here is consistent with the north-star direction (placeholder panels, menu bar, command system).

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/editor/editor-panels.md`**: Accurately describes v1 foundation with clear "current status" notice at the top. North-star content is clearly marked as "Future vision (north-star)". The `v1 foundation (currently implemented)` section matches what was built. No contradictions.

- [x] **`docs/wiki/architecture/module-map.md`**: Updated by wiki-agent to reflect all new editor files (`command.h/.cpp`, `command_stack.h/.cpp`, `commands/`, `shortcut_registry.h`, `editor_menu.h`, `editor_panel.h`, `panels/`). Consistent with spec file list.

- [x] **`docs/wiki/architecture/dependency-map.md`**: Updated with internal editor dependencies. Consistent with implementation.

- [x] **`docs/wiki/architecture/overview.md`**: Updated `src/editor/` directory listing and editor key behaviors. Accurate.

- [x] **`docs/wiki/architecture/data-flow.md`**: Updated with `app.update(ctx)` in render loop and `buddd edit` CLI dispatch. Matches spec frame diagram.

- [ ] **Wiki accurately reflects v1 limitations**: All three aspirational wiki pages (scene-management.md, cross-panel-communication.md) have been updated with north-star notices. No wiki pages claim functionality that doesn't exist in v1. **Conforms.**

## Warnings

Non-blocking concerns for awareness:

- The `ShortcutRegistry::process()` implementation uses a per-binding modifier-key bypass for WantCaptureKeyboard (modifier shortcuts fire even when ImGui captures input), while the implementation contract (Step 6) still describes the simplified `if (want_capture) return;` early-return gating. This is an intentional design choice (documented in code-review) but creates a minor contract-code behavioral description gap. Previously noted; still present. Future contract updates should align the behavioral description with the actual implementation.
- The `#include <imgui_internal.h>` dependency in `editor.cpp` (needed for `ImGuiDockNode*` access in default layout check) relies on an internal ImGui API. Acceptable for v1 but should be reviewed if ImGui version is upgraded.
- Spec NG-05 still has minor textual ambiguity: "No changes to engine core" followed by change descriptions can read as contradictory on first glance. Non-blocking.

## Required governance updates

All three governance updates from the previous review have been resolved:

1. ✅ **ADR-027 amendment** — **DONE**. The `adr-agent` added an Amendment section (12-Jun-2026) documenting the backward-compatible extension. Verified in `docs/adr/ADR-027-editor-architecture.md` lines 183–201.
2. ✅ **Observability logging** — **DONE**. The spec's Observability section was removed (human decision). No logging requirements remain in the spec.
3. ✅ **AC-043** — **DONE**. A unit test was added to `tests/editor_tests.cpp` (lines 251–302) verifying edge-triggered shortcut behavior, matching the spec's AC-043 requirement.

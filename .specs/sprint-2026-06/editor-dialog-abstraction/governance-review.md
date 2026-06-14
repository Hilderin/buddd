# Governance Review — Editor Dialog Abstraction

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec struct vs prose inconsistency (shortcut field)**: The spec's `DialogButton` struct definition (lines 56–60) omits the `std::optional<ImGuiKey> shortcut` field, while the spec prose (line 115, Q-02) describes storing it. The spec-critic flagged this as W-04 (non-blocking). The implementation contract (line 86) correctly includes `std::optional<ImGuiKey> shortcut = std::nullopt;`, the code-review confirms it exists (AC-03), and the wiki documents it. All downstream artifacts agree — the spec struct definition is the sole anomaly. **Resolved by alignment of contract, code, and wiki.**

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-027 (Editor Architecture)**: The dialog abstraction follows all architectural rules: new code in `src/editor/`, `namespace buddd::editor`, direct member variables (no PIMPL), no SDL3/OpenGL/GLM headers. The `Editor` gains `dialogs_`/`opened_dialog_ids_` as direct private members — consistent with ADR-027 Decision 4.
- [x] **ADR-029 (Editor UX Decisions)**: The dialog abstraction replaces Phase 4 content (About popup → general dialog rendering) at the same position in the rendering pipeline. This is a compatible infrastructure change that does not alter any UX decision (tab system, layouts, Play mode, panels). No ADR amendment needed.
- [x] **ADR-026 (ImGui Integration)**: All ImGui APIs used (`OpenPopup`, `BeginPopupModal`, `EndPopup`, `SameLine`, `Button`, `IsKeyPressed(ImGuiKey_Escape)`) are available from the docking branch. No new ImGui integration patterns introduced.
- [x] **No new ADR required**: The implementation follows existing architectural decisions. ADR-027, ADR-026, and ADR-029 fully cover the architectural context. No ADR is deprecated or amended.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **module-map.md**: Correctly documents `editor_dialog.h` in the Editor UI abstractions table with `Dialog`, `CustomDialog`, and `DialogButton`. The `editor.h` entry documents `open_dialog()`, `dialogs_`, `opened_dialog_ids_`, and Phase 4 dialog rendering. No references to `show_about_` or `draw_about_popup()` remain.
- [x] **editor-panels.md**: Status header documents the dialog abstraction alongside previous features. The v1 foundation section provides a detailed bullet on Dialog/CustomDialog/DialogButton, ID-based dedup, Phase 4 rendering, Escape handling, and About popup migration. Related specs list includes the dialog abstraction spec.
- [x] **scene-management.md**: Status header documents Phase 4 as the general dialog rendering phase and notes save-prompt/error modals remain separate. The F-01 foundation section details Phase 4 dialog rendering with OpenPopup tracking, Escape dispatch, and erase_if cleanup. Last reviewed date updated.
- [x] **Wiki is descriptive, not prescriptive**: All wiki entries describe existing implementation state. No entries create future obligations or "become law."

## Warnings

Non-blocking concerns for awareness:

- **Spec struct definition is stale**: The spec's `DialogButton` struct (lines 56–60) omits the `shortcut` field that the spec prose, implementation contract, code, and wiki all include. This is a historical artifact — the spec-critic flagged it as non-blocking (W-04) and subsequent artifacts corrected it. The spec could be amended to include the field in its struct definition, but this is an optional cleanup, not a blocking issue.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None required. The wiki has been updated correctly by the wiki-agent. No ADR amendments are needed. No new ADRs are required.

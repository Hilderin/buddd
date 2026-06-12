# Governance Review — F-03 Entity Selection with Multi-Select

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] All 32 ACs (AC-01 through AC-32) are consistently traced from spec → contract → code → tests. The code-review confirms all ACs satisfied and all 8 DCs met.
- [x] Spec-critic and contract-critic loop-back #1 resolved both blocking issues (B-01: `std::minmax` dangling reference, B-02: empty-area click modifier guard). Code correctly implements both fixes.
- [ ] **Spec doc inaccuracy (non-blocking):** The spec (line 76) states `snapshot()` returns "a `Selection` copy of the current state (including anchor)" — but `Selection` is a pure set-of-`EntityId`s value object; the anchor is stored separately in `EditorSelection` and is NOT included in the `Selection` returned by `snapshot()`. The implementation correctly excludes the anchor. This is a minor documentation inaccuracy in the spec with no behavioral impact.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-027 (Editor Architecture)** — Direct member variables pattern (`EditorSelection selection_` is a direct member, not PIMPL). Namespace `buddd::editor`. Editor as static library. All satisfied.
- [x] **ADR-029 (Editor UX Decisions)** — Scene Panel (Hierarchy) is where entity selection originates (Decision 1: Tab-as-Editor-Context). Selection state is the cross-panel communication mechanism. Multi-select behavior (Ctrl+click toggle, Shift+click range, Ctrl+A select all) is consistent with the north-star UX. No ADR-029 decisions are violated.
- [x] **ADR-019 (Architecture Boundaries)** — No SDL3, OpenGL, or GLM headers appear in any `src/editor/` file. `editor_selection.h` only includes `scene/entity_id.h` (a pure value header) and standard library headers. Boundary fully respected.
- [x] **ADR-011 (Ownership/Nullability/NoDiscard)** — All query-only methods (`contains()`, `size()`, `empty()`, `first()`, `snapshot()`, `anchor()`, `current()`) use `[[nodiscard]]` as required.
- [x] No new ADR is needed. The `Selection` value class and `EditorSelection` manager follow existing patterns established by ADR-027, ADR-029, ADR-011, and ADR-019.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] `docs/wiki/editor/editor-panels.md` — Updated with F-03 banner. Scene Panel section documents selection behavior (click-to-select, multi-select, highlighting, API). v1 foundation entries include `Selection`, `EditorSelection`, `SelectionModifier`, and `ctx.editor.selection()` access pattern. Accurately reflects the implementation.
- [x] `docs/wiki/editor/cross-panel-communication.md` — Updated with F-03 banner. "Hierarchy click → EditorSelection" row marked as Implemented (F-03). Inspector + Viewport consumers correctly marked as deferred (F-05/F-07). F-03 v1 foundation entries accurate.
- [x] `docs/wiki/editor/scene-management.md` — Not modified by F-03, which is correct per spec documentation impact (no changes expected). Still accurately reflects F-01/F-02 state.
- [x] Wiki does not become law — ADRs remain the authority. The wiki accurately describes the current operational state without contradicting any ADR.

## Warnings

Non-blocking concerns for awareness:

- **Test temp file leak**: The `open_scene()` test (`f03_entity_selection_tests.cpp` line 466) creates a temp file via `mkstemp` (`/tmp/buddd_f03_XXXXXX`), then saves the scene to `<name>.yaml`. Only the `.yaml` file is cleaned up; the raw `mkstemp` file (without `.yaml`) leaks on disk. Not a blocking issue (ephemeral `/tmp`), but should be fixed for cleanliness.
- **Spec doc inaccuracy**: The spec describes `snapshot()` as returning "a `Selection` copy of the current state (including anchor)" — but `Selection` does not store the anchor. The anchor is a separate `std::optional<EntityId>` in `EditorSelection`. The implementation correctly excludes anchor from snapshot. This inaccuracy has no behavioral impact since implementers followed the contract, but may cause confusion for future readers of the spec.
- **Anchor asymmetry (documented)**: `clear()` clears `anchor_`, while `set_selection({})` (empty span) leaves `anchor_` unchanged. This is intentional per spec and correctly implemented, but may surprise future callers who expect `set_selection({})` to behave like `clear()`.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- None. All ADRs are consistent with the implementation. The wiki has been correctly updated. No new ADR is required. The spec doc inaccuracy noted above is a minor documentation issue in the archival spec (spec.md is read-only after workflow completion per AGENTS.md, so no update is expected).

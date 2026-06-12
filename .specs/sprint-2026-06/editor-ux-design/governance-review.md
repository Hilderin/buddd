# Governance Review — Editor UX Design

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [ ] **Wiki references ADR-029 as "in-progress" but ADR-029 is Accepted** — `docs/wiki/editor/editor-panels.md` line 309 says "ADR-029 — Editor UX decisions (panel layout, tab system, play mode) — in-progress," but ADR-029's status is `Accepted`. This mismatch must be corrected to avoid confusion about the ADR's authoritative status.
- [ ] **ADR Index (`docs/wiki/decisions/adr-index.md`) does not list ADR-025 through ADR-029** — The index is stale. It only lists ADRs up to ADR-024. ADR-025 (if it exists), ADR-026 (ImGui), ADR-027 (Editor Architecture), ADR-028 (Component Registry), and ADR-029 (Editor UX Decisions) are all missing from the index. This makes it harder for readers to discover these decisions.

## ADR alignment

Required ADRs exist or are proposed:

- [x] ADR-029 (`docs/adr/ADR-029-editor-ux-decisions.md`) exists and is in `Accepted` status.
- [x] ADR-029 correctly captures all 10 key architectural UX decisions from the spec (Tab-as-Editor-Context Model, One-Scene-at-a-Time, Fixed Layout Per Tab Type, Detached Tabs as Separate OS Windows, Play Mode with World Cloning + Read-Only Inspection, Bottom Panel Tab Bar, Entity Creation as Child of Selected, Prefab Editing in Tabs, Console Persistence Across Mode Transitions, Play Mode Visual Indicator). Each decision is traceable to spec sections.
- [x] ADR-029 does not contradict ADR-027 (Editor Architecture). ADR-027 covers technical architecture (static library, App lifecycle, ImGui init, architecture boundary); ADR-029 covers the UX model that builds on that architecture. ADR-029's detached-tabs design (separate OS windows with own GL context via engine `Window` abstraction) respects ADR-027's architecture boundary constraint (no SDL3/OpenGL/GLM headers outside `src/engine/`).
- [x] ADR-029 correctly identifies its relationship to ADR-026, ADR-027, ADR-019 in its "Impact on existing ADRs" section. No amendments needed.
- [x] Spec.md's "Documentation to update" (line 961) suggested updating ADR-027 with UX decisions. Instead, a separate ADR (ADR-029) was created. This is a better approach than amending ADR-027 (which is about technical architecture, not UX). The two ADRs cleanly separate concerns.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] `docs/wiki/editor/editor-panels.md` faithfully reflects the spec and ADR-029 — tab system, panel layouts, Play Mode behavior, entity operations, and panel reference all match the north-star spec without contradiction.
- [x] The wiki correctly references ADRs as authoritative (links to ADR-027, ADR-026, ADR-028, ADR-019, ADR-014) — it does not become law itself.
- [x] `docs/wiki/README.md` includes a link to `[Editor Panels](editor/editor-panels.md)` with a correct description ("Editor panel layout, tab types, panel reference, play mode behavior"). ✓
- [x] The architecture overview and module-map already contain editor references from the prior scaffolding work — no further update needed.

## Warnings

Non-blocking concerns for awareness:

- **ADR Index stale**: `docs/wiki/decisions/adr-index.md` only covers ADR-001 through ADR-024. ADR-025–029 are absent. This is an existing wiki maintenance gap independent of this workflow but should be addressed.
- **Spec's "Documentation to update" (line 959) recommended updating `docs/wiki/architecture/overview.md`**: The overview already references the editor from prior scaffolding work. The module-map also contains editor architecture details. No contradiction, but the spec's guidance was not explicitly tracked — the scaffolding feature handled it.
- **Spec's "Documentation to update" (line 960) recommended updating `docs/wiki/engineering/setup.md`**: No editor-specific setup instructions appear there. This is acceptable for MVP1 (no special setup needed beyond standard build), but may be needed later.
- **No implementation-contract.md or code-review.md exist yet**: This is expected — the workflow is at the `adr+wiki-publishing` stage. These will be created in subsequent steps. The governance review validates what exists.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- Update `docs/wiki/editor/editor-panels.md` line 309: change "in-progress" to "Accepted" for ADR-029.
- Update `docs/wiki/decisions/adr-index.md` to add entries for ADR-026, ADR-027, ADR-028, and ADR-029.

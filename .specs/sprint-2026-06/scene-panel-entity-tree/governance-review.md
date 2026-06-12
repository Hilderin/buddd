# Governance Review — Scene Panel — Entity Tree (F-02)

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec ↔ Contract (AC-01 through AC-06)**: Spec describes unit tests verifying ImGui call semantics (`TreeNodeEx` flags, `Text` calls). Contract defers these to code review + manual smoke test, citing lack of ImGui call-capture infrastructure. Gap accepted by spec-critic, contract-critic, and human approver as a pragmatic implementation detail. No automated regression protection exists for these ACs until `imgui_test_engine` or equivalent is introduced.
- [x] **Contract ↔ Code**: All 11 done criteria (DC-01 through DC-11) are verified satisfied by the code-reviewer. No discrepancies found.
- [x] **Spec ↔ Wiki (north-star EC-01)**: Wiki edge case EC-01 states "Hierarchy shows nothing" for an empty scene, which contradicts F-02's requirement to display "No entities" text. However, this is in the north-star/future section of the wiki — the status banner at the top of `editor-panels.md` correctly documents the current F-02 implementation as showing "No entities". The wiki-agent flagged this; acceptable for north-star content but worth resolving for consistency.
- [x] **Spec ↔ Implementation (null-entity guard)**: Contract and code include a defensive null-entity guard (`entity.id() != EntityId::none()`) not present in the spec's assumptions (A-01). This is a reasonable addition — no contradiction (spec does not forbid it), and the code compiles (API exists).
- [x] **Contract ↔ Wiki**: Both wiki pages (`editor-panels.md`, `scene-management.md`) correctly document the `EditorContext` struct, the signature change pattern, and panel world-access via `ctx.editor.world()`. No contradictions.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-027 (Editor Architecture)**: EditorContext is a lightweight aggregate with direct member variables, consistent with ADR-027's direct-member convention. No PIMPL, no virtual methods. EditorContext is an implementation detail — no ADR amendment needed. ✓
- [x] **ADR-029 (Editor UX Decisions)**: Scene Panel (Hierarchy) is documented as the entity tree. F-02 implements the first working panel (tree rendering only, no selection/deferred). Consistent with ADR-029's panel descriptions. ✓
- [x] **ADR-026 (Dear ImGui Integration)**: Uses `ImGui::TreeNodeEx`, `PushID`/`PopID`, `Text`, `TreePop` — all standard ImGui widgets from the docking branch. No new ImGui dependencies. ✓
- [x] **ADR-019 (Architecture Boundaries)**: No SDL3, OpenGL, or GLM headers in `src/editor/`. `editor_context.h` forward-declares engine types (`EngineContext`) — no engine header leakage. ✓
- [x] **ADR-011 (Ownership/Nullability/NoDiscard)**: EditorContext holds references (`Editor&`, `EngineContext const&`) which are always valid per C++ language rules. No null checks needed. **Note**: The ADR-011 file (`docs/adr/ADR-011-owner-ship-nullability-lifetime-nodiscard.md`) appears to be empty (0 lines of content). This does not block the current review but is a documentation hygiene concern.
- [x] **ADR-001 (Error Result Pattern)**: `editor.cpp` uses `Result<void>` for fallible operations (`save_scene`, `open_scene`, `save_scene_as`). No new error handling patterns introduced. ✓

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/editor/editor-panels.md`**: Updated with status banner noting F-02 additions. New "Important conventions > EditorContext" section documents the aggregate struct, usage pattern, and lifecycle. Scene Panel description updated from "empty placeholder" to entity tree implementation. F-02 bullet added to v1 foundation list. F-02 spec added to Related specs. Last reviewed updated. ✓
- [ ] **North-star EC-01 contradiction**: The wiki's north-star edge case EC-01 states "Hierarchy shows nothing" for empty scenes, while F-02 implements "No entities" text. The status banner correctly documents current state, but the north-star section should be updated to reflect the implemented behavior ("Shows 'No entities' text") for consistency. Currently deferred as a north-star/future section item — already flagged by wiki-agent.
- [x] **`docs/wiki/editor/scene-management.md`**: Updated with status banner noting F-02 addition of `EditorContext` and `ctx.editor.world()` access pattern. F-02 bullet added to F-01 foundation list. Panel-access note added to Domain Concepts > Editor World entry. Related specs updated. Last reviewed updated. ✓

## Warnings

Non-blocking concerns for awareness:

- **ADR-011 is empty**: The file `docs/adr/ADR-011-owner-ship-nullability-lifetime-nodiscard.md` exists but has zero content. It is referenced by the implementation contract as establishing the "references are always valid" principle. The project should populate or remove this ADR.
- **AC-01 through AC-06 have no automated test coverage**: Six acceptance criteria describing ImGui call verification (`TreeNodeEx` flags, `Text` calls, name formatting) are verified only via code review + manual smoke test. No regression protection exists. Adding `imgui_test_engine` or equivalent capture infrastructure in a future sprint would close this gap.
- **`DefaultOpen` flag on leaf nodes**: The code applies `ImGuiTreeNodeFlags_DefaultOpen` unconditionally to all nodes, including leaf nodes. ImGui ignores this flag on leaf nodes — technically harmless but slightly redundant.
- **`#include <cstdint>` in `editor_context.h`**: Present but unused — the header uses only forward declarations. The contract notes this explicitly as "kept for consistency." Minor.
- **Wiki north-star EC-01** still says "Hierarchy shows nothing" instead of "Shows 'No entities' text." Should be updated when the north-star section is next revised.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- Populate or remove `docs/adr/ADR-011-owner-ship-nullability-lifetime-nodiscard.md` (currently empty).
- Consider updating `docs/wiki/editor/editor-panels.md` north-star EC-01 from "Hierarchy shows nothing" to "Shows 'No entities' text" to match current F-02 behavior.

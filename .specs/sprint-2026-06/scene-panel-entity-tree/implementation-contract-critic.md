# Implementation Contract Review — Scene Panel — Entity Tree (IMPL-F-02)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

None. The contract is complete, consistent with the spec, technically correct, and prescriptive enough to prevent uncontrolled edits by the Code Agent.

## Warnings

Non-blocking concerns for awareness:

- **Null-entity guard not in spec assumptions (minor risk):** The contract adds `entity.id() != EntityId::none()` guard in the root iteration loop. The spec's assumptions (A-01) do not list `EntityId::none()` as an available API. This is a reasonable defensive measure but relies on an engine API not explicitly confirmed in the spec. If `EntityId::none()` does not exist, the Code Agent will need to handle null-entity checking differently (e.g., using a sentinel check or skipping the guard entirely).
- **AC-01 through AC-06 not covered by automated tests:** The spec describes unit-test verification for these ACs (verify `ImGui::TreeNodeEx` flags, `ImGui::Text` calls, entity name formatting). The contract defers to code review + manual smoke tests, citing lack of ImGui call-capture infrastructure. This is a pragmatic choice (already flagged by spec-critic as a non-blocking implementation detail), but it means these six ACs have no automated regression protection. If `imgui_test_engine` or equivalent infrastructure is introduced later, these tests should be automated.
- **Stream-of-consciousness include ordering in Step 6:** The contract's discussion of `menu_bar.h` include ordering contains self-correcting internal reasoning that suggests incomplete pre-analysis. The final outcome (add `editor_context.h` after `command_stack.h`) is correct, but the reasoning text could confuse readers. Minor quality concern only.
- **Combined `DefaultOpen` + `Leaf` flags:** The contract applies `ImGuiTreeNodeFlags_DefaultOpen` unconditionally to all nodes, then adds `ImGuiTreeNodeFlags_Leaf` for leaf nodes. The `DefaultOpen` flag has no effect on leaf nodes (ImGui silently ignores it) — technically correct but slightly redundant. No behavioral impact.
- **Wiki north-star contradiction (EC-01):** The wiki `editor-panels.md` edge case EC-01 says "Empty scene (no entities) | Hierarchy shows nothing" — this conflicts with the spec's requirement to show "No entities" text. The contract correctly follows the spec. The wiki-agent will resolve this during documentation updates.

## Required changes

Concrete, actionable changes requested:

None. All review criteria satisfied.

## Suggested improvements

Optional ideas (not required):

- Consider verifying the availability of `EntityId::none()` or `EntityId::invalid()` in the engine API before relying on it in the null-entity guard. If it does not exist, the guard can use a default-constructed `EntityId{}` comparison or be omitted entirely (the spec assumes `get_root_entity()` never returns null entities).
- Consider adding a brief note in the contract acknowledging that the `DefaultOpen` flag is superfluous (but harmless) for leaf nodes — this would prevent future reviewers from questioning the flag combination.

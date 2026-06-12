# Spec Review — F-03 Entity Selection with Multi-Select

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] None — all Definition of Ready criteria are satisfied.

## Warnings

Non-blocking concerns for awareness:

1. **Callback firing on no-op mutations**: The spec states that `fire_callbacks()` occurs on every call to `select()`, `clear()`, `set_selection()`, and `restore()` (AC-15), but does not specify whether callbacks fire when the mutation results in no net state change (e.g., `restore()` with an identical `Selection`, or `select(id, Replace)` when the same entity is already the only selected one). With no consumers in F-03 this is harmless, but the implementation contract should clarify whether callbacks are gated on actual state change to avoid unnecessary churn in future features (F-05+).

2. **Anchor asymmetry**: `clear()` resets the anchor to `nullopt`, but `set_selection({})` (empty span) leaves the anchor unchanged. This is explicitly documented behavior but is asymmetrical and may surprise implementers. The implementation contract should call this out explicitly.

3. **Double-click edges**: NG-09 says "no double-click behaviour" but does not clarify that the *first click* of a double-click still triggers selection (Replace modifier). This is standard ImGui behavior (`IsItemClicked()` fires on both single and double clicks), but a short note in edge cases would remove ambiguity.

4. **Future Command integration path**: The snapshot/restore API is built for Commands (F-04+), but the current `Command::execute()` signature takes `EngineContext const&`, not `EditorContext const&`. How Commands will access `Editor::selection()` (which requires an `Editor&`) is not addressed. This is explicitly deferred (F-03 does not implement Commands), but the architectural gap should be noted for the implementation contract.

5. **`Selection` container choice**: Assumption A-11 allows the implementation contract to choose a different container than `std::unordered_set<EntityId>`. If a deterministic-order container (e.g., `std::vector` or `std::set`) is chosen, `first()` semantics change from "arbitrary" to "deterministic." The spec documents `first()` as arbitrary, so this is consistent, but the implementation contract must be careful not to create an implicit ordering contract that callers may rely on.

## Required changes

None — the spec is complete and consistent.

## Suggested improvements

1. **Const accessor**: Consider adding `[[nodiscard]] auto selection() const -> EditorSelection const&;` alongside the non-const accessor for const-correct callers (unit tests, query-only use sites).

2. **Callback re-entrancy note**: A brief note in edge cases about what happens if a selection-change callback triggers another selection mutation (even though no consumers exist yet) would future-proof the spec.

3. **`on_change` lifetime**: The callback token model uses `size_t` tokens with no mechanism to detect a token that was already removed. A note about token uniqueness (monotonically increasing `next_token_`) is present in the API sketch — this is sufficient.

## Review summary

**Verdict: Accepted.** The spec is thorough, internally consistent, consistent with existing ADRs (ADR-027, ADR-029), consistent with the F-02 spec (Scene Panel Entity Tree), and consistent with the existing wiki. It satisfies all Definition of Ready criteria:

### Clarity & Completeness ✅
- Scope is clearly bounded (Goals ~ Non-goals ~ Out of scope).
- Dependencies on F-00, F-01, F-02, ImGui, ShortcutRegistry, Entity/World APIs are explicitly identified.
- 15 edge cases and 8 error cases are described.
- Behavior is unambiguous and testable (32 acceptance criteria mapped to verification method).

### Verification ✅
- End-to-end verification defined: headless CI unit tests + manual smoke test + clean build.
- ACs are specific and verifiable (AC-01–19: unit test; AC-20–30: manual; AC-31–32: build/test).
- Success criteria (SC-001–006) and failure states (error cases) are described.

### Documentation ✅
- All API/interface changes are documented with code snippets.
- Existing documentation impact is listed (2 wiki pages).

### Technical ✅
- Technical constraints identified (ImGui APIs, EntityId requirements, ShortcutRegistry, `std::hash<EntityId>`).
- Risks/unknowns surfaced (open questions resolved; performance noted for large scenes).
- Performance implications noted (SC-004: <100ms for 1000+ entities; O(n) tree traversal; efficient span-based `set_selection`).

No blocking issues found. The spec is ready to proceed to implementation contract authoring.

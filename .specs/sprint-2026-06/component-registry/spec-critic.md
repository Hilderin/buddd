# Spec Review — Component Registration & Property System (TypeRegistry redesign)

## Blocking issues

Items that must be resolved before the artifact can be accepted.

*None.* Re-review (2026-06-09): All four changes verified consistent and correct across the spec. Registration examples correctly use overload (B) for CameraComponent/light components and overload (C) for MeshRenderer. `shared_ptr<Model>` is properly listed in G-01 and G-08 (count corrected to "eight"). `is_registered<T>()` now included in the TypeRegistry API table. AC-039 ambiguity remains unresolved (not in scope of this change set). No new issues found.

## Warnings

Non-blocking concerns for awareness:

- **[RESOLVED] G-08 count error** — Previously said "six" but listed 7 types. Now corrected: says "eight" and lists 8 types including `shared_ptr<Model>`. Both G-01 and G-08 now agree on 8 built-in types.

- **[RESOLVED] `is_registered<T>()` not in API table** — Now included in the TypeRegistry key entities API table at line 256. Documentation gap closed.

- **[ ] AC-039 primary requirement vs. acceptable alternative** — AC-039's primary statement says unregistered TypeRegistry operations should produce a *compile error* (`static_assert` or `requires` clause), but the parenthetical says "if SFINAE is used, verify at runtime with error result" is acceptable. These are contradictory mechanisms (compile-time vs. runtime). The spec should clarify which is the intended approach, or explicitly state that either is acceptable as an implementation choice. (Not addressed in this update.)

- **Existing implementation contract is now stale** — The previously accepted implementation contract uses `PropertyType` enum and `PropertyValue` variant as core mechanisms, now superseded by the TypeRegistry-based design. The contract needs rewriting. This is a workflow concern, not a spec defect.

## Required changes

Concrete, actionable changes requested:

1. **[x] Fix G-08 count**: Change "six" to "seven" in G-08. → RESOLVED (now "eight" with 8 listed types)
2. **[ ] Clarify AC-039**: Decide whether unregistered TypeRegistry access is a compile error (static_assert/requires) or runtime error (Result), and update the AC to state one behavior unambiguously. If both are acceptable design choices, say so explicitly. (Not addressed in this update.)
3. **[x] Add `is_registered` to TypeRegistry API table** or remove it from the example code. → RESOLVED (`is_registered<T>()` added to TypeRegistry key entities API table at line 256)

## Suggested improvements

Optional ideas (not required):

- Consider adding a note that `TypeRegistry` pre-registration of built-in types should use lazy initialization (first-call), not static initialization, to avoid static init order fiasco and align with the coordination.md note about "no static initializers". The spec already offers "one-time lazy init" as an option; a recommendation would be helpful.
- EC-03 mentions deregistration ("if deregistration is added") — consider removing this speculative edge case since deregistration is not in scope, or mark it explicitly as future-only speculation.

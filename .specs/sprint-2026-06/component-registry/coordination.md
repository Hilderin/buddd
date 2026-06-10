# Workflow Coordination: component-registry

## Orchestrator

**Feature**: `component-registry`
**Status**: in-progress
**Current step**: awaiting-human-validation
**Initial instructions**: Create a component registration system that enables: (1) registering components with their type name and properties, (2) YAML serialization/deserialization of component properties, (3) editor property inspection/editing (UI deferred). Covers property type system, property descriptors, component descriptors, component registry, YAML::convert specializations for engine math types, YAML serialization helpers, and registration of all existing engine components.
**Notes**:
- Scope agreed with human on 2026-06-09. In scope: property type system, property descriptors, component descriptors, component registry, YAML::convert for engine math types, YAML serialization helpers, registration of existing components (CameraComponent, PointLightComponent, DirectionalLightComponent, SpotLightComponent, MeshRenderer), ADR, unit tests, wiki updates. Out of scope: scene file format, editor property panel UI, prefabs, entity references, versioning, Transform special handling.
- Naming: Explicit string names for component types (e.g. "camera", "point_light"). String-only type identification (no enum).
- Error handling: strict validation — error Result on unknown component type, unknown property name, or invalid/out-of-range values.
- YAML round-trip verified via in-memory YAML (no file I/O in tests).
- Registry location: `src/engine/scene/component_registry/`.
- Property accessor pattern: explicit get/set lambdas (not mutable-ref accessor).
- YAML::convert specializations defined alongside each type.
- Registration timing: manual startup init (no static initializers).
- Registration pattern: self-contained per-component registration functions, called from a central `register_all_components()`.
- ADR created within this workflow.

## spec-author

**Status**: completed
**Summary**:
Spec updated with 3 improvements: (1) All 5 TypeRegistry callbacks now receive `const SerializationContext&` for context-dependent operations (e.g. asset resolution). (2) `std::shared_ptr<Model>` registered as a built-in TypeRegistry type — MeshRenderer uses `add_property<std::shared_ptr<Model>>()` instead of string-based asset IDs, TypeRegistry handles encode/decode via ctx.assets internally. (3) Three `add_property` overloads on ComponentInfo<T>: (A) convention-based (deferred for v1), (B) simple lambdas, (C) context-aware lambdas. Registration examples updated to use overload (B) and (C). All AC, SC, edge cases, assumptions, and open questions updated accordingly.
**Artifacts**:
- `.specs/sprint-2026-06/component-registry/spec.md`
**Questions for human**:
none
**Warnings**:
The existing implementation contract at `.specs/sprint-2026-06/component-registry/implementation-contract.md` references `model_asset_id_` and `add_property<std::string>` patterns that are now superseded by the spec changes. The contract needs rewriting to align with the new spec.
**Blocking issues**:
none

## spec-critic

**Status**: completed
**Summary**:
Re-review (2026-06-09, second loop): All four changes verified consistent — TypeRegistry lambdas receive `const SerializationContext&`, `shared_ptr<Model>` pre-registered as a built-in type, three `add_property` overloads (A/B/C) correctly defined and exemplified, CameraComponent/light components use overload (B), MeshRenderer uses (C). No contradictions across the spec. No remaining references to removed concepts. Two previous warnings resolved (G-08 count now "eight" with 8 types, `is_registered` added to API table). AC-039 ambiguity remains as an open warning.
**Artifacts**:
- `.specs/sprint-2026-06/component-registry/spec-critic.md`
**Questions for human**:
none
**Warnings**:
- AC-039: primary requirement (compile error via static_assert/requires) contradicts parenthetical exception (runtime error via Result). Spec-author should clarify which is intended. (Not addressed in this update.)
- Existing implementation contract uses PropertyType enum and PropertyValue as core mechanisms — now inconsistent with the TypeRegistry-based spec. Needs rewriting (workflow concern).
**Blocking issues**:
none

## implementation-contract-author

**Status**: completed
**Summary**:
Contract updated: TypeRegistry lambdas receive SerializationContext&, shared_ptr<Model> registered as a type, three add_property overloads, MeshRenderer simplified.
**Artifacts**:
- `.specs/sprint-2026-06/component-registry/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**: Re-review (2026-06-09, fixes verified): Both blocking issues are resolved. MeshRenderer setter now has full AssetManager integration (stores asset ID via `model_asset_id_`, resolves via `ctx.assets.create<ModelAsset>(id)`). `all_types()` thread safety fixed (mutable member cache with lazy rebuild). All 7 previous warnings addressed (near/far table, yaml-cpp include, span include, Property stubs doc, designated init note, mock helper, dead set_model). 2 new non-blocking concerns noted (getter differs from spec AC-014 pattern, setter destructively moves Model out of cached ModelAsset). Contract is now ACCEPTED.

Quick re-review (2026-06-09): All 6 verification checks pass — no contradictions with spec, all 3 overloads correctly documented, MeshRenderer simplification complete (no `model_asset_id_`), `AssetManager::find_asset_id`/`resolve_model` documented, no compile-time static_assert claims for unregistered types (all runtime), file lists/build/test structure consistent. One minor doc nit: line 951 says "seven" built-in types instead of "eight". Verdict unchanged: ACCEPTED.
**Artifacts**:
- `.specs/sprint-2026-06/component-registry/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- `TypeRegistry::register_type<T>(TypeInfo<T>)` API shape uses struct, differs from spec's 5-callback signature (architecturally equivalent).
- CameraComponent property setters each call `set_perspective()` with current values of other properties — round-trip depends on component state.
- MeshRenderer setter moves Model out of cached ModelAsset — destructive to asset cache if multiple components reference the same model. Suggestion: use non-destructive approach or accept as v1 limitation.
- Line 951 says "Pre-register the **seven** built-in types" but spec G-08 lists 8 types (includes `shared_ptr<Model>`). Comment should say "eight".
**Blocking issues**:
none

## Human Validation

**Status**: pending
**Approver**: pending
**Date**: pending
**Warnings**:
none
**Notes**: pending

## code-implementer

**Status**: pending
**Summary**:
pending
**Artifacts**:
- pending
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## code-reviewer

**Status**: pending
**Summary**:
pending
**Artifacts**:
- `.specs/sprint-2026-06/component-registry/code-review.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## wiki-agent

**Status**: pending
**Summary**:
pending
**Artifacts**:
- pending
**Changes made**:
pending
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## governance-reviewer

**Status**: pending
**Summary**:
pending
**Artifacts**:
- `.specs/sprint-2026-06/component-registry/governance-review.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above.
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.

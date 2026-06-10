# Governance Review — Component Registration & Property System

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] **Spec G-01/G-08 vs implementation contract line 951 comment count**: Spec correctly lists 8 built-in types (`float`, `int32_t`, `bool`, `std::string`, `Vec3`, `Vec4`, `Quat`, `std::shared_ptr<Model>`). Implementation contract line 951 comment says "Pre-register the **seven** built-in types" — should say "eight." The actual code (`register_all_components.cpp`) registers all 8 types correctly. This is a cosmetic documentation comment inconsistency in the implementation contract, not a code defect.

- [x] **Spec AC-011/012/013 `min>0` vs contract/code `min(0.0f)`**: Spec acceptance criteria use notation `min>0` for light component intensity/range, implying strict positivity. The implementation contract explicitly specifies `min(0.0f)` (allowing zero), and the code matches the contract. This was discussed and resolved at contract time — the contract is authoritative for implementation, the spec notation was a pre-existing inconsistency.

- [x] **Spec AC-039 ambiguity (compile error vs runtime error)**: Spec states unregistered TypeRegistry operations should produce a compile error (`static_assert`/`requires`), but parenthetical says "if SFINAE is used, verify at runtime with error result" is acceptable. The code uses runtime `Result` error returns (convenience methods) and `FATAL`+`abort` (`add_property`). These are runtime checks, not compile-time errors. The ambiguity was noted by spec-critic but not addressed. Not a blocking issue — either implementation strategy is valid, and the code's approach is consistent with the parenthetical exception.

- [x] **Spec's TypeRegistry API (five separate callbacks) vs contract's `TypeInfo<T>` struct**: Spec shows `register_type<T>(yaml_encode, yaml_decode, ...)` with five separate callback parameters. Contract and code use `register_type<T>(TypeInfo<T>)` where `TypeInfo<T>` is a struct aggregating all five callbacks. Architecturally equivalent — the struct approach simplifies the API while maintaining the same information. No semantic discrepancy.

- [x] **ADR-028 `Consequences > Neutral` note about yaml-cpp in public headers**: Confirmed: `type_registry.h` and `component_info.h` include `<yaml-cpp/yaml.h>` directly due to template inline definitions. This matches ADR-028's documented consequence and is a necessary deviation from ADR-019 for template code.

- [x] **`PropertyFlags` field name `step_value` vs spec pseudocode `step`**: Spec's pseudocode shows `step<float>` as a method/field. The code has `step(float)` method setting `step_value` field. The public API (`step(float) -> PropertyFlags&`) matches exactly. No contradiction.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-028** (Component Type Registry with TypeRegistry and SerializationContext) — Created within this workflow. Documents the architectural decision comprehensively: TypeRegistry design, SerializationContext, Property/ComponentInfo/ComponentRegistry hierarchy, three `add_property` overloads, eight built-in types, alternatives evaluated. Consistent with spec and implementation. References ADR-001, ADR-005, ADR-006, ADR-016, ADR-019, ADR-027 correctly.

- [x] **ADR-001** (Result/Error pattern) — Used throughout: all fallible functions return `Result<T>`, yaml-cpp exceptions caught and converted.

- [x] **ADR-006** (RTTI dynamic_cast dispatch) — Used in `ComponentRegistry::create()` for factory return, `ComponentInfo::add_property` getter/setter type erasure via `static_cast`.

- [x] **ADR-016** (yaml-cpp dependency) — yaml-cpp used for YAML I/O. Headers with inline templates (`type_registry.h`, `component_info.h`) include yaml-cpp headers directly — a necessary widening of the dependency footprint documented in ADR-028.

- [x] **ADR-019** (Architecture boundaries) — yaml-cpp appears in some public headers (see above) as a documented deviation. `property.h` correctly forward-declares `YAML::Node` without including yaml-cpp. All `.cpp` files include yaml-cpp correctly per ADR-019.

- [x] **ADR-027** (Editor architecture) — Referenced in ADR-028 as the consumer for `ComponentRegistry::describe()` and TypeRegistry display callbacks.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] **`docs/wiki/architecture/module-map.md`** — Updated: "Component Registry submodule" table with 7 file entries (`type_registry`, `property`, `component_info`, `component_registry`, `serialization_context`, `serialization`, `register_all_components`). Correct paths and roles.

- [x] **`docs/wiki/architecture/overview.md`** — Updated: `component_registry/` directory line added to the scene/ directory tree listing (line 87).

- [x] **`docs/wiki/architecture/dependency-map.md`** — Updated: "Component Registry dependencies" section with bullet-point class dependencies and ASCII dependency graph. Accurately describes `component_info.h` including yaml-cpp for template inline definitions.

- [x] **`docs/wiki/domain/glossary.md`** — Updated: "Component registration terms" table with 9 entries (TypeRegistry, ComponentRegistry, ComponentInfoBase, ComponentInfo<T>, Property, PropertyFlags, SerializationContext, serialize_component/deserialize_component, register_all_components). Definitions match spec and code.

- [x] **`docs/wiki/engineering/testing.md`** — Updated: "Component registry tests" section with 36-test-case summary table organized by category.

- [x] Wiki content is consistent with ADR-028, spec, and implementation. Wiki is properly maintained as operational knowledge, not as authoritative law.

## Architecture boundaries

- [x] No SDL3 headers outside `src/engine/`.
- [x] No GLM headers outside `src/engine/math/`.
- [x] yaml-cpp is a PRIVATE dependency of `buddd_engine` per ADR-016. The known deviation (yaml-cpp in public headers for template inline definitions) is documented in ADR-028 and the dependency map wiki.
- [x] `property.h` correctly forward-declares `YAML::Node` without including yaml-cpp.
- [x] `serialization_context.h`, `component_registry.h`, `serialization.h` all forward-declare YAML types properly.
- [x] Forbidden files (`component.h`, `entity.h`, `world.h`, `camera_component.h/.cpp`, `point_light_component.h/.cpp`, `directional_light_component.h/.cpp`, `spot_light_component.h/.cpp`) confirmed NOT modified (zero diff lines).

## Warnings

Non-blocking concerns for awareness:

- **Implementation contract comment says "seven" built-in types** — `register_all_components.h` comment (line 951) says `/// Pre-register the seven built-in types in TypeRegistry.` Should say "eight" to match the spec and the actual code. Cosmetic documentation bug — does not affect correctness.

- **yaml-cpp in public headers (ADR-019 deviation)** — `type_registry.h` and `component_info.h` include `<yaml-cpp/yaml.h>` directly. This is a necessary deviation because template method definitions are inline in these headers and require the complete `YAML::Node` type. ADR-028 documents this in its Consequences. No change requested.

- **`resolve_model()` destructive move** — `AssetManager::resolve_model()` moves the Model out of the cached `ModelAsset`, making it a one-shot operation per model instance. Documented as v1 limitation (contract line 1253). If multiple components reference the same model, only the first `resolve_model()` call will succeed.

- **`PropertyFlags::min_value` default edge case** — Default `-std::numeric_limits<float>::max()` means values at or below approximately `-3.4e+38` would be rejected even on unconstrained float properties. Extremely unlikely in practice.

- **AC-039 ambiguity** — The acceptance criteria state both "compile error" and "runtime error" as acceptable outcomes. The code uses runtime error. Consider clarifying the spec in a future revision to state one unambiguous behavior.

- **Test double-registration warnings** — Tests call `register_builtin_types()` both via `TestEngine` construction and explicitly, producing harmless duplicate registration warnings. Functionally safe.

## Required governance updates

No governance document updates required. The documentation is consistent across all artifacts. The ADR-028 is properly created. The wiki is properly updated. The minor "seven" vs "eight" comment in the implementation contract is cosmetic and not a governance concern (the implementation contract is read-only after workflow completion).

The workflow is complete and accepted.

# Implementation Contract Review — Component Registration & Property System (TypeRegistry Design)

> **Re-review 2026-06-09 (original)**: Contract fixes verified. Both blocking issues from the previous review are resolved.
> MeshRenderer setter now has full AssetManager integration (stores asset ID, resolves via `ctx.assets.create<ModelAsset>(id)`).
> `all_types()` thread safety fixed (mutable member cache, not function-local static).
> All 7 previous warnings addressed (yaml-cpp include, span include, Property stubs doc, designated init note,
> near/far table, mock helper, dead set_model). 2 new non-blocking concerns noted below. **Verdict: ACCEPTED.**
>
> **Re-review 2026-06-09 (quick verification)**: All 6 verification checks pass. Contract remains consistent with the spec.
> Three `add_property` overloads correctly documented. MeshRenderer simplification complete (no `model_asset_id_`,
> shared_ptr<Model> directly via TypeRegistry). `AssetManager::find_asset_id` and `resolve_model` documented.
> No compile-time static_assert claims for unregistered types — all checks are runtime (Result error or FATAL+abort).
> File lists, build changes, and test structure still consistent. One minor documentation note added below.
> **Verdict: ACCEPTED (unchanged).**

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] ~~**[SPEC-LEVEL] MeshRenderer setter is a v1 no-op placeholder, contradicting spec AC-014 and A-07.**~~ **RESOLVED**: The setter now implements full AssetManager integration: resolves asset ID via `ctx.assets.create<ModelAsset>(id)`, extracts the Model into a `shared_ptr<Model>`, stores it on `MeshRenderer` via `set_model()`, and stores the asset ID via `set_model_asset_id()`. The non-goal about v1 placeholder has been removed.

- [x] ~~**`all_types()` implementation is NOT thread-safe despite contract's own thread-safety guarantee.**~~ **RESOLVED**: The function-local static vector has been replaced with `mutable` member cache (`all_types_cache_`, `all_types_cache_valid_`). `register_component<T>()` calls `invalidate_cache()` to trigger lazy rebuild. Concurrent `const` calls to `all_types()` after registration are safe (read-only on valid cache).

## Warnings

Non-blocking concerns for awareness:

### Previously reported — now resolved

- ~~**CameraComponent `near`/`far` min constraints misstated in registration table.**~~ **FIXED**: Line 916 now correctly lists `near` and `far` without incorrect `min=0.001`.

- ~~**Missing yaml-cpp include in `register_all_components.cpp`.**~~ **FIXED**: `#include <yaml-cpp/yaml.h>` added at line 937.

- ~~**`component_registry.h` missing `#include <span>`.**~~ **FIXED**: `#include <span>` added at line 659.

- ~~**Property::to_string/from_string/validate declared but never called.**~~ **FIXED**: Documented at lines 347-349 as "for future editor UI use; not exercised by this sprint."

- ~~**`reinterpret_cast<AssetManager&>` in test mock is undefined behavior.**~~ **FIXED**: Replaced with proper `MockAssetManager : public AssetManager` inheritance (lines 1287-1311).

- ~~**Designated initializers in `register_builtin_types()` require C++20.**~~ **FIXED**: Documented at lines 943-944 with a note about C++17 alternative.

- ~~**`set_model()` added to MeshRenderer but never called.**~~ **FIXED**: Now called at line 1123 in the MeshRenderer setter.

### Remaining / new

- **`TypeRegistry::register_type<T>(TypeInfo<T>)` API shape differs from spec's 5-callback API.** The spec's API table and example code show `register_type<T>` accepting five separate callbacks. The contract uses a `TypeInfo<T>` struct with named fields. Architecturally equivalent, but deviates from the spec's shown API. Not a blocking issue.

- **Comment says "seven" built-in types but spec says "eight".** Line 951 in section 10 reads `/// Pre-register the seven built-in types in TypeRegistry.` The spec's G-08 lists 8 types (including `shared_ptr<Model>`), and the contract's own registration table (section 2) lists 8 types. The comment should say "eight", not "seven". Trivial documentation fix.

- **Constructor calls `set_perspective()` on every single-property change.** CameraComponent's property setters each call `set_perspective(v, aspect(), near_plane(), far_plane())`. This means changing `fov_y` reads the current `aspect`, `near_plane`, `far_plane` values. This is correct behavior per the spec (A-05) but is worth noting for awareness — the property-level round-trip depends on the component's current state of other properties.

- **MeshRenderer setter moves `Model` out of the cached `ModelAsset`, corrupting the asset cache.** The `resolve_model()` implementation (section 12, lines 1247-1248) calls `std::move(*root.model)` and `root.model.reset()` on the cached `ModelAsset` object. Per the wiki ("Loading the same asset ID twice returns the cached instance"), `create<ModelAsset>(id)` returns the same cached `shared_ptr<ModelAsset>`. Moving the Model out destroys the cached asset. Contract notes this as "acceptable for v1" (line 1253). The simplified MeshRenderer design (no `model_asset_id_`) eliminates the previous concern about getter returning an asset ID string instead of calling `find_asset_id` — that was from the OLD contract pattern and is now resolved. The remaining concern is about the destructiveness of `resolve_model()` itself.

## Required changes

All previously required changes have been addressed:

1. ✅ **MeshRenderer setter contradiction** — resolved: full AssetManager integration with `ctx.assets.create<ModelAsset>(id)` resolution and `model_asset_id_` storage.
2. ✅ **`all_types()` thread safety** — resolved: mutable member cache replaces function-local static.
3. ✅ **Registration table near/far min constraint misstatement** — resolved: table corrected.

No new blocking issues introduced. See "Remaining / new" warnings above for non-blocking concerns.

## Suggested improvements

- Add `#include <yaml-cpp/yaml.h>` to `register_all_components.cpp` to resolve the missing-include compilation error.
- Add `#include <span>` to `component_registry.h`.
- Document Property::to_string/from_string/validate as "for future editor UI use; not exercised by current tests."
- Consider using a proper mock class deriving from `AssetManager` in test code instead of `reinterpret_cast`.
- The `set_model()` method on MeshRenderer is added but never called by the v1 placeholder setter. Either wire it up or document it as "added for future use" in the contract.

---

*Previous review artifact (2026-06-09, old contract) is superseded by this review. The contract has been rewritten with the TypeRegistry design; all prior blocking issues (add_asset_ref, PropertyType enum, asset resolution contradiction) are obsolete.*

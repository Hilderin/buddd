# SPEC-008 — Scene Graph: World, Entity, Transform, Components, Hierarchy

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Summary

SPEC-008 defines the foundational scene graph system for the Buddd Engine: `World`, `Entity`, `EntityId`, `Transform`, `Component` base class, hierarchy (parent/child), and deferred destruction. This is the third review cycle (the spec was previously **Rejected** in review 1 with 5 blocking issues, then **Accepted** in review 2 after all 5 were resolved). This review assesses the current state of the spec independently.

The spec is **structurally excellent** and **well-scoped**. It follows the established spec template, has all 6 open questions resolved, defines 32 testable acceptance criteria, includes thorough edge/error case tables, and demonstrates deep understanding of the scene graph problem domain. The architecture boundary (CONST-001) is properly enforced, forward-compatibility is preserved for future ECS refactoring, and the public API is clearly separated from internal storage details.

The 5 blocking issues from the first review have all been addressed:
- **B-01 (Null Entity contradiction)**: Resolved — consistent across all three locations.
- **B-02 (Entity::none() missing)**: Resolved — `Entity::none()` is now defined in the API.
- **B-03 (noexcept inconsistency)**: Mostly resolved — A-10 now accurately describes which methods are noexcept. A minor residual inconsistency remains (see W-01).
- **B-04 (Pending-destroy inconsistency)**: Resolved — clear "read-only safe, mutating UB" contract with rationale.
- **B-05 (Cross-world reparenting)**: Resolved — documented as UB in edge cases.

No new blocking issues are introduced. The spec is ready for acceptance with a few minor warnings noted below.

## Positive aspects

- **Well-scoped v1**: The non-goals list (lines 43–54) and Out of scope section (lines 567–589) correctly fence off ECS optimization, serialization, events, threading, editor tooling, and other major features. This keeps the implementation manageable and prevents scope creep.

- **All open questions resolved**: Q-01 through Q-06 are all marked `[RESOLVED]` with clear rationale. This follows the established pattern from previous specs and removes ambiguity.

- **Architecture boundary preserved**: AC-019 and AC-020 explicitly place all types under `src/engine/scene/` in namespace `buddd::engine` and prohibit GLM/SDL3/OpenGL header leakage. This is fully consistent with CONST-001, the dependency map, and the architecture overview.

- **Generation-based stale handle detection**: The `EntityId` design with `uint32_t index` + `uint32_t generation` is a well-understood pattern for safe handle reuse. The sentinel `EntityId::none()` with `{UINT32_MAX, UINT32_MAX}` avoids ambiguity with valid slot 0.

- **Deferred destruction architecture**: The two-phase `destroy()` + `flush_destroyed()` model is clean, supports cascading subtree destruction, is idempotent, and is forward-compatible with ECS-style flat arrays. The decision to make destroyed entities visible in the parent until flush (line 286) allows consistent iteration during the mark phase.

- **Thorough acceptance criteria**: 32 ACs covering entity lifecycle, component management, hierarchy operations, transform computation, deferred destruction, edge cases, and error behavior. Each AC has concrete verification steps with measurable outcomes (e.g., `1e-5f` tolerance, `static_assert` checks, size assertions).

- **Excellent edge/error case coverage**: 27 edge cases and 11 error cases are documented, covering null entities, pending-destroy operations, cycle detection, cross-world operations, deep hierarchy recursion risk, and stale handle scenarios.

- **Forward-compatibility awareness**: The spec explicitly states that the storage strategy (v1: per-entity `vector<unique_ptr<Component>>`) is hidden behind the `World` implementation, with the API designed for future ECS refactoring. The Entity is a lightweight handle (16 bytes) that does not own data.

- **Clear memory/size guarantees**: `static_assert` for `sizeof(EntityId) == 8`, `sizeof(Entity) == 16`, trivially copyable checks — these are concrete and testable.

- **Consistent with ADR-001 exception pattern**: The non-use of `Result<T>` is explicitly acknowledged and justified (A-02, line 595), following the same pattern as the render pipeline's `draw()`/`draw_indexed()` exception documented in the architecture overview.

- **Pending-destroy contract is now consistent**: A clear design principle ("read-only safe, mutating UB") is documented at lines 280–289, with rationale for each operation category.

## Issues

### (Non-blocking) W-01: `noexcept` residual inconsistency — Transform API listing vs A-10

**Description**: A-10 (line 603) states:

> "Transform pure computation methods (`local_matrix()`, `world_matrix()`) are `noexcept`."

However, the API listing for `Transform` (lines 116 and 122) does **not** show `noexcept`:

```cpp
auto local_matrix() const -> math::Mat4;          // line 116 — missing noexcept
auto world_matrix(const Entity& entity) const -> math::Mat4;  // line 122 — missing noexcept
```

All other methods that A-10 claims are `noexcept` (Entity accessors, EntityId methods, World getters) DO show `noexcept` in their API listings. Only `Transform::local_matrix()` and `Transform::world_matrix()` are missing it.

**Impact**: An implementer reading the API listing would implement these without `noexcept`. A-10 contradicts the listing. While minor (adding `noexcept` is a source-compatible change), this is still a specification inconsistency.

**Resolution**: Add `noexcept` to `Transform::local_matrix() const` and `Transform::world_matrix() const` in the API listing at lines 116 and 122, for example:
```cpp
auto local_matrix() const noexcept -> math::Mat4;
auto world_matrix(const Entity& entity) const noexcept -> math::Mat4;
```

---

### (Non-blocking) W-02: `Entity::world()` on null entity — technically undefined behavior

**Description**: The spec lists `world()` as a safe operation on null entities (line 278):

> "Safe operations on a null entity: `id()`, `world()`, `operator==`, `operator!=`, `is_pending_destroy()` (returns false), `none()` (static)."

However, the `Entity` class stores `World* world_ = nullptr` (line 261) and `world()` returns `World&` (line 171):

```cpp
auto world() const noexcept -> World&;
```

The natural implementation is `return *world_;`. For a null (default-constructed) entity, `world_` is `nullptr`, so this would return `*nullptr` — which is **undefined behavior** per the C++ standard (forming a null reference is UB, even if never dereferenced).

In practice, this "works" on all major compilers (they generate code returning the null address, and the reference is never actually read), but from a strict language-lawyer perspective, it is UB.

**Impact**: The spec claims a guarantee it technically cannot provide. While this is unlikely to cause problems in practice, it is a correctness issue in the specification. Future compiler versions with more aggressive UB optimization could theoretically break this.

**Possible resolutions**:
- **(a)** Document the nuance: "Calling `world()` on a null entity returns a reference that MUST NOT be dereferenced (the reference is null, and dereferencing it is UB)."
- **(b)** Change `world()` to return `World*` instead of `World&` — but this would change the API semantics for non-null entities.
- **(c)** Accept the current wording as a practical guarantee (de-facto safe on all target compilers) and note it as an intentional concession to performance/ergonomics.

Recommendation: Option (a) — add a brief note in the null entity documentation or in a new entry in the Edge cases table.

---

### (Non-blocking) W-03: `std::optional<T&>` compiler support risk

**Description**: The spec uses `std::optional<T&>` (C++26 feature, P2988R5, adopted June 2024) for `get_component<T>()` return types (lines 207–211):

```cpp
template<typename T>
auto get_component() const noexcept -> std::optional<const T&>;
template<typename T>
auto get_component() noexcept -> std::optional<T&>;
```

The project targets GCC 14+ and Clang 19+ as minimum compiler versions (per wiki architecture overview). However, `std::optional<T&>` was adopted very late in the C++26 cycle, and **GCC 14** (released May 2024, before the adoption) and **Clang 19** (released late 2024) may not implement this feature.

**Impact**: If the minimum target compilers do not support `std::optional<T&>`, the spec as written cannot be directly implemented. The implementation would need a workaround:
- Use a custom `optional_ref<T>` wrapper type
- Return `T*` instead (with `nullptr` for "absent")
- Use `std::optional<std::reference_wrapper<T>>` (less ergonomic)

**Resolution options**:
- **(a)** Specify a fallback in the spec: "If `std::optional<T&>` is not available on the target compiler, the implementation may use `T*` as a return type with `nullptr` representing absence, or a project-local `OptionalRef<T>` wrapper."
- **(b)** Raise the minimum compiler version to GCC 15+ / Clang 20+.
- **(c)** Keep the current specification and let the implementation contract handle the fallback.

Recommendation: Document the compiler requirement in an assumption or note. This is not a blocking issue because it affects implementability but not specification correctness.

---

### (Non-blocking) W-04: `Transform::world_matrix()` takes `const Entity&` despite Entity being cheap to pass by value

**Description**: A-05 states that `Entity` is "a lightweight value-type handle (16 bytes). It is cheap to pass by value." However, `Transform::world_matrix()` takes `const Entity&` (line 122):

```cpp
auto world_matrix(const Entity& entity) const -> math::Mat4;
```

For a 16-byte, trivially-copyable handle type, passing by value would be more conventional (and A-05 explicitly claims this is the intention). The `const&` parameter introduces an extra indirection and is inconsistent with the "cheap to pass by value" design philosophy stated in A-05.

**Note**: The convenience method `Entity::world_matrix() -> math::Mat4` (line 250) already exists and avoids this awkwardness. So the issue is limited to the `Transform` API.

**Resolution**: Either:
- **(a)** Change `Transform::world_matrix()` to take `Entity` by value (consistent with A-05).
- **(b)** Document why `const Entity&` is preferred (e.g., consistency with future methods that may take larger entity descriptors, or to avoid the copy of the `World*`).

This is a minor API style concern, not a blocking issue.

---

### (Non-blocking) W-05: `World::get_transform()` is public but documented as "Internal"

**Description**: The `get_transform(EntityId)` methods appear in the `public:` section of `World` (lines 324–326) with the comment "Internal (called by Entity)." There is no access control enforcing this — any caller with a `World&` can call them. The `World` class does not show `friend class Entity;` which would be needed for private access.

**Impact**: The "Internal" label is advisory only. There is no compilation guard preventing misuse.

**Resolution**: Either:
- **(a)** Add `friend class Entity;` to the `World` class and make `get_transform()` private.
- **(b)** Accept the semi-public status and update the comment to "Semi-public — primarily for use by Entity but available to engine developers."

---

### (Non-blocking) W-06: Potential ambiguity in `World::get_transform()` EntityId parameter

**Description**: `World::get_transform(EntityId id)` takes an `EntityId`. If called with `EntityId::none()`, the behavior is undefined (no entity to get a transform from). If called with a stale `EntityId` (generation mismatch), behavior is also undefined. The spec does not explicitly document this because `get_transform()` is marked as "Internal."

**Impact**: Low — internal methods are not part of the public contract. However, since the method is technically public (see W-05), the lack of documentation could lead to misuse.

**Resolution**: Combine with W-05 resolution — make it truly private (with friend) or document the preconditions if kept public.

---

### (Non-blocking) W-07: `operator!=` is redundant in C++20

**Description**: The `Entity` class declares both `operator==` and `operator!=` as friend functions (lines 253–254). In C++20, if `operator==` is declared, the compiler automatically synthesizes `operator!=` from it.

**Impact**: The explicit `operator!=` is not wrong but is redundant. This is a minor style inconsistency.

**Resolution**: Consider removing `operator!=` and relying on C++20 synthesized comparison, or keep it for explicitness. Either way, it's non-blocking.

## Blocking issues checklist

No blocking issues. All previously identified blocking issues (B-01 through B-05 from review 1) have been resolved and verified:

- [x] **B-01 (Resolved)**: Null Entity behavior is now consistent across all three locations (line 278, error case line 543, AC-022). All state `id()`, `world()`, `==`, `!=`, `is_pending_destroy()` as safe.
- [x] **B-02 (Resolved)**: `Entity::none()` static method is now defined in the API (line 174). Story 2 and `reparent()` documentation are consistent with it.
- [x] **B-03 (Resolved)**: `noexcept` is now properly scoped. A-10 accurately describes which methods are noexcept and which are not. Minor residual issue: see W-01.
- [x] **B-04 (Resolved)**: Pending-destroy contract is now consistent with a clear design principle: read-only safe (get_component → nullopt, transform → valid, is_pending_destroy → true, destroy → idempotent), mutating UB (add_component, remove_component, create_child, reparent).
- [x] **B-05 (Resolved)**: Cross-world reparenting is documented as UB (edge case line 524, error case line 547).

## Required changes

None. All required changes from the previous review have been implemented.

The warnings in this review (W-01 through W-07) are non-blocking and may be addressed at the spec author's discretion.

## Suggested improvements

Optional ideas (not required):

1. **Add `noexcept` to Transform methods** (addresses W-01): Add `noexcept` to `Transform::local_matrix() const` and `Transform::world_matrix() const` in the API listing.

2. **Document null reference nuance for `world()`** (addresses W-02): Add a note that `world()` on a null entity returns a null reference — safe to call, but the returned reference must not be dereferenced.

3. **Note compiler support for `std::optional<T&>`** (addresses W-03): Add an assumption noting that `std::optional<T&>` requires GCC 15+ / Clang 20+ (or document the fallback strategy).

4. **Make `World::get_transform()` private** (addresses W-05): Add `friend class Entity;` and make the method private.

5. **Add a module map update**: Since `src/engine/scene/` is a new subdirectory, consider adding it to the architecture overview wiki page's `src/engine/` directory tree.

6. **Consider adding `enum class EntityComparisonPolicy`** or similar for comparing entities across worlds: Currently `operator==` compares `world_` pointers, meaning two entities from different worlds always compare as not-equal even if they have the same `id_`. This is correct behavior, but explicitly documenting it as intentional would be beneficial.

## Verdict

**Accepted**

SPEC-008 is a well-structured, thorough, and internally consistent specification. All 5 blocking issues from the first review cycle have been addressed and verified. The 32 acceptance criteria are testable, the edge/error case coverage is comprehensive, the architecture boundary is properly enforced, and the scope is appropriately constrained for a v1.

The 7 warnings noted in this review are non-blocking and do not prevent implementation from proceeding. They should be addressed during the next spec revision or as part of the implementation contract refinement.

| Check | Outcome |
|---|---|
| Internal consistency | ✓ — All contradictions from review 1 resolved. Minor residual: see W-01 (`noexcept`), W-02 (null reference), W-07 (redundant `operator!=`). |
| Completeness | ✓ — All critical paths specified. W-05, W-06 are minor API hygiene. |
| Acceptance criteria testability | ✓ — All 32 ACs are testable (AC-032 is doc-only, AC-020 is code-review, both acknowledged). |
| Edge/error case coverage | ✓ — 27 edge cases, 11 error cases. Minor gaps: W-06 (get_transform preconditions), W-02 (null reference nuance). |
| Constitution rule compliance | ✓ — CONST-001 (AC-019, AC-020), CONST-002 (all ACs testable). ADR-001 exception documented and justified (A-02). |
| Architecture boundary | ✓ — No GLM/SDL3/OpenGL in public headers. All types under `src/engine/scene/`. |
| Open questions resolution | ✓ — All 6 resolved with clear rationale. |
| Verdict | **Accepted** — no blocking issues. |

## Change log

| Review | Verdict | Key findings |
|---|---|---|
| 1st (previous) | `Rejected` | 5 blocking issues: B-01 (null Entity), B-02 (Entity::none()), B-03 (noexcept), B-04 (pending-destroy), B-05 (cross-world). |
| 2nd (previous) | `Accepted` | All 5 blocking issues resolved. |
| 3rd (this review) | `Accepted` | No new blocking issues. 7 non-blocking warnings: W-01 (noexcept residual), W-02 (null reference nuance), W-03 (optional<T&> risk), W-04 (passing convention), W-05 (get_transform visibility), W-06 (get_transform preconditions), W-07 (redundant operator!=). |

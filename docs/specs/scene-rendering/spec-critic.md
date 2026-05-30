# Spec Review — Scene-Based Rendering (SPEC-011)

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

None.

## Warnings

None.

## Required changes

None.

## Suggested improvements

None.

## Verification checklist

### 1. `reference_wrapper` removal

| Item | Expected | Actual | Verdict |
|---|---|---|---|
| `active_camera()` return type | `std::optional<CameraComponent&>` | `std::optional<CameraComponent&>` (line 249) | ✅ |
| Internal storage | `std::optional<CameraComponent&>` | `std::optional<CameraComponent&> active_camera_` (line 257) | ✅ |
| Include for `world.h` | `<optional>` only (no `<functional>`) | `<optional>` (line 758, was `<functional>` and `<optional>` — fixed) | ✅ |
| No `std::reference_wrapper` in spec | Zero occurrences | 0 matches in spec.md | ✅ |
| No `std::ref()` in spec | Zero occurrences | 0 matches | ✅ |
| No `->get()` for `reference_wrapper` in spec | Zero occurrences | 0 matches (`.get()` calls are `unique_ptr::get()`, not `reference_wrapper::get()`) | ✅ |

### 2. Access pattern updates

| Item | Expected | Actual | Verdict |
|---|---|---|---|
| `register_camera()` stores reference | `active_camera_ = camera` (no `std::ref`) | "stores a reference to the CameraComponent in `active_camera_`" (line 261) | ✅ |
| `unregister_camera()` compares addresses | `&*active_camera_ == &camera` | "clears `active_camera_` only if it refers to the same component (address comparison)" (line 262) | ✅ |
| `active_camera()` description | `std::optional<CameraComponent&>` or `std::nullopt` | "returns the stored `std::optional<CameraComponent&>` or `std::nullopt`" (line 263) | ✅ |
| `RenderSystem::render()` access pattern | `*cam_opt` (operator* yields `CameraComponent&`) | `auto& cam_comp = *cam_opt;` with comment "optional<CameraComponent&> — operator* yields CameraComponent&" (line 423) | ✅ |

### 3. Consistency with ADR-005 and ADR-010

| Item | ADR requirement | Spec compliance | Verdict |
|---|---|---|---|
| ADR-005: Use `std::optional<T&>` for optional references | `get_component<T>()` returns `std::optional<T&>` | `active_camera()` returns `std::optional<CameraComponent&>` — follows same pattern | ✅ |
| ADR-005: Compiler baseline GCC 16+, Clang 22+ | Assumption A-05 states C++26 baseline | ✅ (A-05: "The Buddd Engine uses the C++26 standard, supporting `std::optional<T&>`") | ✅ |
| ADR-010: No raw pointers in public API | `T*` → `std::optional<T&>` for nullable returns | `active_camera()` returns `std::optional<CameraComponent&>`, not `CameraComponent*` | ✅ |
| ADR-010: Non-null stored references | `std::reference_wrapper<T>` or `T&` | `active_camera_` is `std::optional<CameraComponent&>` (C++26, no `reference_wrapper` needed) | ✅ |
| ADR-010: Public API signatures | No raw pointer parameters | `register_camera(CameraComponent&)` and `unregister_camera(const CameraComponent&)` use references, not pointers | ✅ |

### 4. Cross-spec consistency

| Document | Check | Verdict |
|---|---|---|
| SPEC-008 (Scene Graph) | `Component` entity awareness additions are correctly described as superseding SPEC-008's deferral of lifecycle hooks | ✅ |
| SPEC-008 (Scene Graph) | `dynamic_cast` dispatch, `vector<unique_ptr<Component>>` storage, deferred destruction unchanged | ✅ |
| ADR-005 | `std::optional<T&>` pattern followed exactly | ✅ |
| ADR-006 | RTTI dispatch (`dynamic_cast`) consistent | ✅ |
| ADR-010 | Raw pointer rule followed: no raw pointers in public API | ✅ |

## Review history

| Date | Verdict | Summary |
|---|---|---|
| 2026-05-30 | `Accepted` | All changes verified: `reference_wrapper` fully removed, access patterns updated to `optional<T&>`, `<functional>` include removed, consistent with ADR-005 and ADR-010. No remaining issues. |

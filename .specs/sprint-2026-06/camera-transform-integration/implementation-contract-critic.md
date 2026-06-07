# Implementation Contract Review — Camera → Transform Integration

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **BLOCKING: Architecture boundary violation in `look_at(eye, center, up)` implementation** — RESOLVED. The contract now extracts GLM-dependent look-at math into `math::look_at_rotation(Vec3, Vec3, Vec3)` in `math/math.h` (`src/engine/math/`), which is permitted per ADR-002/ADR-019. Both `CameraComponent::look_at()` overloads delegate to this free function via `math::look_at_rotation(...)` with no GLM types in scene/ code.

- [x] **BLOCKING: Implementation order will produce broken build** — RESOLVED. Implementation order now has camera.h/camera.cpp deletion as step 8 (after all app and test file migrations in steps 6-7). A grep verification step is included to confirm no remaining includes before deletion.

All previous blocking issues have been resolved in the updated contract.

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## Warnings

Non-blocking concerns for awareness:

- **`CameraComponent::view_matrix()` member not specified in spec** — The contract adds a `view_matrix() -> Mat4` member to CameraComponent (section 2 header: `auto view_matrix() const -> math::Mat4;`). The spec's explicitly listed CameraComponent API in "User-visible behavior" item 2 does NOT include `view_matrix()` as a member; it only references the free function `math::view_matrix(position, orientation)`. While this is a natural convenience method used internally by `view_projection_matrix()`, it is technically scope creep (new public method not in the spec). The spec-author should decide whether to formally add it to the spec.

## Required changes

All previously requested changes have been implemented in the updated contract:

1. ~~Fix the `look_at(eye, center, up)` implementation to not use raw GLM types~~ — **DONE**: `math::look_at_rotation()` free function in `math/math.h`.
2. ~~Reorder the implementation sequence~~ — **DONE**: camera.h/camera.cpp deletion moved to step 8.
3. ~~Define the `look_at(Vec3 target)` implementation unambiguously~~ — **DONE**: delegates to `math::look_at_rotation(t.position, target, math::Vec3::unit_y())`.

## Suggested improvements

Optional ideas (not required):

- Consider explicitly listing `CameraComponent::view_matrix()` in the spec if it's going to be a public member. If it stays, update the spec's "User-visible behavior" to include it.
- The grep pattern `\.camera()` in done criteria could match string literals, comments, or code that happens to contain `.camera(` (like a method on a different type). A note to manually inspect grep results would prevent false positives.

## Re-review verdict (June 7, 2026)

Both previous blocking issues are **resolved**. The contract is now clean:

- **Architecture boundary**: GLM-dependent math extracted to `math::look_at_rotation()` in `math/math.h`. No raw GLM types in scene/ code.
- **Implementation order**: camera.h/camera.cpp deletion deferred to step 8, after all app/test migrations.
- **All previous warnings addressed**: `look_at(Vec3 target)` is now fully specified; `#include "math/camera.h"` removal tracking is explicit per file.
- **No new blocking issues found.**

Status: **All blocking issues cleared — contract ready for acceptance.**

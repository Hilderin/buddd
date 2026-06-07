# Governance Review — Camera → Transform Integration

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [ ] **ADR-024 missing from ADR Index** — `docs/adr/ADR-024-camera-transform-integration.md` exists on disk and is accepted, but `docs/wiki/decisions/adr-index.md` does not list it. The index jumps from ADR-023 to no ADR-024 entry. Every ADR must be listed in the index for traceability. **Fix**: Add an ADR-024 row to `docs/wiki/decisions/adr-index.md`.
- [x] Spec (SPEC-NNNN), implementation contract (IMPL-2026-06-001), and code are consistent — all 30 acceptance criteria (AC-001 through AC-030) are verified as satisfied in the code review.
- [x] Implementation order matches the contract: camera.h/camera.cpp deletion deferred to step 8, after all app/test migrations.
- [x] GLM architecture boundary respected: GLM types only inside `math/math.h` via `math::look_at_rotation()`, no GLM in scene/ code.
- [x] All 13 app files migrated, `cube_app` and `multi_material_app` use `Entity camera_entity_` instead of `math::Camera camera_`.
- [x] All 425 tests pass (100%).

When re-reviewing, mark resolved items with `[x]`. Add new items as `[ ]`.

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-024** (`docs/adr/ADR-024-camera-transform-integration.md`) — Created, accepted. Correctly documents: removal of `math::Camera`, CameraComponent projection-only redesign, free functions `view_matrix()` and `look_at_rotation()`, migration approach, and references to ADR-002/ADR-005/ADR-019.
- [x] **ADR-002** (GLM wrapper math) — Referenced correctly: `look_at_rotation()` uses GLM types in `math/math.h` only.
- [x] **ADR-019** (architecture boundaries) — Referenced correctly: free functions live in `math/math.h` (only file permitted to use GLM types directly).
- [x] **ADR-005** (optional ref component API) — Referenced correctly: `CameraComponent` accesses `entity().transform()` after `on_attach()`.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] `docs/wiki/domain/glossary.md` — Camera entry correctly notes "Removed in ADR-024". CameraComponent entry updated to projection-only description. `buddd_engine` entry no longer lists Camera in math module.
- [x] `docs/wiki/architecture/overview.md` — Math/ directory description correctly lists `view_matrix, look_at_rotation` instead of `Camera`. `camera.h`/`camera.cpp` entries removed from file tree. SPEC-004 reference notes `[removed in ADR-024]`.
- [x] `docs/wiki/domain/business-rules.md` — Light component accessor pattern correctly references CameraComponent pattern per ADR-024.
- [x] `docs/wiki/architecture/module-map.md` — `math.h` entry correctly lists `view_matrix(Vec3, Quat)` and `look_at_rotation(Vec3, Vec3, Vec3)`. CameraComponent entries updated to projection-only. `camera.h`/`camera.cpp` removed from math submodule table.
- [x] `docs/wiki/architecture/dependency-map.md` — No remaining references to camera.h, camera.cpp, or math::Camera wrapper type.

## Warnings

Non-blocking concerns for awareness:

- **`CameraComponent::view_matrix()` not in spec's explicit API list** — The spec's "User-visible behavior" (spec.md lines 47-55) lists `projection_matrix()` and `view_projection_matrix()` but not the `view_matrix()` member. The free function `math::view_matrix()` is the spec'd API; the member is an implementation convenience. Minor spec omission — does not block.
- **`tests/math_tests.cpp` modified outside contract's allowed file list** — Necessary change: old `math::Camera` tests lived in that file and were replaced with `view_matrix`/`look_at_rotation` tests. The contract's file list should have included it.
- **`docs/adr/ADR-024-camera-transform-integration.md` untracked** — File exists on disk but is not yet staged in git. Workflow housekeeping: should be `git add`'d.

## Required governance updates

Concrete changes to governance documents (ADRs, wiki):

- **Add ADR-024 to `docs/wiki/decisions/adr-index.md`** — Insert a row for ADR-024 between ADR-023 and the closing table boundary.

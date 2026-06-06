# ADR Index

| ADR | Title | Status |
|---|---|---|
| ADR-001 | `docs/adr/ADR-001-result-error-pattern.md` — `Result<T>` / `Error` as the project-wide error handling pattern | Accepted |
| ADR-002 | `docs/adr/ADR-002-glm-wrapper-math.md` — GLM wrapper pattern for math types (Vec2/3/4, Mat4, Quat) | Accepted |
| ADR-003 | `docs/adr/ADR-003-render-pipeline-architecture.md` — Render pipeline architecture (void draw, poll_events) | Accepted |
| ADR-004 | `docs/adr/ADR-004-demo-system-architecture.md` — Per-demo files and extensible dispatch | Accepted |
| ADR-005 | `docs/adr/ADR-005-optional-ref-component-api.md` — `std::optional<T&>` for component lookup API | Accepted |
| ADR-006 | `docs/adr/ADR-006-rtti-component-dispatch.md` — RTTI-based `dynamic_cast<T*>()` for component dispatch | Accepted |
| ADR-007 | `docs/adr/ADR-007-release-dependency-build.md` — Build fetched dependencies in Release mode | Accepted |
| ADR-008 | `docs/adr/ADR-008-ci-docker-infrastructure.md` — Docker-based CI infrastructure | Accepted |
| ADR-009 | `docs/adr/ADR-009-test-file-naming-convention.md` — Plural `_tests.cpp` suffix for test files | Accepted |
| ADR-010 | `docs/adr/ADR-010-no-raw-pointers-in-public-api.md` — Raw pointers prohibited in public API signatures; prefer `T&`, `std::optional<T&>`, `std::reference_wrapper<T>`, `std::span<T>` | Accepted |
| ADR-012 | `docs/adr/ADR-012-navigable-object-graph-engine-service.md` — Navigable object graph (RenderDevice→Window→Platform), EngineService lifecycle owner, virtual diagnostic accessors on RenderDevice, mouse capture on Window abstract interface | Accepted |
| ADR-014 | `docs/adr/ADR-014-cli-app-system.md` — CLI App System: centralised render loop with App lifecycle, unified `run` command, removal of `demo`/`capture` commands. Partially supersedes ADR-004. | Accepted |
| ADR-016 | `docs/adr/ADR-016-yaml-cpp-dependency.md` — yaml-cpp as FetchContent dependency for YAML asset metadata parsing, PRIVATE linkage, exception-safety wrappers. | Accepted |

The current project state was established via:

| Document | Type | Status |
|---|---|---|
| [SPEC-001](/.specs/sprint-2026-05/project-setup/spec.md) | Product specification | Accepted |
| [IMPL-001](/.specs/sprint-2026-05/project-setup/implementation-contract.md) | Implementation contract | Accepted |
| [SPEC-002](/.specs/sprint-2026-05/platform-abstraction/spec.md) | Product specification | Accepted |
| [IMPL-002](/.specs/sprint-2026-05/platform-abstraction/implementation-contract.md) | Implementation contract | Accepted |
| [SPEC-005](/.specs/sprint-2026-05/render-pipeline/spec.md) | Product specification | Accepted |
| [IMPL-005](/.specs/sprint-2026-05/render-pipeline/implementation-contract.md) | Implementation contract | Accepted |
| [SPEC-009](/.specs/sprint-2026-05/3d-cube-demo/spec.md) | Product specification | Accepted |
| [IMPL-009](/.specs/sprint-2026-05/3d-cube-demo/implementation-contract.md) | Implementation contract | Accepted |

The following design decisions established by SPEC-002 are recorded as accepted specifications (not ADRs — they remain within the spec/contract framework):

| Decision | Rationale |
|---|---|
| Runtime backend selection via `Backend` enum (`SDL3`/`Headless`) | Enables same binary for production and headless testing — no compile-time toggle needed |
| `Result<T>` (`std::expected<T, Error>`) as standard error pattern | C++26 `std::expected` provides value-or-error semantics; `Error` struct with `Category` + `code` + `message` covers all failure modes |
| Factory methods on abstract classes (`Platform::create()`, `RenderDevice::create()`) | Centralizes backend dispatch; hides concrete types behind pure virtual interfaces |
| `FetchContent` for SDL3 | Consistent with how Catch2 is already handled; no system package dependency required |
| Architecture boundary by convention (no automated guard yet) | Manual code review is sufficient at this stage; automated CI enforcement deferred to future work |

## Scene graph decisions

The following design decisions are recorded as accepted ADRs:

| ADR | Decision | Rationale |
|---|---|---|
| ADR-005 | `std::optional<T&>` for `get_component<T>()` return type | Provides type-safe optional reference semantics — no raw pointer or null check needed. C++26 feature (P2988R12). |
| ADR-006 | `dynamic_cast<T*>()` for component dispatch | Zero boilerplate in component types; no type registration; `Component` base class stays minimal (virtual destructor only). O(n) linear scan acceptable for v1 (< 10 components per entity). |

All accepted ADRs are listed at the top of this page with their status and a brief summary.

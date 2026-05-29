# ADR Index

- **ADR-001**: `docs/adr/001-result-error-pattern.md` — Establishes `Result<T>` / `Error` as the project-wide error handling pattern.

The current project state was established via:

| Document | Type | Status |
|---|---|---|
| [SPEC-001](/docs/specs/project-setup/spec.md) | Product specification | Accepted |
| [IMPL-001](/docs/specs/project-setup/implementation-contract.md) | Implementation contract | Accepted |
| [SPEC-002](/docs/specs/platform-abstraction/spec.md) | Product specification | Accepted |
| [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) | Implementation contract | Accepted |

The following design decisions established by SPEC-002 are recorded as accepted specifications (not ADRs — they remain within the spec/contract framework):

| Decision | Rationale |
|---|---|
| Runtime backend selection via `Backend` enum (`SDL3`/`Headless`) | Enables same binary for production and headless testing — no compile-time toggle needed |
| `Result<T>` (`std::expected<T, Error>`) as standard error pattern | C++26 `std::expected` provides value-or-error semantics; `Error` struct with `Category` + `code` + `message` covers all failure modes |
| Factory methods on abstract classes (`Platform::create()`, `RenderDevice::create()`) | Centralizes backend dispatch; hides concrete types behind pure virtual interfaces |
| `FetchContent` for SDL3 | Consistent with how Catch2 is already handled; no system package dependency required |
| Architecture boundary by convention (no automated guard yet) | Manual code review is sufficient at this stage; automated CI enforcement deferred to future work |

Future ADRs will be listed here with their status and a brief summary.

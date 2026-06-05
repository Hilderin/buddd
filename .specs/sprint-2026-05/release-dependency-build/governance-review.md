# Governance Review — Release Dependency Build (ADR-007)

## Status

`Accepted with warnings`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Cross-document coherence

Contradictions or gaps between spec, contract, code, and tests:

- [x] `src/engine/CMakeLists.txt` — `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release` is present on the SDL3 `FetchContent_Declare` block (line 7). Matches ADR-007's decision exactly.
- [x] `.vscode/launch.json` — Contains 2 GDB configurations (type `cppdbg`) and 2 LLDB configurations (type `lldb`). The LLDB configs are new; both debugger families are now represented. ADR-007's Consequences reference both debuggers benefiting. Consistent.
- [x] `CMakePresets.json` — `debug` preset sets `CMAKE_BUILD_TYPE=Debug` (line 10), `release` preset sets `CMAKE_BUILD_TYPE=Release` (line 21). ADR-007 references this correctly as the baseline that previously propagated to all fetched dependencies.
- [x] `docs/wiki/engineering/setup.md` — The Release build note (line 20) and the LLDB mention (line 82) are consistent with ADR-007 and `launch.json`.
- [x] `docs/wiki/architecture/dependency-map.md` — The Release build bullet (line 36) describing `CMAKE_BUILD_TYPE=Release` via `CMAKE_ARGS` is consistent with ADR-007.
- [x] ADR-007 itself — No contradictions between its Context, Decision, Consequences, and the actual state of the codebase.

All files are internally consistent with each other. No blocking contradictions found.

## Constitution violations

Checks against `docs/constitution/**`:

- [x] **CONST-001 (Architecture Boundaries)**: Not affected. The changes concern build configuration and debugger setup, not code architecture.
- [x] **CONST-002 (Testing Policy)**: Not affected. No changes to test code or testing requirements.
- [x] **CONST-003 (Documentation Policy)**: Not affected. This rule is still TODO and places no constraints.
- [x] **CONST-004 (Security Policy)**: Not affected. This rule is still TODO and places no constraints.
- [x] **Principles** (principles.md): The principle *"Governance documents must not contradict each other"* is satisfied — no contradictions between ADR-007 and any constitution document.
- [x] **Principles**: The principle *"Prefer existing conventions over new patterns"* — ADR-007 explicitly positions the change as a carve-out (not a replacement) of the existing FetchContent convention, with scope criteria for when it applies. This is within acceptable bounds.

**No constitution violations found.**

## ADR alignment

Required ADRs exist or are proposed:

- [x] **ADR-007 (Build Fetched Dependencies in Release Mode)** — Exists, is `Accepted`, and correctly documents the decision with context, alternatives, consequences, and references.

**Issues found:**

- [ ] **Incorrect cross-reference in ADR-007**: Line 104 of ADR-007 states *"per ADR-001 build conventions"*. ADR-001 is titled "Project-wide `Result<T>` / `Error` Pattern" and is entirely about error handling — it does not mention FetchContent, CMake, build presets, or any "build conventions". This cross-reference is inaccurate and should either cite a correct source (e.g., the project-setup spec or IMPL-001) or be rephrased to refer to the project's established FetchContent convention without citing ADR-001.

## Wiki alignment

Wiki reflects current state and does not become law:

- [x] `docs/wiki/engineering/setup.md` — Correctly references ADR-007 (line 20) and describes the Release build pattern accurately. The LLDB debugger mention (line 82) is consistent with the new launch.json configurations.
- [x] `docs/wiki/architecture/dependency-map.md` — Correctly references ADR-007 (line 36) and links to both the setup guide and the ADR. Consistent.

**Issues found:**

- [ ] **Small inaccuracy in wiki extension requirement**: `docs/wiki/engineering/setup.md` line 84 states that `.vscode/` files *"require the C/C++ extension (ms-vscode.cpptools)"*. This is still true overall (the GDB configurations and other VS Code files need it), but the new LLDB configurations additionally require the **CodeLLDB extension** (`vadimcn.vscode-lldb`) because they use `"type": "lldb"`. Consider updating the wiki to reflect that LLDB configurations require a separate extension.

## Warnings

Non-blocking concerns for awareness:

1. **ADR-007 line 104 cites "ADR-001 build conventions" but ADR-001 is about error handling, not build conventions.** The reference appears in Alternative 5 (Build dependencies as external projects): *"FetchContent is the project's established dependency management pattern (per ADR-001 build conventions)"*. This is factually incorrect — ADR-001 contains no information about FetchContent or build conventions. The reference should be corrected or removed. (Not a governance violation, but a documentation accuracy issue.)

2. **Wiki setup.md does not mention the CodeLLDB extension dependency for LLDB configurations.** Developers who only install the C/C++ extension will find GDB configs work but LLDB configs are unrecognized. The wiki should note this additional requirement.

## Required governance updates

Concrete changes to governance documents (constitution, ADRs, wiki):

1. **Fix ADR-007 line 104**: Either remove the parenthetical *"(per ADR-001 build conventions)"* or replace it with a correct reference (e.g., the project-setup spec/IMPL-001 that established FetchContent as the dependency management pattern).
2. **Update `docs/wiki/engineering/setup.md`**: Add a note that the LLDB debugger configurations require the CodeLLDB extension (`vadimcn.vscode-lldb`) in addition to the C/C++ extension.

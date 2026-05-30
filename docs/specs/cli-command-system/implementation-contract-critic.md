# Implementation Contract Review — CLI Command System (IMPL-006)

## Status

`Accepted`

Allowed values: `Accepted`, `Accepted with warnings`, `Rejected`

> The next workflow step MUST NOT proceed while the status is `Rejected` or any blocking issue remains unchecked.

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **CONST-002 violation — No unit tests for new/modified testable code**  
  **RESOLVED**: Unit tests added to Required tests section for `VersionCommand`, `HelpCommand`, unknown command handling, and no-args defaulting. These use process-level invocation of the `buddd` binary and are tagged `[cli]`. Human explicitly approved adding tests.

- [x] **Done criterion AC-015 describes a different CMake glob than the contract's specified content**  
  **RESOLVED**: Done criterion AC-015 updated to reference the same `${CMAKE_CURRENT_SOURCE_DIR}/*.cpp` form as the specified CMakeLists.txt content.

## Warnings

Non-blocking concerns for awareness:

- [x] **`help_command.h` unnecessarily includes `<cstdio>`**  
  **RESOLVED**: Removed `<cstdio>` from `help_command.h`. Include is only in `help_command.cpp`.

- [x] **`demo_helpers.cpp` include list includes `"render/primitive_topology.h"` unnecessarily**  
  **RESOLVED**: Removed `"render/primitive_topology.h"` from the include list.

- [x] **`std::string_view::data()` with `%s` printf format assumes null termination**  
  **RESOLVED**: Changed `version_command.cpp` to use `std::fwrite()` instead of `std::printf()` with `%s`.

- [x] **CONST-002 constitutional tension**  
  **RESOLVED**: Unit tests added for testable commands; human approval obtained.

- [x] **The `argc`/`argv` naming convention is inconsistent**  
  **RESOLVED**: Changed all parameter suppression from `/*unused*/` comment style to `[[maybe_unused]]` attribute, consistent with modern C++26 conventions.

## Required changes

Concrete, actionable changes requested:

1. [x] **Align Done criterion AC-015 with the specified CMakeLists.txt content**  
   Done criterion updated to reference `${CMAKE_CURRENT_SOURCE_DIR}/*.cpp` form.

2. [x] **Resolve the CONST-002 unit test gap**  
   Unit tests added for `VersionCommand`, `HelpCommand`, unknown command, and no-args default. Process-level integration tests via `buddd` binary invocation.

3. [x] **Remove `<cstdio>` from `help_command.h` includes**  
   Moved to `help_command.cpp`.

4. [x] **Remove `"render/primitive_topology.h"` from `demo_helpers.cpp` include list**  
   Removed.

## Suggested improvements

Optional ideas (not required):

- [x] **Add `[[nodiscard]]` to `run()` methods**  
   Applied to all four command classes.

- [ ] **Consider including `.h` files in the CMake glob**  
   Not applied — rationale documented in CMakeLists.txt section.

- [ ] **Extract `version_command.cpp` output format into a testable constant**  
   Not applied — process-level tests suffice per human approval.

- [x] **Consider `[[maybe_unused]]` instead of `/*param*/` comments**  
   Applied to all command `.cpp` implementations.

- [x] **Document the rationale for dropping `.h` from the CMake glob**  
   Added comment in CMakeLists.txt section.

## Review summary

| Category | Count |
|---|---|
| Blocking issues | 0 (2 resolved) |
| Warnings | 0 (5 resolved) |
| Required changes | 0 (4 resolved) |
| Suggested improvements | 2 pending (non-blocking) |
| Verdict | Accepted |

The contract is well-written, precise, and faithfully translates SPEC-006. All blocking issues and warnings have been resolved. The contract is ready for human validation.

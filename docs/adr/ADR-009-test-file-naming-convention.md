# ADR-009: Test File Naming Convention — Plural `_tests.cpp` Suffix

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

The project uses Catch2 for unit testing, with a `buddd_tests` executable built from multiple test files under `tests/`. As the test suite grew, two naming conventions emerged organically:

- **6 files** used the singular suffix `_test.cpp`: `version_test.cpp`, `math_test.cpp`, `platform_abstraction_test.cpp`, `sdl3_backend_test.cpp`, `cmd_test.cpp`, `demo_test.cpp`.
- **2 files** used the plural suffix `_tests.cpp`: `scene_graph_tests.cpp`, `model_tests.cpp`.

This inconsistency caused a build bug (see SPEC-009 governance) when `tests/CMakeLists.txt` switched from an explicit file listing to `file(GLOB_RECURSE)`. The glob pattern `*_test.cpp` silently excluded `*_tests.cpp` files, dropping 73 test cases from the build without any compiler warning or error.

The project already uses the convention that each test file contains *multiple* test cases — a single file like `math_test.cpp` contains 60+ individual `TEST_CASE` entries. The plural `_tests.cpp` suffix better reflects this reality and reduces ambiguity.

Standardising on a single, documented convention prevents future GLOB-related regressions and makes the build system's auto-discovery robust and predictable.

## Decision

All Catch2 test files under `tests/` MUST use the plural suffix `_tests.cpp`.

- `scene_graph_tests.cpp` ✅ — already conforms
- `model_tests.cpp` ✅ — already conforms
- `version_test.cpp` → `version_tests.cpp`
- `math_test.cpp` → `math_tests.cpp`
- `platform_abstraction_test.cpp` → `platform_abstraction_tests.cpp`
- `sdl3_backend_test.cpp` → `sdl3_backend_tests.cpp`
- `cmd_test.cpp` → `cmd_tests.cpp`
- `demo_test.cpp` → `demo_tests.cpp`

The `tests/CMakeLists.txt` GLOB pattern SHALL use `*_tests.cpp` to match all test files:

```cmake
file(GLOB_RECURSE BUDDD_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*_tests.cpp
)
```

Existing git history for renamed files is preserved via `git mv`.

### Enforcement

This convention is enforced by:
1. **Build system**: The `file(GLOB_RECURSE ... *_tests.cpp)` pattern only matches the plural suffix. Any new test file with a different suffix is silently excluded from the build — a clear signal that the naming convention was not followed.
2. **Code review**: Reviewers check that new test files use the `_tests.cpp` suffix.
3. **Governance**: This ADR serves as the authoritative reference for test file naming.

### Rationale

- **Plural reflects content**: Each file contains multiple `TEST_CASE` entries — the plural better describes what the file holds.
- **Consistency**: A single convention is easier to remember and enforce than two.
- **Glob safety**: With a single, unambiguous glob pattern, there is no risk of silently excluding valid test files.
- **Minimal churn**: Only 6 files need renaming. The rename is a one-time operation that establishes the convention going forward.

## Consequences

### Positive

- Consistent, discoverable naming convention for all test files.
- GLOB-based test discovery is robust — no risk of silent exclusion.
- New contributors can determine the naming convention by looking at any existing test file.
- The `tests/` directory listing is visually uniform.

### Negative

- 6 existing files must be renamed, which breaks `git blame` history for those files. This is mitigated by `git mv` (which Git tracks as a rename, preserving history across the rename boundary) and the relatively small number of affected files.
- Any open PRs that reference the old filenames will need rebasing.

### Risks

- Low. The rename is purely mechanical and does not change any test logic, assertions, or build artifacts. The `CHANGES` macro in Catch2 matches `TEST_CASE` names, not file names, so no test identifiers change.

## Compliance

- `tests/CMakeLists.txt` SHALL use the pattern `*_tests.cpp`.
- All new test files SHALL use the `_tests.cpp` suffix.
- Existing files SHALL be renamed via `git mv` to preserve history.
- Code review SHALL verify new test files follow the convention.

## Related documents

- `tests/CMakeLists.txt` — GLOB pattern updated to `*_tests.cpp`.
- `docs/wiki/engineering/testing.md` — Documents the naming convention.

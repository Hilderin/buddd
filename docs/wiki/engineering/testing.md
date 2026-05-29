# Testing

## Test framework

The project uses **Catch2 v3** (v3.7.0) as its unit test framework. Catch2 is downloaded automatically via CMake's `FetchContent` — no manual installation is required.

The test binary is named **`buddd_tests`** and links against `buddd_engine` and `Catch2::Catch2WithMain`.

## Running tests

```bash
# Via ctest (preset-based)
ctest --preset debug
ctest --preset release

# Direct invocation
./build/debug/tests/buddd_tests
```

Both Debug and Release presets include a `testPreset` that runs all registered tests.

## Current tests

At the bootstrap stage, there is a single sanity test:

| Test case | Tags | Source | Verification |
|---|---|---|---|
| `engine version is non-empty` | `[sanity]` | `tests/version_test.cpp` | `buddd::engine::version()` returns a non-empty `std::string_view` |

## Adding tests

1. Add a new `.cpp` file in `tests/`.
2. Include the appropriate Catch2 header (`<catch2/catch_test_macros.hpp>`).
3. Write `TEST_CASE` blocks with descriptive names and tags.
4. Register the new source file in `tests/CMakeLists.txt` (append to the `add_executable` call).
5. Tests are automatically discovered by `catch_discover_tests()`.

## Constitution reference

[CONST-002 (Testing Policy)](/docs/constitution/rules/CONST-002-testing-policy.md) requires that all testable code must have corresponding unit tests and those tests must pass.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — AC-007 (version sanity test), AC-008 (FetchContent), AC-010 (ctest passes)
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — sections 9 and 10 (test structure)

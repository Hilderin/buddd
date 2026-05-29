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

### Sanity test (bootstrap)

| Test case | Tags | Source | Verification |
|---|---|---|---|
| `engine version is non-empty` | `[sanity]` | `tests/version_test.cpp` | `buddd::engine::version()` returns a non-empty `std::string_view` |

### Headless platform abstraction tests

The platform abstraction layer introduces headless backend tests that run **without a display or GPU** — they are safe for CI:

| ID | Test case | Tags | Verification |
|---|---|---|---|
| T-01 | `Platform::create(Headless) succeeds` | `[headless]` `[platform]` | Returns valid `unique_ptr<Platform>` |
| T-02 | `Headless Platform creates Window with valid config` | `[headless]` `[window]` | `create_window()` returns valid window |
| T-03 | `Headless Window creates RenderDevice` | `[headless]` `[render]` | `RenderDevice::create()` returns valid device |
| T-04 | `Headless frame cycle completes` | `[headless]` `[render]` | `begin_frame()` / `end_frame()` sequence completes |
| T-05 | `Headless RenderDevice::size() returns correct dimensions` | `[headless]` `[render]` | `size()` matches window config |
| T-06 | `Headless Window::native_handle() returns nullptr` | `[headless]` `[window]` | `native_handle()` is `nullptr` |
| T-07 | `WindowConfig negative dimensions return error` | `[headless]` `[window]` | Returns `WindowCreationFailed` error |
| T-08 | `Error struct construction and to_string` | `[headless]` `[error]` | `to_string()` format is correct |
| T-09 | `make_error helper compiles and returns correct category` | `[headless]` `[error]` | Category and message are correct |
| T-10 | `make_error with explicit code` | `[headless]` `[error]` | `code` field is set correctly |
| T-11 | `Result<T> compiles with unique_ptr` | `[headless]` `[error]` | `Result<std::unique_ptr<int>>` compiles and works |

### SDL3/OpenGL tests (require display)

These tests require a display server and may be skipped or marked as `[!mayfail]` in CI environments without a display:

| ID | Test case | Tags | Verification |
|---|---|---|---|
| T-12 | `Backend enum values exist` | `[sdl3]` `[platform]` | `Backend::SDL3` and `Backend::Headless` are valid identifiers |
| T-13 | `Platform::create(SDL3) success` | `[sdl3]` `[platform]` | Returns valid Platform (requires display) |

SDL3-specific tests that require a display should be conditionally compiled (e.g., guarded by `#ifdef BUDDD_HAS_DISPLAY`) or tagged appropriately so CI does not fail.

## Test conventions

- All assertions use `REQUIRE`/`REQUIRE_FALSE` (not `CHECK`).
- Headless tests are tagged `[headless]` plus a subsystem tag (`[platform]`, `[window]`, `[render]`, `[error]`).
- Test files go in `tests/` and are registered in `tests/CMakeLists.txt`.

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
- Spec: [SPEC-002](/docs/specs/platform-abstraction/spec.md) — Acceptance criteria, User stories (headless testing)
- Implementation contract: [IMPL-002](/docs/specs/platform-abstraction/implementation-contract.md) — Required tests (T-01 through T-13)

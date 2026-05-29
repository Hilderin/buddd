# Business Rules

## CLI output behavior

| Input | Output | Exit code |
|---|---|---|
| `buddd` (no arguments) | `Buddd Engine v0.1.0` | 0 |
| `buddd --version` | `buddd 0.1.0` | 0 |
| `buddd --help` | `Buddd Engine v0.1.0` | 0 |
| `buddd <any other arguments>` | `Buddd Engine v0.1.0` | 0 |

- The only recognized flag is `--version` as the sole argument.
- All other argument combinations (including `--help`, multiple args, unknown flags) fall through to the greeting.
- There is no error output (stderr is empty) for any argument combination.
- The greeting message format is `Buddd Engine v<version>` with a trailing newline.
- The version output format is `buddd <version>` with a trailing newline.

## Version API contract

```cpp
namespace buddd::engine {
    auto version() -> std::string_view;
}
```

- The function returns a `std::string_view` pointing to a compile-time constant string.
- The return value is never empty (at minimum, it contains a valid version string).
- The initial return value is `"0.1.0"`.
- Changing the namespace, function name, return type, or semantic meaning of the returned string constitutes a breaking change.
- The `version.cpp` string and the `project()` VERSION in `CMakeLists.txt` must be kept in sync manually.

## Project conventions

- Source files use `snake_case` naming.
- Directory names use `snake_case`.
- Code formatting is enforced via `.clang-format` (LLVM style with 4-space indent, 100-column limit, `c++26` standard).
- CMake targets use `snake_case` naming.
- Formatting is applied by running `cmake --build --preset debug --target format`.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — User-visible behavior, User stories 1-3, Conventions
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — sections 5, 7 (version and CLI behavior)

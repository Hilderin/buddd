# Dependency Map

## Target dependencies

```
buddd ──PRIVATE──► buddd_engine
                      │
buddd_tests ──PRIVATE─┤
             ──PRIVATE──► Catch2::Catch2WithMain (external)
                         │
buddd_editor            (standalone, no dependencies)
```

| Source target | Dependency | Link type | Notes |
|---|---|---|---|
| `buddd` | `buddd_engine` | PRIVATE | CLI needs the engine's version API |
| `buddd_editor` | *(none)* | — | INTERFACE placeholder, no compiled code |
| `buddd_tests` | `buddd_engine` | PRIVATE | Tests exercise engine library code |
| `buddd_tests` | `Catch2::Catch2WithMain` | PRIVATE | Catch2 provides `main()` and test runner |

## External dependencies

| Dependency | Version | Source | Fetch method |
|---|---|---|---|
| **Catch2** | v3.7.0 | `https://github.com/catchorg/Catch2.git` | CMake `FetchContent` (automatic download at configure time) |

- Catch2 is fetched once and cached in the build directory; subsequent configures use the cached copy.
- No network access is required after the initial fetch.
- No system-installed Catch2 is needed.

## Build toolchain dependencies

| Tool | Minimum version | Required for |
|---|---|---|
| CMake | 3.28 | Build configuration, presets, FetchContent |
| Ninja | 1.11 | Build execution |
| C++ compiler (GCC 14+ / Clang 19+) | C++26 support | Compilation |
| clang-format | 18 (optional) | `cmake --build ... --target format` |

## Key constraints

- `buddd_engine` has **zero** external dependencies at bootstrap — no rendering, physics, audio, or third-party libraries.
- The engine is a **static library** (`STATIC`), not header-only. This may change in the future.
- The editor target produces **no binary** and links **nothing** — it is a structural placeholder.
- Catch2 is **not** a dependency of the engine or the CLI — only of the test binary.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — Assumptions A-05 through A-10
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — sections 3-10 (target definitions)

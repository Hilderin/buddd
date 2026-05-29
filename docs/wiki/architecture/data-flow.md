# Data Flow

## CLI data flow

At the bootstrap stage, the data flow through the CLI binary is minimal:

```
User invocation
      │
      ▼
main(int argc, char* argv[])
      │
      ├── argc == 2 && argv[1] == "--version" ?
      │       ├── YES ──► printf("buddd %s\n", buddd::engine::version().data())
      │       │               │
      │       │               └──► stdout: "buddd 0.1.0\n"
      │       │
      │       └── NO  ──► printf("Buddd Engine v%s\n", buddd::engine::version().data())
      │                       │
      │                       └──► stdout: "Buddd Engine v0.1.0\n"
      │
      └── return 0
```

There are two distinct output formats:
- **No arguments / unknown arguments** → `Buddd Engine v0.1.0` (human-readable greeting)
- **`--version` (sole argument)** → `buddd 0.1.0` (machine-parseable version string)

## Test data flow

```
Catch2 test runner
      │
      ▼
TEST_CASE("engine version is non-empty", "[sanity]")
      │
      └── REQUIRE_FALSE(buddd::engine::version().empty())
              │
              └──► Calls version() → returns "0.1.0" → .empty() is false → test passes
```

## Version string source

The version string `"0.1.0"` is defined in a single location:

```
src/engine/version.cpp  ──►  return "0.1.0";
```

It is consumed by:
- `src/cmd/main.cpp` (via `buddd::engine::version()`)
- `tests/version_test.cpp` (via `buddd::engine::version()`)

The version in `CMakeLists.txt` (`project(buddd VERSION 0.1.0 ...)`) must be kept in sync with `version.cpp` manually — no automation is introduced at bootstrap.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — User-visible behavior, User stories 1-3
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — section 7 (`main.cpp` behavior)

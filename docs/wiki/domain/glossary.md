# Domain Glossary

| Term | Definition |
|---|---|
| **Buddd Engine** | The C++26 game engine project. At bootstrap, it consists of a static library with a version API, a CLI binary, an editor placeholder, and a test suite. |
| **buddd** | The CLI binary produced from `src/cmd/`. It links the engine library and prints version information. |
| **buddd_engine** | The static library target produced from `src/engine/`. The core engine library — currently exposes only `buddd::engine::version()`. |
| **buddd_editor** | The INTERFACE library target produced from `src/editor/`. A structural placeholder for the future editor; no code is compiled. |
| **buddd_tests** | The test executable produced from `tests/`. Links `buddd_engine` and Catch2. |
| **version API** | The function `buddd::engine::version() -> std::string_view` that returns the current engine version string `"0.1.0"`. |
| **CMake preset** | A named build configuration defined in `CMakePresets.json`. The project has `debug` and `release` presets. |
| **FetchContent** | CMake module used to automatically download Catch2 v3.7.0 at configure time. No manual installation required. |
| **Catch2 v3** | The C++ unit testing framework used by the project. Fetched via `FetchContent` at version v3.7.0. |

## Version scheme

The project uses [Semantic Versioning](https://semver.org/) (major.minor.patch). The initial version is `0.1.0`.

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — Assumptions A-05 through A-09
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — API compatibility impact section

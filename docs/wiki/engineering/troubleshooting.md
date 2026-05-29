# Troubleshooting

## CMake configuration failures

| Symptom | Likely cause | Solution |
|---|---|---|
| `CMake Error: Could not find CMAKE_ROOT` | CMake not installed | Install CMake >= 3.28 |
| `CMake Error: Could not create named generator Ninja` | Ninja not installed or not on PATH | Install Ninja >= 1.11 |
| `The compiler does not support C++26` | Compiler too old | Use GCC 14+ or Clang 19+ |
| `Failed to download Catch2` | Network unavailable during first configure | Ensure network access; after first successful configure, FetchContent is cached |
| `Unknown argument --preset invalid` | Typo in preset name | Run `cmake --list-presets` to see available presets |
| `Catch2 repository or tag not found` | Git tag `v3.7.0` was deleted or moved | Check the tag in `CMakeLists.txt` and update if needed |

## Build failures

| Symptom | Likely cause | Solution |
|---|---|---|
| `ninja: error: ...` with compiler diagnostics | Compilation error in source code | Fix the error and rebuild |
| `clang-format: command not found` | `clang-format` not installed | Install clang-format >= 18, or skip the `format` target |
| Build fails after FetchContent error | First configure failed mid-way | Clean the build dir (`rm -rf build/debug`) and reconfigure |

## Test failures

| Symptom | Likely cause | Solution |
|---|---|---|
| `ctest` reports 0 tests | Tests not built, or `catch_discover_tests` not run | Run `cmake --build --preset debug` first |
| Test binary crashes | Linking issue or missing symbol | Verify `buddd_tests` links `buddd_engine` |

## Binary behavior

| Observation | Explanation |
|---|---|
| `buddd --help` prints the greeting | `--help` is not handled at bootstrap — any unrecognized argument falls through to the greeting branch |
| `buddd arg1 arg2` prints the greeting | Only `--version` as the sole argument is special-cased; everything else prints the greeting |
| Incremental build says "no work to do" | No source files changed — this is correct behavior |

## Reference

- Spec: [SPEC-001](/docs/specs/project-setup/spec.md) — Edge cases and Error cases sections
- Implementation contract: [IMPL-001](/docs/specs/project-setup/implementation-contract.md) — Edge cases section

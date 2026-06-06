# ADR-007: Build Fetched Dependencies in Release Mode

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

The project uses CMake FetchContent to manage third-party dependencies:

- **SDL3** — a compiled C library (~300k+ lines) for windowing, input, and graphics context creation.
- **Catch2** — a C++ testing framework (used for unit tests).
- **GLM** — a header-only C++ math library (not affected by build type).

When the `debug` CMake preset is used, `CMAKE_BUILD_TYPE=Debug` propagates to all fetched dependencies via FetchContent. For SDL3 specifically, this produces several issues:

1. **Large static library**: The SDL3 static library compiled in Debug mode includes full DWARF debug symbols and is typically 100–300+ MB on disk.

2. **Slow debugger startup (~30 seconds)**: Both GDB and LLDB must parse all DWARF debug symbols from SDL3 before the user can interact. With the project's own code being a small fraction of total binary size, the vast majority of debugger startup time is spent parsing SDL3's debug information.

3. **Longer build times**: Debug mode compiles with `-O0` and includes debug info, increasing both compilation time and link time for every first build or whenever the dependency cache is cleared.

The project's own code rarely needs to step into SDL3 internals during debugging. SDL3 is a well-tested, stable library; when bugs occur, they are almost always in the project's usage of SDL3 (wrong parameter order, missing initialisation, incorrect event handling) rather than in SDL3 itself.

## Decision

**Use `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release` in `FetchContent_Declare` for compiled dependencies where stepping into the dependency during debugging is not expected.**

This was applied to SDL3 in `src/engine/CMakeLists.txt`:

```cmake
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.30
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
)
```

The `CMAKE_ARGS` argument overrides the build type for that specific dependency only, regardless of the CMake preset used for the main project. The main project continues to build in the preset's configured mode (Debug, Release, etc.).

Header-only dependencies (GLM) are unaffected — they produce no compiled output, so build type does not apply.

### Scope

This pattern applies to any compiled FetchContent dependency where:

- The dependency is large enough to noticeably affect build times or debugger startup.
- Stepping into the dependency's internals during normal debugging is rare or never needed.
- The dependency's API is used through stable, well-documented interfaces.

Future compiled dependencies should be evaluated against these criteria. Catch2, for example, is small and occasionally useful to step into during test debugging, so it may remain in Debug mode — this decision does not mandate Release for all dependencies.

## Alternatives considered

### 1. Keep dependencies in Debug (status quo ante)

Leave SDL3 building in Debug mode along with the main project.

- **Pros**: Simple, no build system changes needed. Full debug info for all code.
- **Cons**: ~30-second debugger startup on every launch. 100–300+ MB static library. Longer first builds.
- **Verdict**: Rejected — the debugger startup cost alone makes this unacceptable for the daily development workflow.

### 2. Use GDB index (`-Wl,--gdb-index`)

Add linker flags to generate a GDB index, which speeds up symbol loading in GDB.

```cmake
target_link_options(SDL3::SDL3 INTERFACE -Wl,--gdb-index)
```

- **Pros**: Helps GDB load symbols faster without changing the build type.
- **Cons**: Only works with the gold or lld linker (not the default GNU ld on many systems). Does not reduce build times or library size. Does not help LLDB users. Adds complexity to linker flags.
- **Verdict**: Rejected — a partial solution that does not address the root cause.

### 3. Use LLDB instead of GDB

LLDB is faster at loading DWARF debug info than GDB, so switching debuggers mitigates the startup time problem.

- **Pros**: No build system changes needed. LLDB startup is faster than GDB even with debug deps.
- **Cons**: Does not eliminate the root cause (massive debug symbols from dependencies). Team members may prefer GDB or use tools (VS Code, CLion) that default to GDB on Linux. Does not reduce build times or library size.
- **Verdict**: Rejected — relies on developer workflow changes rather than fixing the underlying issue. LLDB is still available as an alternative debugger, but the build change benefits all debuggers.

### 4. Strip debug symbols post-build

Build SDL3 in Debug mode, then run a post-build step to strip debug symbols from the library or the final executable.

```cmake
add_custom_command(TARGET SDL3 POST_BUILD COMMAND ${CMAKE_STRIP} ...)
```

- **Pros**: Debug mode compilation can still be used if deep debugging is needed.
- **Cons**: More complex build process. Still pays the build time overhead (Debug mode is slower to compile). If the strip happens after linking the final binary, the linker still processed oversized object files. Makes it harder to optionally re-enable debug symbols for deps when needed.
- **Verdict**: Rejected — more complexity than the chosen approach with no benefit for build times.

### 5. Build dependencies as external projects (no FetchContent)

Use `ExternalProject_Add` or system-installed prebuilt binaries for dependencies, keeping FetchContent only for the project's own code.

- **Pros**: Complete control over dependency build configuration. Can install Release-built SDL3 system-wide.
- **Cons**: Loses FetchContent's convenience (automatic download, no system install required, reproducible builds from git tags). Adds setup friction for new developers. Harder to manage dependency versions across machines.
- **Verdict**: Rejected — FetchContent is the project's established dependency management pattern (per SPEC-001 / IMPL-001 build conventions), and the `CMAKE_ARGS` approach keeps that workflow intact.

## Consequences

### Positive

- **Fast debugger startup**: GDB and LLDB start in ~1–3 seconds instead of ~30 seconds, because SDL3's debug symbols are absent from the linked binary. The debugger only parses symbols for the project's own code.
- **Faster build times**: SDL3 compiles with `-O2` (default for Release) instead of `-O0`, and no debug info is generated for SDL3 objects. First builds and cache-cleared rebuilds are noticeably faster.
- **Smaller build artifacts**: The SDL3 static library is a fraction of its Debug-mode size (~10–30 MB instead of 100–300+ MB), reducing disk usage and link time.
- **Transparent to developers**: The change is in a single line of CMake. Developers build and debug as before — no workflow changes are needed.
- **No impact on header-only deps**: GLM is unaffected because it produces no compiled output.

### Negative

- **Cannot step into SDL3 internals by default**: If a developer encounters a suspected SDL3 bug and needs to trace through its source, the Release build means optimised code with no debug symbols. This is a rare scenario (SDL3 is a mature, well-tested library). The workaround is to temporarily remove the `CMAKE_ARGS` line and rebuild.
- **Inconsistent build types across the project**: The main project builds in Debug while SDL3 builds in Release. This is unusual but fully supported by CMake and causes no ABI issues because SDL3 is a C library with a stable ABI, linked as a static library.
- **Pattern requires manual enforcement**: Each compiled FetchContent dependency must be individually evaluated and annotated with `CMAKE_ARGS`. New dependencies added without this pattern will default to the main project's build type (Debug), potentially reintroducing the problem. Code review should catch this.

### Precedent

This decision applies to SDL3 specifically and sets a precedent for future compiled FetchContent dependencies. It does **not** change the project's overall build system approach (FetchContent + CMake presets remain the standard). It introduces a carve-out pattern: `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release` for dependencies where debugger access to internals is not expected.

This pattern does **not** apply to the project's own libraries or executables — those should always use the build type from the CMake preset. It also applies to Catch2 (since ADR-008 / Docker CI), where building in Release mode reduces CI build time. Developers can temporarily remove the `CMAKE_ARGS` line for Catch2 if they need to step into test framework internals during debugging.

## References

- `src/engine/CMakeLists.txt`: The SDL3 `FetchContent_Declare` call with `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release` (lines 3–8).
- `CMakeLists.txt` (root): The Catch2 `FetchContent_Declare` call with `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release`.
- `CMakePresets.json`: The `debug` configure preset sets `CMAKE_BUILD_TYPE=Debug` (line 10), which previously propagated to all fetched dependencies.
- `.vscode/launch.json`: Debugger configurations for GDB and LLDB (both debuggers benefit from the faster startup this ADR enables).
- ADR-008: Docker-based CI infrastructure — motivated the Catch2 Release build to reduce CI times.

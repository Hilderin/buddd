# ADR-016: Add yaml-cpp as a Dependency for YAML Asset Metadata Parsing

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

The Asset Manager feature (SPEC-018) introduces a system where game assets (textures, materials, and future types) are defined by YAML metadata files. Each `.yaml` file in the `assets/` directory tree specifies the asset's `type`, `version`, and type-specific fields (e.g., `source` for textures, `shaders`/`textures`/`constants` for materials). The engine needs a YAML parsing library to read these files at runtime.

### Requirements

1. **Parse structured YAML documents** — Scalar fields (`type: Texture`), maps (`shaders: { vertex: ... }`), and sequences (future use for animation frames or multi-texture layers).
2. **No network or filesystem access beyond local file I/O** — Parsing is offline; all `.yaml` files are on local disk.
3. **No custom YAML tag evaluation** — The parser is used in default mode (scalar/map/sequence only). No YAML tags, anchors, aliases, or custom type resolution is needed.
4. **Exception safety** — The project uses `Result<T>` for error propagation (ADR-001). Any YAML parsing library that throws exceptions must have those exceptions caught and converted to `Result` errors at the engine boundary.
5. **CMake FetchContent integration** — The project manages all third-party dependencies via `FetchContent` (SDL3, GLM, stb, Catch2). Any new dependency must follow this pattern.
6. **PRIVATE linkage** — YAML types must not appear in public engine headers (CONST-001). The dependency is an implementation detail of the Asset Manager.

### Why YAML over other formats

Three structured text formats were evaluated for asset metadata: **YAML**, **JSON**, and **TOML**.

| Criterion | YAML | JSON | TOML |
|---|---|---|---|
| **Human readability** | Excellent — comments, no quotes on keys | Good — no comments, keys must be quoted | Good — comments, INI-like |
| **Multi-line strings** | Native (`\|` block scalar) | Requires `\n` escapes | Literal strings with `'''` |
| **Type inference** | Auto — `version: 1` is int, `roughness: 0.5` is float | Auto — same behaviour | Auto — same behaviour |
| **Reference/cross-file linking** | Natural with anchors/aliases (future use) | Not supported natively | Not supported natively |
| **C++ library maturity** | yaml-cpp (mature, widely used) | Many options (nlohmann, rapidjson, etc.) | toml11, toml++ (less established) |
| **Game industry convention** | Widely used in game engines (Unity YAML, Godot .tscn, Unreal .uasset metadata) | Common for web APIs, less common for game asset metadata | Rare in game engines |
| **Spec complexity** | Large spec (many features we won't use) | Minimal spec | Minimal spec |

**Decision**: YAML. The primary driver is human readability for content creators working with asset files. Comments, unquoted keys, and block scalars make YAML the most ergonomic format for artists and designers editing asset metadata by hand. The game industry precedent (Unity, Godot, Unreal metadata) confirms YAML is a natural fit. The risk of YAML spec complexity is mitigated by using the parser in default mode with no custom tag evaluation.

### Why yaml-cpp over other C++ YAML parsers

| Parser | License | Active | FetchContent | Exception safety | Quality |
|---|---|---|---|---|---|
| **yaml-cpp** (jbeder) | MIT | Yes (v0.8.0, 2024) | Native CMake support | Throws `YAML::Exception` | Battle-tested, widely used |
| **libyaml** (C library) | MIT | Maintenance | No CMake (C library) | Error codes (no exceptions) | Low-level C API, more boilerplate |
| **rapidyaml** (biojppm) | MIT | Yes | Native CMake support | Safer (no exceptions by default) | Newer, smaller ecosystem |
| **yyjson** (ibireme) | MIT | Yes | CMake (JSON not YAML) | N/A (JSON only) | Not a YAML parser |

**Decision**: yaml-cpp. It is the most established C++ YAML parser, with native CMake support (trivial `FetchContent` integration), MIT license, and an API well-suited to the project's use case (`YAML::LoadFile`, `node["key"].as<T>()`, `node["key"].IsDefined()`). The exception-safety concern (yaml-cpp throws `YAML::Exception` on parse errors) is addressed at the call site — all `YAML::LoadFile()` calls are wrapped in `try-catch` blocks that convert exceptions to `Result<T>` errors (see Consequences below).

rapidyaml was the strongest alternative. It avoids exceptions entirely, which aligns with ADR-001's preference for error-code-based error propagation. However, yaml-cpp's wider ecosystem, longer track record, and simpler API for the project's straightforward parsing needs outweigh the minor exception-safety advantage of rapidyaml. The exception-handling wrapper is a bounded, auditably small pattern (applied to exactly two call sites in V1).

### Why FetchContent over a system package manager

| Approach | Pros | Cons |
|---|---|---|
| **FetchContent** (selected) | Reproducible builds (git tag pinning); no system install required; automatic download on first configure; matches existing project pattern (SDL3, GLM, stb, Catch2) | Download on first build (one-time cost); no system-wide caching |
| **apt package** (`libyaml-cpp-dev`) | Pre-built binary, no build time for yaml-cpp | Version varies across distros (Ubuntu 22.04 has 0.6.3, Ubuntu 24.04 has 0.8.0); non-reproducible across CI/developer machines; adds system dependency documentation overhead |
| **Git submodule** | Pin to exact commit; no CMake-level abstraction | Manual update step; submodule management overhead; requires `add_subdirectory` or `find_package` after init |
| **Conan / vcpkg** | Package manager with version resolution | Adds a new package manager to the project; team must learn and maintain it; overkill for two-dozen dependencies |

**Decision**: FetchContent. It is the project's established dependency management pattern. All existing third-party dependencies (SDL3, GLM, stb, Catch2) use FetchContent. Adding yaml-cpp via FetchContent is consistent, requires no new infrastructure, and the `CMAKE_ARGS` flag allows building the dependency in Release mode (per ADR-007 precedent).

### CONST-001 implications

CONST-001 (Architecture Boundaries) prohibits platform, graphics, or windowing library headers outside `src/engine/`. yaml-cpp is neither a platform, graphics, nor windowing library — it is a general-purpose data format parser — so CONST-001 does not directly restrict it. However, the principle behind CONST-001 — that consumers of the engine library should not be coupled to internal implementation details — applies by analogy.

yaml-cpp is linked as a **PRIVATE** dependency:

```cmake
target_include_directories(buddd_engine PRIVATE
    ${yaml-cpp_SOURCE_DIR}/include
)
target_link_libraries(buddd_engine PRIVATE
    yaml-cpp
)
```

- No public header in `src/engine/` includes `<yaml-cpp/yaml.h>`.
- yaml-cpp types (`YAML::Node`, `YAML::Exception`, etc.) appear only in `.cpp` files (`asset_manager.cpp`).
- Consumers of `buddd_engine` (e.g., `src/cmd/`, tests) link against `buddd_engine` PUBLIC dependencies (SDL3, GLM, OpenGL) but NOT yaml-cpp.
- This means the engine's public API surface is free of YAML types, preserving the abstraction boundary.

**Relation to ADR-007**: yaml-cpp is built in Release mode via `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release`, following the precedent set by ADR-007 for compiled dependencies where stepping into the dependency during debugging is not expected.

### Exception safety and ADR-001

ADR-001 establishes `Result<T>` (via `std::expected`) as the project-wide error propagation mechanism. yaml-cpp throws `YAML::Exception` (a subclass of `std::runtime_error`) on parse errors, malformed files, and I/O failures. This is inconsistent with the project's no-exception error propagation pattern.

The inconsistency is resolved at every yaml-cpp call site with a `try-catch` wrapper:

```cpp
YAML::Node yaml;
try {
    yaml = YAML::LoadFile(yaml_path);
} catch (const YAML::Exception& e) {
    return Err(Error{
        .type = ErrorType::IoFailed,
        .message = "YAML parse error in " + yaml_path + ": " + e.what()
    });
} catch (const std::exception& e) {
    return Err(Error{
        .type = ErrorType::InternalError,
        .message = "Unexpected error parsing " + yaml_path + ": " + e.what()
    });
}
```

This pattern:
- Prevents yaml-cpp exceptions from escaping the engine boundary.
- Converts all yaml-cpp errors into project-standard `Result<T>` errors.
- Is applied to every `YAML::LoadFile()` call (two call sites in V1: `create<TextureAsset>` and `create<MaterialAsset>`).
- Is documented as a mandatory pattern for any future yaml-cpp usage in the codebase.

The exception-handling wrapper is acceptable because:
- The number of yaml-cpp call sites is small (bounded, auditable).
- The wrapper is localised to a single file (`asset_manager.cpp`).
- The pattern is enforced by code review (any new yaml-cpp usage without a try-catch must be rejected).

## Decision

We add **yaml-cpp v0.8.0** as a FetchContent dependency to `src/engine/CMakeLists.txt`, linked as a PRIVATE dependency of `buddd_engine`, with all `YAML::LoadFile()` calls wrapped in exception-safe try-catch blocks.

### CMake integration

The dependency is declared after the stb block and before `find_package(OpenGL)`:

```cmake
# ----- yaml-cpp (YAML parsing for asset metadata) -----
set(CMAKE_POLICY_VERSION_MINIMUM 3.5) # yaml-cpp 0.8.0 requires CMake >= 3.5
FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
              -DCMAKE_POLICY_VERSION_MINIMUM=3.5
              -DYAML_CPP_BUILD_TESTS=OFF
              -DYAML_CPP_BUILD_TOOLS=OFF
              -DYAML_CPP_INSTALL=OFF
)
FetchContent_MakeAvailable(yaml-cpp)
```

Key details:
- **Release build**: per ADR-007 (no need to step into yaml-cpp internals during debugging).
- **Tests/tools/install disabled**: avoids unnecessary build work (saves ~5-10s on first configure).
- **`CMAKE_POLICY_VERSION_MINIMUM=3.5`**: yaml-cpp 0.8.0's CMakeLists.txt targets CMake 3.5 minimum; setting this avoids a CMake policy warning.
- **PRIVATE include and link**: yaml-cpp headers and library are visible only inside `src/engine/`.

### Scope

| Aspect | Decision |
|---|---|
| **Format** | YAML (not JSON, TOML, or custom formats) |
| **Parser** | yaml-cpp 0.8.0 (MIT license) |
| **Integration** | FetchContent (not apt, submodule, or package manager) |
| **Linkage** | PRIVATE (not exposed in public headers) |
| **Build type** | Release (not Debug — matches ADR-007) |
| **Exception safety** | `try-catch` at every `YAML::LoadFile()` call site |
| **Custom tags** | None — default scalar/map/sequence mode only |
| **Git tag** | Pinned to exact release (`0.8.0`), not a branch |

### Non-decisions (explicitly out of scope)

- JSON or TOML support as an alternative format in the same system.
- Custom YAML tag registration or advanced yaml-cpp features (anchors, aliases, custom `convert<T>` specialisations).
- System-wide installation of yaml-cpp (the library is fetched at configure time only).
- Writing YAML files from the engine (the engine is read-only for YAML).
- yaml-cpp version auto-update policy.

## Alternatives considered

### 1. JSON with nlohmann/json

**Pros**: Familiar format, excellent C++ library (nlohmann/json), no exceptions if using the `-fno-exceptions` configuration, widely used in game tooling.

**Cons**: JSON has no comment syntax (blocks inline documentation of asset fields), keys must be quoted (adds visual noise), no native multi-line string support. The trade-off is ergonomic: JSON is better for machine-to-machine communication, worse for human-authored asset definitions.

**Verdict**: Rejected. Asset files are primarily authored by humans (artists, designers). YAML's readability advantage outweighs JSON's tooling maturity.

### 2. TOML with toml++

**Pros**: INI-like syntax familiar from CMake and Python `pyproject.toml`, comments supported, unquoted keys, simpler spec than YAML.

**Cons**: Less established in game engine contexts, fewer examples and community knowledge. The `toml++` library is newer than yaml-cpp with a smaller user base. TOML's table syntax (`[section]`) is less natural than YAML's nested maps for multi-layer structures like `shaders: { vertex: ..., fragment: ... }`.

**Verdict**: Rejected. YAML's tree structure maps more naturally to the nested metadata format (type → version → type-specific fields) than TOML's table-based sections.

### 3. rapidyaml

**Pros**: Exception-free by default (aligns with ADR-001), MIT license, CMake support, faster than yaml-cpp in benchmarks.

**Cons**: Smaller ecosystem, fewer examples, less battle-tested in production. API is more verbose for the simple read-only use case (requires explicit node-by-node traversal rather than `node["key"].as<T>()` in yaml-cpp).

**Verdict**: Rejected. The exception-safety advantage is real but mitigated by the try-catch wrapper pattern. yaml-cpp's simpler API and wider adoption make it the lower-risk choice for the project's straightforward parsing needs.

### 4. Custom hand-written parser

**Pros**: Zero external dependencies, complete control, no exception-safety concerns, no licensing considerations.

**Cons**: Significant implementation effort for YAML parsing (the YAML spec is ~80 pages). High risk of edge-case bugs (escaping, Unicode, type inference). Ongoing maintenance cost. Not justified for a single feature that parses at most a few dozen lines per asset file.

**Verdict**: Rejected. A custom parser would require an order of magnitude more effort than integrating yaml-cpp, with no benefit commensurate with the cost.

### 5. System package (`apt install libyaml-cpp-dev`)

**Pros**: Pre-built binary, no build time, system-wide caching.

**Cons**: Version varies across OS releases; adding a system dependency increases setup friction for new developers; CI Docker image must be rebuilt on yaml-cpp version changes (or the image must always include it).

**Verdict**: Rejected. FetchContent is the project's established pattern and avoids coupling the build environment to a specific OS package version.

## Consequences

### Positive

- **Consistent dependency management**: yaml-cpp follows the same FetchContent pattern as SDL3, GLM, stb, and Catch2. No new build infrastructure is needed.
- **PRIVATE linkage preserves abstraction**: Consumers of `buddd_engine` are not coupled to yaml-cpp. No YAML types leak into public headers.
- **Exception safety**: All yaml-cpp exceptions are caught at the call site and converted to `Result<T>` errors. No yaml-cpp exception escapes the engine boundary.
- **Release build per ADR-007**: yaml-cpp compiles in Release mode, avoiding debug symbol bloat and debugger startup cost for an implementation-detail dependency.
- **Human-readable asset files**: Content creators can write YAML with comments, unquoted keys, and block scalars — the most ergonomic structured format for hand-authored metadata.
- **Battle-tested library**: yaml-cpp is used in hundreds of C++ projects; known edge cases (escaping, Unicode, type inference) are already handled.

### Negative

- **Exception-safety wrapper required**: Every yaml-cpp call site must be wrapped in `try-catch`. This is a manual enforcement point — code review must catch any new call site without exception handling. The risk is mitigated by the small number of call sites (two in V1) and their concentration in a single file.
- **Increased first-configure time**: FetchContent downloads and builds yaml-cpp on the first CMake configure (~10-15s with the Release preset and disabled tests/tools). This is a one-time cost; subsequent configures use the cached build.
- **yaml-cpp is C++ with exceptions**: While the project may target `-fno-exceptions` environments in the future, yaml-cpp requires C++ exceptions enabled (it uses exceptions for error reporting). This does not block the project's current targets (GCC 16+, Clang 22+ all have exceptions enabled), but it is a constraint to be aware of if a no-exceptions configuration is adopted later. The exception wrapper pattern is a first step toward isolating exception usage, but yaml-cpp itself cannot be used without exception support.
- **YAML spec complexity**: YAML has a large specification with rarely-used features (anchors, aliases, tags, multi-document streams). yaml-cpp may interpret edge cases differently than expected. Mitigation: the parser is used in default mode with no custom tag evaluation, minimising surface area.
- **Duplicate build in CI**: If the CI Docker image is rebuilt, yaml-cpp is fetched and built from source (not pre-installed). This adds ~10-15s to CI configure time on cache-cleared builds.

### Migration

No migration is required. yaml-cpp is a new dependency for new functionality (the Asset Manager). No existing code uses YAML parsing, and no existing dependency is replaced.

## Related

- **ADR-001**: `Result<T>` / `Error` pattern — establishes the error propagation convention that yaml-cpp exceptions are converted to.
- **ADR-007**: Build fetched dependencies in Release mode — sets the precedent for `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release` on compiled FetchContent dependencies.
- **ADR-008**: Docker-based CI infrastructure — the CI Docker image fetches yaml-cpp at configure time (not pre-installed).
- **CONST-001**: Architecture Boundaries — yaml-cpp is PRIVATE, no types leak into public headers.
- **SPEC-018 / IMPL-018**: Asset Manager feature — the consumer of yaml-cpp.
- `src/engine/CMakeLists.txt`: yaml-cpp `FetchContent_Declare` block (lines 27–39) and PRIVATE link (line 76).
- `src/engine/asset/asset_manager.cpp`: yaml-cpp usage with exception-safe `YAML::LoadFile()` wrappers.

# ADR-018: Add tinygltf as a Dependency for glTF 2.0 Model Loading

## Status

`Accepted`

Allowed values: `Proposed`, `Accepted`, `Superseded`, `Rejected`

## Context

The glTF model loading feature introduces support for importing 3D models in the glTF 2.0 format (both `.gltf` and `.glb` files) via the AssetManager. The engine needs a glTF parsing library to read model data (vertex buffers, index buffers, materials, textures, node hierarchy) at runtime.

### Requirements

1. **Parse glTF 2.0 documents** — Meshes, primitives, accessors, buffer views, buffers, materials (PBR metallic-roughness), textures, images, nodes, scenes, cameras, lights, animations, and extensions metadata.
2. **Support both `.gltf` (JSON + external buffers/images) and `.glb` (binary container) formats** — The library must handle the GLB container format with embedded binary data and external file references.
3. **Header-only or minimal build footprint** — The project values fast configure-and-build cycles. A library that adds significant compilation time or complex linkage is undesirable.
4. **No exceptions required** — The project uses `Result<T>` for error propagation (ADR-001). A parser that uses error codes rather than exceptions aligns naturally with this pattern.
5. **CMake FetchContent integration** — The project manages all third-party dependencies via `FetchContent` (SDL3, GLM, stb, yaml-cpp, Catch2). Any new dependency must follow this pattern.
6. **PRIVATE linkage / internal types** — glTF library types must not appear in public engine headers (CONST-001 principle). The dependency is an implementation detail of the model loading subsystem.

### Why glTF 2.0 over other 3D formats

| Criterion | glTF 2.0 | OBJ | FBX | Collada (DAE) |
|---|---|---|---|---|
| **Industry standard** | Khronos standard, "JPEG of 3D" | Legacy, no PBR materials | Proprietary (Autodesk), complex spec | Legacy, deprecated |
| **PBR material support** | Native (metallic-roughness + specular-glossiness) | Not supported | Supported but format-specific | Limited |
| **Binary format** | `.glb` — single file, self-contained | Not available | Binary version available | XML-based, verbose |
| **Animation support** | Full (skinning, morph targets) | Not supported | Full | Full |
| **Tool/engine support** | Universal (Blender, Maya, Unreal, Godot, three.js) | Universal but limited | Widespread but licensing-restricted | Legacy |
| **Spec complexity** | Medium, well-documented | Simple, but limited | Very large, closed spec | Large, open spec |

**Decision**: glTF 2.0. It is the Khronos-standard, royalty-free, open-spec format that has become the de-facto "JPEG of 3D". Every major DCC tool and game engine supports glTF export/import. PBR material support is native. The binary `.glb` container enables single-file distribution. The format is the clear choice for a modern game engine.

### Why tinygltf over other C++ glTF parsers

| Parser | License | Header-only | FetchContent | Error handling | glTF 2.0 coverage |
|---|---|---|---|---|---|
| **tinygltf** (syoyo) | MIT | Yes (single header, C++11) | Yes (CMake target) | Error codes (`bool` + `std::string* err`) | Full — core spec + PBR + extensions |
| **assimp** (Open Asset Import) | BSD | No (compiled lib, 30+ formats) | Yes (CMake) | Error codes + exceptions | Partial (glTF supported but not primary focus) |
| **cgltf** (jkuhlmann) | MIT | Yes (single header, C99/C++) | Manual (no CMake) | Error codes | Full |
| **nlohmann/json + manual parsing** | MIT | Yes | Yes (CMake) | Exceptions or error codes | N/A — custom parser needed |

**Decision**: tinygltf. It is the most widely used single-header glTF parser in the C++ ecosystem, with full glTF 2.0 spec coverage, native CMake FetchContent support, MIT license, and error-code-based error reporting (no exceptions). Its header-only nature means zero link-time dependencies — just an include path.

assimp was the most seriously considered alternative. It is a mature, battle-tested library supporting 40+ 3D formats. However, its build time (~30+ minutes on first configure), large binary size, and the project's need for only glTF 2.0 made assimp disproportionate. assimp's glTF support is also not its primary focus — glTF-specific features (PBR materials, extensions, Draco compression) are better supported by tinygltf.

cgltf is a strong lightweight alternative — also header-only, with excellent glTF 2.0 coverage. It was considered but rejected because it lacks native CMake/FetchContent support (no CMakeLists.txt), requiring manual integration or a wrapper. tinygltf's CMake integration and wider adoption make it the more maintainable choice for the project's build system.

### Why FetchContent over vendoring or system packages

| Approach | Pros | Cons |
|---|---|---|
| **FetchContent** (selected) | Reproducible builds (git tag pinning); no system install required; automatic download on first configure; matches existing project pattern (SDL3, GLM, stb, yaml-cpp, Catch2) | Download on first build (one-time cost); no system-wide caching |
| **Vendored header** (copy `tiny_gltf.h` into repo) | Zero network dependency; no build-system coupling | Manual updates; bloated git history (the header is ~12k lines); version drift risk; uncommon for project's dependency model |
| **Git submodule** | Pin to exact commit; no CMake-level abstraction | Manual update step; submodule management overhead; requires `add_subdirectory` or `FetchContent`-like handling |
| **System package** | Pre-built binary, no build time | No standard system package for tinygltf; non-reproducible across CI/developer machines |

**Decision**: FetchContent. It is the project's established dependency management pattern. All existing third-party dependencies (SDL3, GLM, stb, yaml-cpp, Catch2) use FetchContent. Adding tinygltf via FetchContent is consistent, requires no new infrastructure, and the header-only nature means no actual compilation — just download + include path setup.

### Exception safety and ADR-001

tinygltf uses error codes, not exceptions. All API functions return `bool` and write error messages to a `std::string*` output parameter:

```cpp
tinygltf::TinyGLTF loader;
tinygltf::Model model;
std::string err;
std::string warn;
bool success = loader.LoadASCIIFromFile(&model, &err, &warn, path);
if (!success) {
    // err contains description
    return make_error(Error::Category::InvalidFormat, "glTF parse error: " + err);
}
```

This aligns naturally with ADR-001's `Result<T>` error propagation pattern. No exception-handling wrappers are needed (unlike yaml-cpp in ADR-016). The pattern is simply: check the `bool` return, propagate the error string as a `Result<T>` error.

### Custom image loader callback

tinygltf bundles its own copy of stb_image for decoding embedded texture data. However, the project already depends on stb (a different revision, pinned by commit hash). To avoid symbol conflicts and ensure consistent image decoding, the project registers a custom image loader callback that decodes image data using the project's own stb_image:

```cpp
bool load_image_data_callback(tinygltf::Image* image, const int /*image_idx*/,
                              std::string* err, std::string* /*warn*/,
                              int /*req_width*/, int /*req_height*/,
                              const unsigned char* bytes, int size,
                              void* /*user_data*/)
{
    int w, h, comp;
    unsigned char* pixels = stbi_load_from_memory(bytes, size, &w, &h, &comp, 4);
    if (!pixels) {
        if (err) *err = "Failed to decode image data: " + std::string(stbi_failure_reason());
        return false;
    }
    image->width = w;
    image->height = h;
    image->component = 4;
    image->image.resize(static_cast<size_t>(w * h * 4));
    std::memcpy(image->image.data(), pixels, image->image.size());
    stbi_image_free(pixels);
    return true;
}
```

The callback is registered via `loader.SetImageLoader(&load_image_data_callback, nullptr)` before any `LoadASCIIFromFile`/`LoadBinaryFromFile` call. This:

- Eliminates the risk of two stb_image symbols conflicting (tinygltf's bundled copy vs the project's own).
- Ensures consistent image decode behaviour (4-channel RGBA output, matching the project's texture creation expectations).
- Uses the same stb_image instance that the rest of the engine uses for non-gltf textures.

### Stb image symbol conflict avoidance

Because tinygltf includes its own `stb_image.h` internally, and the project also depends on `stb` (via FetchContent, a different pinned commit), there is a risk of ODR violations if tinygltf's internal stb_image defines symbols that conflict with the project's.

This is mitigated by two mechanisms:

1. **Custom image loader callback** (described above) — tinygltf calls `load_image_data_callback` instead of its internal stb_image path when this callback is registered. This is the primary mechanism: as long as the callback is registered, tinygltf's internal stb_image code path is not used at runtime.

2. **Compile-time isolation** — tinygltf's header (`tiny_gltf.h`) wraps its stb_image includes in conditional compilation guards (`#ifdef TINYGLTF_USE_CPP_POPEN` / `#ifndef TINYGLTF_NO_STB_IMAGE`). If a compile-time symbol conflict arises, `TINYGLTF_NO_STB_IMAGE` can be defined before including `tiny_gltf.h` to disable the internal stb_image entirely. At the time of writing, the custom callback approach is sufficient — no `TINYGLTF_NO_STB_IMAGE` define is needed.

The dual-mechanism approach (primary: callback; fallback: compile-time define) provides defence-in-depth against stb_image ODR issues.

### CONST-001 implications

CONST-001 (Architecture Boundaries) prohibits platform, graphics, or windowing library headers outside `src/engine/`. tinygltf is a general-purpose data format parser — it is neither a platform, graphics, nor windowing library — so CONST-001 does not directly restrict it.

tinygltf is included as a **PRIVATE** dependency:

```cmake
target_include_directories(buddd_engine PRIVATE
    ${tinygltf_SOURCE_DIR}
)
```

- No public header in `src/engine/` includes `<tiny_gltf.h>`.
- tinygltf types (`tinygltf::Model`, `tinygltf::Primitive`, `tinygltf::TinyGLTF`, etc.) appear only in `.cpp` files (`model_loader.cpp`).
- Consumers of `buddd_engine` (e.g., `src/cmd/`, tests) link against `buddd_engine` PUBLIC dependencies (SDL3, GLM, OpenGL) but NOT tinygltf.
- The internal header `model_loader.h` declares free functions in a `detail` namespace with no tinygltf types in function signatures — the types are fully hidden in the `.cpp` implementation.

This preservation of the abstraction boundary is confirmed by code review (DC-20 in the implementation contract).

**Relation to ADR-007**: tinygltf is a header-only library (no compiled translation unit). The `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release` flag is still passed for consistency, even though it has no effect on a header-only dependency. This follows ADR-007's precedent of passing Release build flags to all FetchContent dependencies regardless of whether they compile.

## Decision

We add **tinygltf v2.9.7** as a FetchContent dependency to `src/engine/CMakeLists.txt`, included as a PRIVATE dependency of `buddd_engine`, with a custom image loader callback using the project's own stb_image to avoid symbol conflicts.

### CMake integration

The dependency is declared after yaml-cpp and before `find_package(OpenGL)`:

```cmake
# ----- tinygltf (glTF 2.0 model loading, header-only) -----
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.9.7
    CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release
              -DTINYGLTF_BUILD_LOADER_EXAMPLE=OFF
              -DTINYGLTF_BUILD_LOADER_TEST=OFF
)
FetchContent_MakeAvailable(tinygltf)
```

And the PRIVATE include path:

```cmake
target_include_directories(buddd_engine PRIVATE
    ${tinygltf_SOURCE_DIR}
)
```

Key details:
- **Header-only**: No `target_link_libraries(... PUBLIC/PRIVATE tinygltf)` is needed — it is not a compiled library.
- **Release build**: Passed as `CMAKE_ARGS` for consistency with ADR-007, though tinygltf is header-only and has no compiled output from this project's build.
- **Examples/tests disabled**: `TINYGLTF_BUILD_LOADER_EXAMPLE=OFF` and `TINYGLTF_BUILD_LOADER_TEST=OFF` prevent tinygltf's own example and test targets from being added to the build (they would otherwise add unnecessary build targets).
- **PRIVATE include path**: tinygltf headers are visible only inside `src/engine/`.

### Scope

| Aspect | Decision |
|---|---|
| **Format** | glTF 2.0 (both `.gltf` and `.glb`) |
| **Parser** | tinygltf v2.9.7 (MIT license) |
| **Integration** | FetchContent (not vendored, submodule, or system package) |
| **Linkage** | PRIVATE (header-only — no link step; include path only) |
| **Build type** | Release (passed as CMAKE_ARGS per ADR-007; header-only so no actual compilation) |
| **Image decoding** | Custom callback using project's stb_image (avoid dual stb_image symbols) |
| **Error handling** | Error codes (`bool` + `std::string* err`) — no exception wrappers needed |
| **Git tag** | Pinned to exact release (`v2.9.7`), not a branch |

### Non-decisions (explicitly out of scope)

- Support for 3D formats other than glTF 2.0 (no OBJ, FBX, Collada, PLY, etc. — if needed, a future ADR may add another library or migrate to assimp).
- Draco mesh compression support (tinygltf supports it via `TINYGLTF_ENABLE_DRACO` but Draco is a separate library — out of scope for V1).
- glTF extension handling beyond the core spec and PBR metallic-roughness (`KHR_materials_pbrSpecularGlossiness` and other extensions are not parsed in V1 — tinygltf parses them into generic extension data but the engine ignores it).
- Writing glTF files from the engine (the engine is read-only for glTF).
- Animation or skinning data (tinygltf parses it, but the engine does not import it in V1).
- tinygltf version auto-update policy.

## Alternatives considered

### 1. assimp (Open Asset Import Library)

**Pros**: Supports 40+ 3D formats, battle-tested (used in hundreds of projects), more features (optimisation, post-processing), BSD license.

**Cons**: Massive build footprint (~30+ minutes first configure on a mid-range developer machine), compiled library (not header-only), most of the 40+ format parsers are irrelevant to the project (only glTF 2.0 is needed), glTF support is not its primary focus (glTF-specific features lag behind tinygltf), adds a link-time dependency.

**Verdict**: Rejected. The build-time cost is disproportionate for a single-format need. If the project ever needs multi-format support, assimp may be reconsidered with prebuilt CI caching to mitigate the configure cost.

### 2. cgltf (jkuhlmann)

**Pros**: Header-only (single `.h` file), C99/C++, MIT license, excellent glTF 2.0 coverage, minimal footprint, error-code-based error reporting.

**Cons**: No native CMake/FetchContent support (no `CMakeLists.txt` — the project ships as a standalone header only). Requires manual integration: either vendoring the header, writing a custom CMake wrapper, or using `FetchContent` with `add_library` boilerplate. Smaller ecosystem and fewer examples than tinygltf.

**Verdict**: Rejected. cgltf is a technically excellent library and was the closest alternative. The lack of native CMake support was the deciding factor — every other dependency in the project has a CMakeLists.txt that integrates naturally with `FetchContent`. Writing and maintaining a custom CMake wrapper would add maintenance overhead with no functional benefit over tinygltf.

### 3. Manual glTF parsing (nlohmann/json + custom GLB reader)

**Pros**: Zero new dependencies, complete control, no ODR or symbol-conflict concerns.

**Cons**: Enormous implementation effort — the glTF 2.0 spec is ~200+ pages covering JSON schema, buffer views, accessors, binary GLB container, PBR materials, animations, skins, extensions, and validation rules. Custom code would need to handle all edge cases (padding, alignment, endianness, URI resolution, base64 decoding, Draco, etc.). No benefit over using an existing well-tested library.

**Verdict**: Rejected. The effort of reimplementing what tinygltf already provides is orders of magnitude larger than integrating and maintaining the library, with no commensurate benefit.

### 4. Custom JSON schema + manual mesh format

**Pros**: Maximum control, no dependency.

**Cons**: Not an industry standard — any tool that needs to export models must write a custom exporter. No existing tool ecosystem, no community tooling, no validation. The project would lose all the benefits of the glTF ecosystem (Blender export, Khronos sample models, validator tools).

**Verdict**: Rejected. Using an industry-standard format (glTF 2.0) means the engine can load models from any DCC tool that exports glTF. A custom format would require the project to create and maintain its own toolchain.

### 5. Vendored tinygltf header (copy `tiny_gltf.h` into repo)

**Pros**: Zero network dependency at configure time; no external fetch step; complete control over version.

**Cons**: `tiny_gltf.h` is ~12k lines — committing it bloats the git history unnecessarily. Manual update process (must re-copy on version bumps). Breaks the project's consistent FetchContent pattern. Risk of drift between the vendored copy and the upstream version.

**Verdict**: Rejected. FetchContent provides the same version-pinning guarantees without polluting the repo with generated/large third-party files. The project's established pattern is FetchContent for all dependencies (SDL3, GLM, stb, yaml-cpp, Catch2) — vendoring would be inconsistent and harder to maintain.

## Consequences

### Positive

- **Consistent dependency management**: tinygltf follows the same FetchContent pattern as SDL3, GLM, stb, yaml-cpp, and Catch2. No new build infrastructure is needed.
- **Zero link-time dependency**: Header-only means no compiled library, no link step, no shared library versioning concerns. Just an include path.
- **PRIVATE inclusion preserves abstraction**: Consumers of `buddd_engine` are not coupled to tinygltf. No tinygltf types leak into public headers.
- **Error-code-based error handling**: tinygltf's `bool` + `std::string* err` pattern aligns naturally with ADR-001's `Result<T>` — no exception wrappers needed (unlike yaml-cpp in ADR-016).
- **Fast first-configure**: tinygltf is header-only — `FetchContent_MakeAvailable` downloads the header but performs no compilation. First-configure time is effectively just the download time.
- **Custom image loader avoids ODR conflicts**: Using the project's own stb_image via callback eliminates the risk of dual stb_image symbol definitions.
- **Full glTF 2.0 spec coverage**: tinygltf parses meshes, accessors, buffer views, PBR materials, textures, images, nodes, scenes, cameras, lights, animations, and all core extensions — the engine can import the full breadth of glTF 2.0 content.
- **Battle-tested library**: tinygltf is used extensively in the glTF ecosystem (Khronos validator tools, three.js exporter, countless game engines and tools). Known edge cases are already handled.

### Negative

- **glTF-only format support**: The engine cannot load OBJ, FBX, Collada, or any other 3D format. If multi-format support is needed in the future, a second library or a migration to assimp would be required.
- **Community-maintained**: tinygltf is maintained by a single developer (syoyo) and community contributors. There is no corporate backing or guaranteed release cadence. However, the library is mature (active since 2016) and widely adopted in the Khronos ecosystem.
- **stb_image ODR risk**: While the custom callback is the primary mitigation, a future tinygltf version might change how it bundles stb_image, potentially requiring re-evaluation of the mitigation strategy. The `TINYGLTF_NO_STB_IMAGE` compile-time define is available as a fallback.
- **No Draco/meshopt compression in V1**: tinygltf supports Draco via an optional define, but adding Draco would require a separate Draco dependency. This is explicitly deferred.
- **Header-only is not "no cost"**: Even though tinygltf is header-only, it is still ~12k lines of C++ that must be parsed by the compiler in every translation unit that includes it. In practice, only `model_loader.cpp` includes `<tiny_gltf.h>`, so the parse cost is incurred once.

### Compliance

- All glTF model loading code SHALL use tinygltf via the internal `model_loader.cpp` module. Direct use of tinygltf types from other files (especially public headers) SHALL NOT occur.
- tinygltf parse errors SHALL be converted to `Result<T>` errors using `Error::Category::InvalidFormat` (for parse failures) or `IoFailed` (for I/O errors where applicable).
- The custom image loader callback SHALL be registered before any `LoadASCIIFromFile`/`LoadBinaryFromFile` call. Any code path that loads glTF images without the callback SHALL be rejected.
- tinygltf types SHALL NOT appear in any public header of `buddd_engine`. This is enforced by code review (DC-20).

## Related

- **ADR-001**: `Result<T>` / `Error` pattern — establishes the error propagation convention that tinygltf error codes are converted to.
- **ADR-007**: Build fetched dependencies in Release mode — sets the precedent for `CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release` on FetchContent dependencies.
- **ADR-013**: Standard Vertex Format — the 72-byte vertex struct that tinygltf data is converted to during model loading.
- **ADR-016**: yaml-cpp dependency — establishes the FetchContent + PRIVATE linkage pattern that tinygltf follows, and contrasts exception-based (yaml-cpp) vs error-code-based (tinygltf) error handling.
- **ADR-017**: Multi-Material Model architecture — the `SubMesh`/`material_index` design that glTF loading populates.
- **CONST-001**: Architecture Boundaries — tinygltf is PRIVATE, no types leak into public headers.
- **SPEC / IMPL gltf-model-loading**: glTF model loading feature — the consumer of tinygltf.
- `src/engine/CMakeLists.txt`: tinygltf `FetchContent_Declare` block (lines 41–50) and PRIVATE include path (line 78).
- `src/engine/asset/model_loader.cpp`: tinygltf usage with error-code-based error handling and custom image loader callback.

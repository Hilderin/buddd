# Implementation Contract Review — glTF Model Loading

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [x] **BL-001: Internally inconsistent error category for corrupt glTF files (Test #10 vs implementation behavior)**

  **RESOLVED**: A new `Error::Category::InvalidFormat` was added to the enum, and ALL references to corrupt glTF errors now consistently use `InvalidFormat`:
  - Line 167 (preamble): mandates `InvalidFormat` for corrupt files
  - Line 163 (enum): `InvalidFormat` added to `Category` enum
  - Line 343 (impl behavior): parse failure → `InvalidFormat`
  - Line 609 (Test #10): expects `InvalidFormat`
  - Line 726 (DC-12): `Corrupt glTF → InvalidFormat`
  - Line 112 (modified files): `error.h` listed for `InvalidFormat` addition

  Fix verified across all contract locations. No inconsistency remains.

## Warnings

Non-blocking concerns for awareness:

- **W-001: `ModelLoadResult::materials` vector appears unused after construction**

  The `ModelLoadResult` struct (line 307–310) includes a `materials` vector alongside the `root` node. Materials are also stored inside `Model` objects within the tree (via `Model::create_indexed()`). The contract's `load_model()` step 9 says "Record materials from `ModelLoadResult::materials` (they are owned by the Model's materials vectors in the tree)" — but this is a descriptive comment, not an action. The implementer may be confused about what to actually *do* with this vector (log it? ignore it?). Consider documenting whether the vector exists only for observability or if it's a leftover from an earlier design.

- **W-002: Several spec error cases lack explicit test coverage**

  The following error cases from the spec (lines 620–639) are not explicitly tested in the contract's test table:
  - YAML syntax error → `IoFailed` (spec line 624) — missing a dedicated test case
  - YAML `source` field missing or empty → `InvalidArgument` (spec line 627)
  - glTF source file not found → `IoFailed` (spec line 628) — Test #10 covers corrupt files, not missing files
  - Out-of-bounds accessor/buffer view indices → `InvalidArgument` (spec line 631)

  These may be implicitly covered by the AssetManager's existing test infrastructure or by other tests (e.g., Test #11 covers type mismatch, not missing `source` field), but they are not explicitly tracked. Adding test cases or noting in the contract where they are covered would improve completeness.

- **W-003: Spec-critic warnings W-001 through W-006 are not addressed**

  The spec review identified 6 warnings that remain unresolved:
  - **W-001**: `RenderDevice::create_texture()` API compatibility for embedded textures is unverified (Assumptions A-07/A-08). The contract relies on `Image::create(ImageBuffer)` + `device.create_texture(image)` but does not verify this path exists.
  - **W-002**: AC-015 title says "compiles and renders" but verification only checks known uniforms.
  - **W-003**: `doubleSided` flag stored but rendering effect unspecified (the contract documents this, which is good, but doesn't resolve the spec-critic's concern).
  - **W-004**: No license/attribution for Khronos test models documented.
  - **W-005**: Hot-reload handler extension for `.gltf`/`.glb` file changes is assumed but not explicitly confirmed in scope.
  - **W-006**: AC-017 requires pixel-level headless texture inspection — capability not confirmed.

  The contract does not need to resolve these spec-level warnings, but the code-implementer should be aware of them as risks.

- **W-004: No test for normal texture loading**

  Test #6 ("Load DamagedHelmet — success, PBR materials have textures") verifies that textures are set but doesn't specifically verify the `normal_texture` slot is populated or that all 5 texture slots (base_color, metallic_roughness, normal, occlusion, emissive) are correctly mapped. The DamagedHelmet model has a normal texture — a more explicit test for the normal map slot would improve coverage.

- **W-005: `tinygltf` exception safety not addressed**

  The contract wraps yaml-cpp calls in try-catch (line 149) but does not specify whether `tinygltf` can throw exceptions, or whether calls to `TinyGLTF::LoadASCIIFromFile()` / `LoadBinaryFromFile()` need exception handling. If the project may compile with `-fno-exceptions`, this could be a problem. Consider documenting whether tinygltf exceptions are expected or whether the project already handles this.

- **W-006: Missing `.bin` file edge case — edge case table (line 648) says `IoFailed`, but implementation behavior (line 343) would return `InvalidFormat`**

  Contract line 648 (edge case table) states: "If the `.bin` is missing, tinygltf parse fails → `IoFailed` error." However, the implementation behavior (line 343) specifies: "On parse failure: return `make_error(Error::Category::InvalidFormat, ...)`" — which wraps ALL tinygltf parse failures (including a missing `.bin` file) as `InvalidFormat`.

  These are inconsistent. The implementation behavior is prescriptive (what the code must do) and the edge case table is descriptive (what is expected), so the code will likely return `InvalidFormat` for a missing `.bin` file. Either update the edge case to say `InvalidFormat`, or add code to detect a missing `.bin` before calling tinygltf and return `IoFailed` separately.

  This is a pre-existing spec-level inconsistency (spec line 606 says IoFailed for missing .bin, while spec line 629 says InvalidFormat for corrupt/invalid). The contract should resolve this one way or the other.

## Required changes

All previously requested changes are now resolved:

1. **[x] Resolve BL-001**: Test #10, implementation behavior, preamble, DC-12, and modified files now consistently use `InvalidFormat`. (See detailed fix notes in the BL-001 entry above.)

## Suggested improvements

Optional ideas (not required):

- Consider adding an explicit test for "YAML source field missing or empty → InvalidArgument" to close the error case gap.
- Consider adding an explicit test for "glTF source file not found → IoFailed" to distinguish missing-file errors from corrupt-file errors.
- Consider documenting whether `ModelLoadResult::materials` is used for logging/observability or is vestigial.
- Consider verifying that `Image::create(const ImageBuffer&)` actually exists in the engine before the implementation depends on it (this was flagged as W-001 in the spec-critic).

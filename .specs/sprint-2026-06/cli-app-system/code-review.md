# Implementation Contract Review — CLI App System

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [ ] **Scene abort message uses 0-based frame number (violates implementation contract)**

  In `src/cmd/app.cpp` line 100:
  ```cpp
  std::fprintf(stderr, "Scene aborted by user (frame %d)\n", frame);
  ```
  The `frame` variable is 0-based (0 = first rendered frame), but the implementation contract step 10 explicitly states:
  > `"Scene aborted by user (frame N)" to stderr (frame is 1-based)`

  This should print `frame + 1` instead of `frame` to be 1-based. All other frame numbers in observability messages (capture specs, frame_limit messages) are 1-based — this inconsistency is a bug.

## Warnings

Non-blocking concerns for awareness:

- **`--frame 0` edge case reconciled as error**: The spec's edge case table says `--frame 0` is "treated as no limit (interactive mode)" but the error cases table says `N < 1` is invalid. The implementation treats `--frame 0` as an error (`n < 1`), consistent with the error cases table. This is a strict interpretation but the spec has an internal inconsistency. Not a bug, but worth noting.

- **`is_running()` public accessor not in spec**: The `App` class adds `[[nodiscard]] auto is_running() const noexcept -> bool` as a public accessor for `running_`. The spec shows `running_` as protected with no public accessor. Since `run_app()` is a free function (not a member), it cannot access `running_` directly — the accessor is necessary and well-designed.

- **`app.h` includes `error.h` from engine path**: `src/cmd/app.h` includes `"error.h"`, which resolves to `src/engine/error.h` via CMake include directories rather than a relative path. This works because the engine's include directory is added by the build system, but it's implicit. The same pattern exists in other cmd files, so this is consistent with existing conventions.

- **Extra-argument warning format differs from spec language**: Spec says format is `"Warning: unexpected arguments: arg1 arg2"`, implementation says `"Warning: unexpected arguments after '<scene>': arg1 arg2"`. The implementation contract specified this alternate format, the spec critic review accepted it, and the test checks for substring `"Warning: unexpected arguments"`. Non-issue but worth noting.

- **No unit tests for `parse_running_args()`**: The contract lists AP-01 through AP-10 (unit tests for flag parsing) but these are tested indirectly via CLI integration tests (`cli_app_tests.cpp` and `cmd_tests.cpp`). The spec acknowledges this in the implementer's warnings: "Unit tests for parse_running_args() are tested indirectly via CLI integration tests (subprocess calls)." All 279 tests pass.

- **Aspect ratio change (800×600 → 1024×768)**: All scenes now use 1024×768 window (previously 800×600 for demos). This is intentional per the spec but changes rendered output and camera aspect ratios. CubeApp and CubeSceneApp correctly use `static_cast<float>(config().width) / config().height` (1024/768) for perspective projection.

- **PhongApp removes ~200 lines of manual RenderSystem reimplementation**: The old `phong_capture.cpp` manual light-collection/MeshRenderer-iteration code is gone. PhongApp now uses `RenderSystem::render_scene()`. This is correct per the spec but visual verification is needed to confirm identical rendering.

## Required changes

Concrete, actionable changes requested:

- Fix frame numbering in `src/cmd/app.cpp` line 100: Change `frame` to `frame + 1` in the "Scene aborted by user" message to match the 1-based numbering required by the implementation contract.

## Suggested improvements

Optional ideas (not required):

- Add explicit unit tests for `parse_running_args()` (AP-01 through AP-10) calling the function directly from `app_config.h`, as described in the implementation contract, rather than relying solely on subprocess-based integration tests.
- Consider adding a brief comment in `app.cpp` at the abort-message line noting the 1-based convention.
- The `run_buddd` test helper's handling of frame-limited scenes (like `"run triangle extra_arg"` which runs for 120 frames) could benefit from an explicit `--frame` argument to avoid depending on the offscreen driver's ability to complete frames rapidly.

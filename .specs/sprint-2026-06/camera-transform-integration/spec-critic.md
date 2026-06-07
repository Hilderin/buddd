# Spec Review — Camera → Transform Integration

## Blocking issues

No blocking issues found. The spec satisfies all Definition of Ready criteria.

## Warnings

Non-blocking concerns for awareness:

- **Missing: `src/engine/math/math.h` update** — `math/math.h` currently includes `#include "camera.h"` (line 12). The spec lists `math/math.h` for adding the `view_matrix()` free function but does not mention removing the `#include "camera.h"` include directive. When `camera.h` is deleted, this include will fail to compile. This should be added to the files-to-modify list.

- **Incomplete wiki documentation update list** — The spec lists `docs/wiki/domain/business-rules.md` and `docs/wiki/architecture/module-map.md` for updates, but other wiki files that directly reference `math::Camera` are not listed:
  - `docs/wiki/domain/glossary.md` (lines 67, 74: Camera class definition; line 93: CameraComponent wraps math::Camera)
  - `docs/wiki/architecture/overview.md` (lines 71-72: camera.h/camera.cpp; line 77: CameraComponent wrapping math::Camera)
  
  While the wiki-agent will handle these comprehensively, listing them in the impact analysis would be more accurate.

- **E2E verification relies partly on visual inspection** — The E2E verification section says "or visual inspection" for app rendering correctness. Visual inspection is not automated or repeatable. The `--capture` + screenshot comparison approach is the correct path; the spec should commit to it rather than offering visual inspection as an alternative.

## Required changes

Concrete, actionable changes requested:

- Add `math/math.h` update (remove `#include "camera.h"`) to the files-to-modify list under "Engine core".

## Suggested improvements

Optional ideas (not required):

- Consider expanding the "Engine core" files-to-modify table to include the removal of `#include "camera.h"` from `math/math.h`.
- Consider listing `docs/wiki/domain/glossary.md` and `docs/wiki/architecture/overview.md` in the wiki files-to-modify section for completeness.
- Consider tightening the E2E verification language to prefer automated `--capture` comparison over visual inspection.

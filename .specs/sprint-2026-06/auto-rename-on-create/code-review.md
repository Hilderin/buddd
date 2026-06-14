# Implementation Contract Review — Auto-Rename on Create

## Blocking issues

Items that must be resolved before the artifact can be accepted.

- [ ] **Scope creep: right-click selection logic added without authorization** — The implementation adds a ~30-line right-click selection handler in `scene_panel.cpp` (entity right-click → selection modification before context menu opens) that is NOT specified in the implementation contract. This logic implements a separate feature (`scene-panel-right-click`) documented in the untracked spec directory `.specs/sprint-2026-06/scene-panel-right-click/`. The auto-rename-on-create contract authorizes only the 4 listed files with specific changes. Adding right-click selection logic in the same diff:
  - Makes the review diff 115 lines instead of the expected ~85 for auto-rename alone
  - Links two independent features in a single uncommitted change, preventing independent revert/review
  - Modifies behavior (AC-32 in `entity-operations/spec.md` changed from "Context menu does not change selection" to "right-click selects") without any mention in the auto-rename spec or contract

- [ ] **Forbidden files modified** — The following files are modified but are NOT in the allowed files list per the implementation contract, and some (`docs/`) are explicitly forbidden:
  - `docs/wiki/editor/editor-panels.md` (forbidden — wiki updates handled by wiki-agent separately)
  - `docs/wiki/editor/entity-selection.md` (forbidden — same reason)
  - `.specs/sprint-2026-06/entity-operations/spec.md` (changed AC-32 from "right-click does not select" to "right-click selects")
  - `.specs/sprint-2026-06/entity-selection/spec.md` (removed NG-10 about right-click behavior)
  - `experiments-spec-driven-dev.md` (added an unrelated observation)

## Warnings

Non-blocking concerns for awareness:

- **Forward declaration instead of `#include` in `scene_panel.h`** — The contract specified adding `#include "commands/create_entity_command.h"` in the header, but a forward declaration `class CreateEntityCommand;` is used instead, with the include placed in `scene_panel.cpp`. The implementer noted a circular dependency issue. Functionally equivalent (pointer member only needs forward decl), but deviates from the contract's explicit instruction. Acceptable but should be documented and acknowledged.

- **Auto-rename confirmed log duplicates entity ID** — The debug log in `confirm_rename()` auto-rename path outputs:
  ```
  "Auto-rename confirmed: entity {} named \"{}\"", id.index, pending_create_command_->created_entity_id().index
  ```
  Both `{}` placeholders resolve to the same entity ID, producing output like `entity 5 named "5"` instead of including the actual name string (e.g., `entity 5 named "Player"`). This follows the contract's code block but contradicts the spec's observability table which says `name` should be logged. Minor logging quality issue — no functional impact.

- **`pending_create_command_` pointer stability** — The raw pointer stored from `cmd.get()` before `std::move(cmd)` into the command stack relies on the invariant that moving a `unique_ptr` does not change the pointed-to object's address. This is a valid assumption for `std::unique_ptr` but is implicit. The design would benefit from an explicit comment documenting this safety guarantee.

## Required changes

Concrete, actionable changes requested:

- **Remove scope creep**: Revert the right-click selection logic from `scene_panel.cpp` (the entity right-click handler block that modifies selection before opening context menu). If the right-click selection feature is ready, it should be implemented and reviewed as a separate change, not mixed with auto-rename-on-create.
- **Revert forbidden file modifications**: Revert changes to `docs/wiki/`, `.specs/sprint-2026-06/entity-operations/spec.md`, `.specs/sprint-2026-06/entity-selection/spec.md`, and `experiments-spec-driven-dev.md`. Wiki updates are handled by the wiki-agent in a separate step. Specs are historical snapshots that should not be modified as part of implementation work.

## Suggested improvements

Optional ideas (not required):

- Fix the auto-rename confirmed log to include the entity name (capture `new_name` before `std::move`):
  ```cpp
  auto name_copy = new_name;
  pending_create_command_->set_post_creation_name(std::move(new_name));
  BUDDD_LOG_TAGGED_DEBUG("Editor:ScenePanel",
      "Auto-rename confirmed: entity {} named \"{}\"", id.index, name_copy);
  ```
- Add a comment documenting the pointer safety invariant for `pending_create_command_` (unique_ptr move does not change object address).

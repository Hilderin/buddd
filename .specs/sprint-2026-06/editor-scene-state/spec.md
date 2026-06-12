# SPEC-029 — Editor Scene State + [[nodiscard]] Fixes

## Problem

The `Editor` class currently has no scene state — it manages menus, panels, and keyboard shortcuts but owns no `World`. Every future panel feature (hierarchy, inspector, viewport, play mode) needs a World reference to display entities, components, and transforms. Without this plumbing:

- Panels would need to reach into `ctx.world` (the engine's demo-scene world), coupling the editor to the demo pipeline.
- There is no clear place to store the editor's working scene.
- The editor cannot provide an empty World on launch.

This phase establishes the foundation: the `Editor` class gets a `std::unique_ptr<World>` member, **created in the Editor constructor** (so `world()` always returns a valid `World&` for the entire Editor lifetime) and destroyed in the destructor via `unique_ptr`.

Additionally, 11 `[[nodiscard]]` warnings from the editor-foundation code review (ignored return values from `CommandStack::undo()` / `CommandStack::redo()`) were identified for resolution. These fixes have already been applied to the codebase; this phase verifies the build produces zero warnings.

## Goals

| ID | Goal |
|---|---|
| G-01 | **World ownership**: `Editor` owns a `std::unique_ptr<World>` created during **Editor construction** and destroyed during Editor destruction. The World is empty on construction. |
| G-02 | **World accessor**: `Editor` exposes a `world()` method returning `World&` that is safe to call at any point during the Editor's lifetime. |
| G-03 | **Clean lifecycle**: World is automatically destroyed via `unique_ptr` when the `Editor` destructor runs. No manual cleanup of the World is required in `shutdown()`. |
| G-04 | **Zero warnings**: Verify that all 11 `[[nodiscard]]` warnings from ignored `CommandStack::undo()` / `CommandStack::redo()` return values have been eliminated, and the build produces zero warnings from `src/editor/` and `tests/`. |
| G-05 | **Unit test coverage**: Tests verify the World accessor, empty-world state, and lifecycle (create/destroy). |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | No panels use the World yet (that is F-02+). Panels continue to operate as they do today. |
| NG-02 | No scene load/save (F-01). The World is always empty on creation. |
| NG-03 | No entity operations — no create, delete, select, rename, or transform. |
| NG-04 | No changes to `EngineContext` or `ctx.world`. The editor's World is separate from the engine's demo-scene world. |
| NG-05 | No `SceneLoader` / `SceneSaver` integration. |
| NG-06 | No changes to `EditorApp` or `run_app()`. |
| NG-07 | No changes to panels, menus, shortcuts, or the command system beyond the `[[nodiscard]]` warning verification. |
| NG-08 | No changes to the World class itself (`src/engine/scene/world.h` is used as-is). |
| NG-09 | No changes to engine code (`src/engine/`). |
| NG-10 | No assertion or UB pattern for `world()` — the World is always valid (constructed in constructor, destroyed in destructor). No debug assertions needed. |

## Actors

| Actor | Description |
|---|---|
| **Editor developer** | A developer adding new editor panels or features. Calls `editor.world()` to access the editor's World instance. Never needs null checks or guards. |
| **Code reviewer** | Verifies that `[[nodiscard]]` warnings are resolved and the build is clean. |

## User-visible behavior

There is **no user-visible change** in this phase. The editor runs identically to the post-editor-foundation state — same panels, same menu bar, same empty workspace. The World exists internally but no panel displays it yet.

The only externally observable change is:

1. The build produces **zero warnings** from `src/editor/` and `tests/` (previously 11 `[[nodiscard]]` warnings — already fixed, verified).

## Key entities

| Entity | Description |
|---|---|
| **`Editor`** | Top-level editor class. Gains a `std::unique_ptr<World>` member created in the constructor and a `world()` accessor that is always safe to call. |
| **`World`** | Engine class from `src/engine/scene/world.h`. Default-constructed empty world with `entity_count() == 0`. Non-copyable, non-movable. |

### Interface changes

**`Editor` class** (`src/editor/editor.h`):

```cpp
/// Returns a reference to the editor's World.
/// Always valid — created in the constructor, destroyed in the destructor.
/// Safe to call at any point during the Editor's lifetime.
[[nodiscard]] auto world() -> buddd::engine::World&;
```

New private member:

```cpp
std::unique_ptr<buddd::engine::World> world_;
```

**`Editor` constructor** — creates the World via `std::make_unique<World>()`.

**`Editor` destructor** — `unique_ptr` automatically destroys the World. No manual cleanup needed.

**`Editor::shutdown()`** — unchanged for the World member. The World lives for the Editor's entire lifetime. `shutdown()` continues to clean up other state (`engine_`, `window_`, `initialized_`).

## User stories

### Story 1 — Editor owns a World lifecycle (Priority: P1)

As an editor developer, I want the Editor to own a World instance from construction to destruction, so that panels can later use it without null-checking or checking lifecycle state.

**Given** a fresh `Editor`
**When** it is constructed
**Then** the editor has a valid non-null World
**And** `world().entity_count() == 0`

**Given** an editor that is about to be destroyed
**When** the `Editor` destructor runs
**Then** the World is destroyed as part of Editor destruction (handled by `unique_ptr`)

### Story 2 — World accessor is always safe (Priority: P1)

As a panel developer, I want `world()` to return a non-nullable reference at any point during the Editor's lifetime, so that I can use it directly without null checks, assertions, or guards.

**Given** an editor at any point (before setup, after setup, after shutdown)
**When** I call `editor.world()`
**Then** I receive a `World&` reference to a valid, empty World

### Story 3 — Editor destruction cleans up the World (Priority: P1)

As a developer, I want the World to be cleaned up automatically when the Editor is destroyed, so that there are no resource leaks.

**Given** an editor
**When** the `Editor` object goes out of scope (destructor runs)
**Then** the World is destroyed as part of Editor destruction (handled by `unique_ptr`)

### Story 4 — Zero-warnings build (Priority: P1)

As a developer, I want the editor to compile with zero warnings in `src/editor/` and `tests/`, so that the CI build passes cleanly.

**Given** the codebase with previous `[[nodiscard]]` warnings (already fixed)
**When** I build with `cmake --build --preset debug`
**Then** no warnings are emitted from files in `src/editor/` or `tests/`

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `Editor` has a `std::unique_ptr<World>` member created in the **constructor** (not setup). | Unit test: construct Editor, verify `editor.world().entity_count() == 0`. No `setup()` call required. |
| AC-002 | `world()` returns a valid `World&` immediately after construction, before `setup()` is called. | Unit test: construct Editor without calling `setup()`, verify `world()` returns a reference to a non-null World object. Verify by calling `entity_count()` on the returned reference. |
| AC-003 | World is destroyed when the `Editor` destructor runs. The `unique_ptr` handles automatic cleanup. | Unit test: construct Editor (World created), let `Editor` go out of scope, verify no memory leak (ASan/Valgrind clean). No manual `shutdown()` needed. |
| AC-004 | World is empty on construction. | Unit test: after construction (no setup), `world().entity_count() == 0` and `world().root_entity_count() == 0`. |
| AC-005 | Editor holds a valid World for its entire lifetime — the World outlives `shutdown()`. | Unit test: construct Editor, call `setup()` then `shutdown()`, verify `world().entity_count() == 0` (World still accessible after shutdown). |
| AC-006 | Build produces zero warnings from `src/editor/` and `tests/`. Previously identified warning sites (3 in `editor.cpp`, 2 in `menu_bar.h`, 6 in `editor_tests.cpp`) are verified to use suppression patterns or consume return values. | Build with `cmake --build --preset debug`. Verify zero warnings. |
| AC-007 | `world()` is declared with `[[nodiscard]]`. | Inspect `editor.h`: verify `[[nodiscard]] auto world() -> buddd::engine::World&;` is declared. |
| AC-008 | `world()` does **not** require assertions or guards — it is always safe to call. The World is constructed in the Editor constructor and destroyed in the destructor. | Unit test: construct Editor, call `world()` at any lifecycle point (before setup, after setup, after shutdown), verify it returns a valid `World&`. |
| AC-009 | The World is created before any `setup()` call — the constructor creates it. If `setup()` fails (e.g., ImGui not initialized), the World persists and is cleaned up by the destructor. | Unit test: headless test where `setup()` fails (ImGui not initialized); verify `world()` still returns a valid reference and no memory leak on destruction (ASan clean). |
| AC-010 | `shutdown()` does not reset or invalidate the World — the World remains valid. | Unit test: call `setup()`, `shutdown()`, then `world().entity_count() == 0`. |

## E2E Verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Build with `BUDDD_HAS_DISPLAY=OFF`. Run `buddd_tests`. Verify `[editor][scene_state]` tagged tests pass — World lifecycle, accessor, empty state, and nodiscard fix verification. |
| **Clean build verification (CI)** | Run `cmake --build --preset debug` and verify zero warnings from `src/editor/` and `tests/`. |
| **Manual smoke test (display)** | Run `buddd edit`. Verify editor launches and behaves identically to before (menus, panels all work). No user-visible change expected. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A panel developer can call `editor.world()` at any point (before setup, after setup, after shutdown) and receive a valid `World&` to an empty World, without any null checks, assertions, or error handling. | Developer creates a minimal panel, registers it, calls `editor.world().entity_count()` in various lifecycle stages, builds, and runs. |
| SC-002 | Build compiles with zero warnings from `src/editor/` and `tests/`. | `cmake --build --preset debug` completes with zero warnings in the affected files. |
| SC-003 | All existing tests pass and no regressions. | `buddd_tests` suite passes with 100% of tests at baseline count or higher (new tests added). |
| SC-004 | The World destruction does not leak memory. | ASan / Valgrind pass on editor lifecycle tests. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **`world()` called before `setup()`** | Returns valid `World&` to an empty World. Always safe — World was created in constructor. |
| **`world()` called after `shutdown()`** | Returns valid `World&` to an empty World. Always safe — World lives for the Editor's entire lifetime. |
| **`setup()` called twice without `shutdown()`** | Unprotected (consistent with current Editor patterns — double-setup is undefined behaviour). Not a supported use case. |
| **`shutdown()` called twice** | Idempotent. Second call is a no-op (consistent with current `Editor::shutdown()` which sets `initialized_ = false` and nulls pointers). World remains valid. |
| **`setup()` returns an error (e.g., ImGui not initialized)** | `setup()` returns an error. World persists in the Editor (created in constructor) and is cleaned up by the destructor. No leak. |
| **Editor destroyed without calling `shutdown()`** | Destructor calls `shutdown()` (current pattern) which cleans up engine_/window_ pointers. `unique_ptr` handles World destruction automatically. |
| **`world()` on a moved-from Editor** | Editor is not movable (no move constructor/assignment declared). Consistent with current Editor design. |
| **Editor destroyed with World still holding entities** | `unique_ptr<World>::reset()` destroys the World and all its entities. No leak. |

## Error cases

| Case | Expected behavior |
|---|---|
| **World construction failure (out of memory)** | `std::make_unique<World>` throws `std::bad_alloc`. This is consistent with existing project patterns (exceptions are not banned, though `Result<T>` is preferred for predictable errors). The exception propagates from the Editor constructor — callers should not catch it. |
| **`[[nodiscard]]` warnings not fully eliminated** | The build reports warnings. CI fails. The 11 identified sites are already fixed (verified by inspection); if any new warning sites were introduced by this phase's code changes, they must also be resolved. |

## Permissions and security

- No changes to permissions or security posture.
- The World is owned entirely within the `Editor` class — no file I/O, no network access, no elevated privileges.
- No sensitive data is involved.

## Observability

| Signal | Source |
|---|---|
| **World creation** | Log `BUDDD_LOG_DEBUG("Editor: created empty World")` in the Editor constructor when the World is created. |
| **World destruction** | Log `BUDDD_LOG_DEBUG("Editor: destroyed World")` in the Editor destructor when the World is destroyed. No need for a special log in `shutdown()` — the destructor handles it. |
| **World access** | No special logging. `world()` is a trivial accessor and always safe. |

## File changes

### Created

None. No new files are introduced.

### Modified

| File | Change |
|---|---|
| `src/editor/editor.h` | Add `#include "scene/world.h"` (or appropriate path). Add `std::unique_ptr<World> world_` private member. Add `[[nodiscard]] auto world() -> buddd::engine::World&;` public accessor declaration. |
| `src/editor/editor.cpp` | In constructor: create `world_` via `std::make_unique<World>()` (member-initializer list or body). Implement `world()` accessor returning `*world_`. Add logging for World creation/destruction. |
| `tests/editor_tests.cpp` | Add `[editor][scene_state]` test cases: World accessor before setup, World accessor after shutdown, empty-world state, lifecycle leak check (ASan). |
| `docs/wiki/architecture/module-map.md` | Update Editor class entry in the editor section: add `world_` member and `world()` accessor. |
| `docs/wiki/editor/editor-panels.md` | Note that Editor now owns a `World` via `unique_ptr`, created in the constructor and available via `editor.world()`. |
| `docs/wiki/editor/scene-management.md` | Update to reflect World lifecycle in Editor: World is created in the constructor, valid for entire Editor lifetime, destroyed in destructor. |

### Unchanged

| File | Reason |
|---|---|
| `src/engine/scene/world.h` | Used as-is. No changes to the World class. |
| `src/engine/` | All engine files. No changes needed. |
| `src/cmd/app.cpp` / `app.h` | No lifecycle changes — EditorApp setup/shutdown remains the same. |
| `src/editor/panels/menu_bar.h` | `[[nodiscard]]` fixes already applied (verified). No further changes. |
| All other files | No changes needed. |

## Out of scope

- Panels referencing `editor->world()` — deferred to F-02+.
- Scene loading / saving (F-01).
- Any entity operations (create, delete, select, rename, transform).
- Changes to `EngineContext`, `ctx.world`, or any engine APIs.
- `SceneLoader` / `SceneSaver` integration.
- Changes to `EditorApp` or `run_app()`.
- Changes to any panel, menu, shortcut, or command logic (the nodiscard fixes are already applied and only need verification).
- Changes to existing test infrastructure — new tests reuse the existing headless setup pattern.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `World` default constructor creates a completely empty world (`entity_count() == 0`, `root_entity_count() == 0`). Verified by inspecting `world.h` and existing usage. |
| A-02 | `World` is non-copyable and non-movable (confirmed by `world.h`: copy/move constructors and assignment operators are `= delete`). The `unique_ptr` holder is appropriate. |
| A-03 | `unique_ptr<World>::reset()` properly destroys the `World` and releases memory. No custom deleter is needed. |
| A-04 | The 11 `[[nodiscard]]` warning sites are already fixed in the current codebase using the `[[maybe_unused]] auto _` pattern in `src/editor/editor.cpp` (3 sites) and `src/editor/panels/menu_bar.h` (2 sites), and via `REQUIRE()`/`REQUIRE_FALSE()` assertions in `tests/editor_tests.cpp` (6 sites). Zero warnings from these sites have been verified by code inspection. No additional sites exist. |
| A-05 | The fix pattern for `[[nodiscard]]` warnings (`[[maybe_unused]] auto _` or void-cast) is already applied and correct. No further changes needed for the warnings. |
| A-06 | The existing headless test setup (`tests/editor_tests.cpp`) can be extended with new test cases for World lifecycle without breaking existing tests. |
| A-07 | The Editor constructor remains minimal — no additional constructor arguments are needed beyond the default. |
| A-08 | The Editor destructor already calls `shutdown()` (current implementation). The World's `unique_ptr` member is destroyed after `shutdown()` runs (data member destruction order is reverse of declaration), so the World outlives `shutdown()`. |
| A-09 | The `#include` for `World` in `editor.h` follows the project convention: `#include "scene/world.h"` with appropriate `buddd::engine` namespace usage. |
| A-10 | `shutdown()` does not need to reset the World `unique_ptr` — the World lives for the Editor's entire lifetime. `shutdown()` continues to clean up `engine_`, `window_`, and `initialized_` as before. |

## Open questions

None. All questions were resolved during the spec-critic review and human decision process:

- **Q-01 (resolved)**: Should `world()` have an assertion for pre-setup/post-shutdown access? **Resolved**: No assertion needed. The World is created in the constructor, so `world()` is always safe. The always-valid-World pattern eliminates the need for any assertion or UB guard.

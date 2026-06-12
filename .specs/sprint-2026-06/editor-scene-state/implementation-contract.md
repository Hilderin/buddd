# IMPL-029 — Editor Scene State + [[nodiscard]] Fixes

## Source spec

- `.specs/sprint-2026-06/editor-scene-state/spec.md`

## Goal

Add a `std::unique_ptr<World>` member to the `Editor` class, created in the constructor (via `std::make_unique<World>()`), exposed via a `[[nodiscard]] auto world() -> buddd::engine::World&` accessor that is always safe to call at any lifecycle point (before `setup()`, after `setup()`, after `shutdown()`). The World is automatically destroyed by `unique_ptr` when the Editor destructor runs. The `shutdown()` method does not reset or invalidate the World — the World lives for the Editor's entire lifetime. 

Additionally, verify that all 11 `[[nodiscard]]` warnings from the editor-foundation code review (ignored `CommandStack::undo()` / `CommandStack::redo()` return values) are already fixed in the codebase and the build produces zero warnings from `src/editor/` and `tests/`.

## Non-goals

- No panels use the World yet — that is deferred to future features (F-02+).
- No scene load/save (F-01). The World is always empty on creation.
- No entity operations — no create, delete, select, rename, or transform.
- No changes to `EngineContext` or `ctx.world`. The editor's World is a **separate** instance.
- No `SceneLoader` / `SceneSaver` integration.
- No changes to `EditorApp` or `run_app()`.
- No changes to panels, menus, shortcuts, or the command system beyond the `[[nodiscard]]` warning verification.
- No changes to the `World` class itself (`src/engine/scene/world.h` is used as-is).
- No changes to engine code (`src/engine/`).
- No assertion or UB pattern for `world()` — the World is always valid (constructed in constructor, destroyed in destructor). No debug assertions needed.
- No changes to `src/editor/panels/menu_bar.h` — the `[[nodiscard]]` fixes are already applied there.

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-027 (Editor Architecture) | Editor class with direct member variables, `buddd::editor` namespace, `buddd_editor` static library. The new `world_` member follows the existing direct-member pattern. |
| ADR-019 (Architecture Boundaries) | No SDL3/OpenGL/GLM headers outside `src/engine/`. The `world.h` include in `editor.h` is a permitted engine abstraction include. |
| ADR-026 (Dear ImGui Integration) | ImGui frame lifecycle is automated. Not affected by this change. |
| ADR-011 (Ownership/Nullability/NoDiscard) | `[[nodiscard]]` conventions. `world()` is declared `[[nodiscard]]`. |
| ADR-001 (Result/Error Pattern) | `Result<void>` pattern for fallible APIs. Editor constructor does not return a Result — World creation throws `std::bad_alloc` on OOM (consistent with existing project patterns). |
| ADR-029 (Editor UX Decisions) | This phase establishes the World that future panels (hierarchy, inspector, viewport, play mode) will use. No changes to UX yet. |

## Files to inspect

| File | Reason |
|---|---|
| `src/editor/editor.h` | Current Editor class declaration — must add `world_` member and `world()` accessor. |
| `src/editor/editor.cpp` | Current Editor implementation — must modify constructor, add `world()` implementation, add logging. |
| `src/engine/scene/world.h` | World class API — verify `entity_count()`, `root_entity_count()`, default constructor. Used as-is. |
| `tests/editor_tests.cpp` | Existing test patterns — must add `[editor][scene_state]` test cases. Already includes `"scene/world.h"`. |
| `src/editor/panels/menu_bar.h` | Verify `[[nodiscard]]` warning fixes are already applied (2 sites use `[[maybe_unused]] auto _`). |
| `src/editor/shortcut_registry.h` | Understand `ShortcutRegistry` API — the `bind` action signature takes `EngineContext const&`. |

## Files allowed to change

| File | Change type |
|---|---|
| `src/editor/editor.h` | **modify** — Add `#include "scene/world.h"`, `std::unique_ptr<buddd::engine::World> world_` private member, `[[nodiscard]] auto world() -> buddd::engine::World&;` public accessor declaration. |
| `src/editor/editor.cpp` | **modify** — In constructor: create `world_` via `std::make_unique<buddd::engine::World>()`. Implement `world()` returning `*world_`. Add `BUDDD_LOG_DEBUG("Editor: created empty World")` logging in constructor. Add `BUDDD_LOG_DEBUG("Editor: destroyed World")` logging in destructor. No changes to `shutdown()` for the World member. |
| `tests/editor_tests.cpp` | **modify** — Add `[editor][scene_state]` tagged test cases for World lifecycle, accessor, empty state, and shutdown persistence. |

## Files forbidden to change

- Any file under `src/engine/` — no engine changes for this feature.
- `src/editor/panels/menu_bar.h` — `[[nodiscard]]` fixes already applied, no further changes.
- `src/cmd/apps/editor_app.h` / `src/cmd/apps/editor_app.cpp` — no lifecycle changes.
- `src/cmd/app.h` / `src/cmd/app.cpp` — no changes.
- `tests/CMakeLists.txt` — no changes needed.
- `src/editor/CMakeLists.txt` — no changes needed (no new `.cpp` files).
- Any existing `.h`/`.cpp` files not listed in "Files allowed to change".
- Any wiki or ADR files (wiki updates are the wiki-agent's responsibility).

## Existing conventions to follow

1. **Include style**: `#include "..."` for project headers (relative to `src/engine/`, `src/editor/`); `<...>` for system/external headers.
2. **Namespace**: `buddd::editor` for all editor code. Use unindented namespace blocks (project style).
3. **`#pragma once`**: Already present in `editor.h`. Do not remove.
4. **`[[nodiscard]]`**: All `Result<T>`-returning and boolean-query functions must be marked `[[nodiscard]]`. The `world()` accessor is marked `[[nodiscard]]` per spec AC-007.
5. **Direct member variables**: No PIMPL — direct members for simplicity (per ADR-027). The new `world_` member follows this pattern.
6. **Mutable `unique_ptr`**: Non-const `std::unique_ptr<World>` (not const/immutable) since the pointer itself is not reassigned after construction — only the pointed-to World is used.
7. **Include order**: Project headers first (alphabetical), then external headers. The `#include "scene/world.h"` in `editor.h` should be placed in alphabetical order among other project includes.
8. **Logging**: Use `BUDDD_LOG_TAG("Editor")` (already present in `editor.cpp`). Use `BUDDD_LOG_DEBUG` for World creation/destruction messages. `BUDDD_LOG_DEBUG` macro is defined in `"log/log.h"` which is already included in `editor.cpp`.
9. **Test pattern**: Catch2 `TEST_CASE("name", "[tag]")` with `#include <catch2/catch_test_macros.hpp>`. Use `REQUIRE()` and `REQUIRE_FALSE()` for assertions. The `[editor][scene_state]` tag should be used for new World lifecycle tests.
10. **Type aliases**: `editor.cpp` already uses `namespace be = buddd::engine;`. Use `be::World` for World references and `be::EngineContext` for context.
11. **Forward declarations**: Prefer forward declarations to includes in headers where possible. However, `World` is needed as a complete type (used with `unique_ptr`), so `#include "scene/world.h"` is required in `editor.h`.
12. **Constructor initializer list**: The World member should be initialized in the constructor body (via `std::make_unique`) rather than in the initializer list, since it requires heap allocation. The existing constructor body is the right place.
13. **Member declaration order**: Data member destruction order is reverse of declaration. Place `world_` after `show_about_` (after all other members that might reference it in their destructors). The `unique_ptr` will be destroyed first when declared last.

## Required implementation behavior

### Step 1: Modify `src/editor/editor.h`

**Changes:**

1. Add `#include "scene/world.h"` in the include section after `#include "shortcut_registry.h"` (alphabetical order among project includes: `scene/world.h` comes after `shortcut_registry.h`).

2. Add the public accessor declaration after the existing public methods, before the `private:` section:

```cpp
    /// Returns a reference to the editor's World.
    /// Always valid — created in the constructor, destroyed in the destructor.
    /// Safe to call at any point during the Editor's lifetime.
    [[nodiscard]] auto world() -> buddd::engine::World&;
```

3. Add the private member variable at the end of the private section (last member, so it is destroyed first per reverse-declaration-order):

```cpp
    // Editor's own World (separate from ctx.world)
    std::unique_ptr<buddd::engine::World> world_;
```

**Exact placement details:**
- The `#include "scene/world.h"` line should go between `#include "shortcut_registry.h"` and `#include <memory>`.
- The `world()` declaration should go after `shutdown()` and before `private:`.
- The `world_` member should go after `show_about_` (i.e., last private member, since `unique_ptr` destruction should happen first among members — reverse declaration order means declaring it last causes it to be destroyed first).

**Verification:** File compiles with no errors. `world_` member is `std::unique_ptr<buddd::engine::World>`. `world()` returns `buddd::engine::World&` and is marked `[[nodiscard]]`.

### Step 2: Modify `src/editor/editor.cpp`

**Changes:**

1. **Constructor** — Replace the existing defaulted constructor with one that creates the World:

```cpp
Editor::Editor()
    : world_(std::make_unique<be::World>())
{
    BUDDD_LOG_DEBUG("Editor: created empty World");
}
```

2. **Destructor** — Add logging for World destruction. The existing destructor body calling `shutdown()` remains unchanged. Add a log statement before or after `shutdown()`:

```cpp
Editor::~Editor() {
    BUDDD_LOG_DEBUG("Editor: destroyed World");
    shutdown();
}
```

Note: The log is placed before `shutdown()` so that the World destruction log appears before the shutdown log. The `unique_ptr` automatically destroys the World after the destructor body completes (member destruction is reverse declaration order).

3. **`world()` accessor** — Add after the destructor (or in a logical location, e.g., after the constructor and before `setup()`):

```cpp
auto Editor::world() -> be::World& {
    return *world_;
}
```

No null check needed — `world_` is always non-null because it is created in the constructor. No `[[nodiscard]]` on the definition (the declaration in the header carries the attribute).

4. **No changes to `shutdown()`** — The World lives for the Editor's entire lifetime. `shutdown()` continues to clean up `initialized_`, `engine_`, and `window_` only.

**Verification:** File compiles. Constructor creates `world_` via `make_unique<World>()`. `world()` returns `*world_`. Logging is present. No new includes needed (the existing includes already cover everything needed: `"log/log.h"` for `BUDDD_LOG_DEBUG`, and the header provides `"scene/world.h"`).

### Step 3: Add tests to `tests/editor_tests.cpp`

**Changes:**

Add the following test cases after the existing `[editor]` lifecycle test (line 171). All new tests use the `[editor][scene_state]` tag.

**Test case 1 — World accessor before setup (AC-001, AC-002, AC-004):**

```cpp
TEST_CASE("Editor: world() returns valid empty World before setup", "[editor][scene_state]") {
    buddd::editor::Editor editor;

    // world() must return a valid World& before any setup() call
    auto& w = editor.world();
    REQUIRE(w.entity_count() == 0);
    REQUIRE(w.root_entity_count() == 0);
}
```

**Test case 2 — World persists after shutdown (AC-005, AC-010):**

```cpp
TEST_CASE("Editor: world() valid after setup+shutdown", "[editor][scene_state]") {
    // Create a headless engine for setup()
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Editor Test", .width = 128, .height = 128});
    REQUIRE(engine.has_value());
    auto& eng = **engine;

    auto world = std::make_unique<buddd::engine::World>();
    auto render_system = std::make_unique<buddd::engine::RenderSystem>(eng.device(), *world);

    buddd::engine::EngineContext ctx{
        eng, eng.window(), eng.device(), *world, *render_system, 0.016f, 0
    };

    buddd::editor::Editor editor;

    // World is accessible before setup
    REQUIRE(editor.world().entity_count() == 0);

    // Setup (may fail in headless, that's OK)
    auto result = editor.setup(ctx);
    (void)result;

    // World is accessible after setup
    REQUIRE(editor.world().entity_count() == 0);

    // Shutdown
    editor.shutdown();

    // World is accessible after shutdown — must still be valid
    REQUIRE(editor.world().entity_count() == 0);
}
```

**Test case 3 — World persists through setup() failure (AC-009):**

```cpp
TEST_CASE("Editor: world() valid after setup failure", "[editor][scene_state]") {
    // Create a headless engine (same as existing test — setup will fail because ImGui is not initialized)
    auto engine = buddd::engine::EngineService::create(
        buddd::engine::Backend::Headless,
        buddd::engine::WindowConfig{.title = "Editor Test", .width = 128, .height = 128});
    REQUIRE(engine.has_value());
    auto& eng = **engine;

    auto world = std::make_unique<buddd::engine::World>();
    auto render_system = std::make_unique<buddd::engine::RenderSystem>(eng.device(), *world);

    buddd::engine::EngineContext ctx{
        eng, eng.window(), eng.device(), *world, *render_system, 0.016f, 0
    };

    buddd::editor::Editor editor;

    // Setup will fail (ImGui not initialized in headless mode)
    auto result = editor.setup(ctx);
    REQUIRE_FALSE(result.has_value());  // Verify setup() actually fails

    // World must still be valid even after setup() failure
    REQUIRE(editor.world().entity_count() == 0);

    // No leak: Editor destructor handles cleanup (verified by ASan/Valgrind at test level)
}
```

**Test case 4 — World is empty on construction (AC-004 expanded):**

```cpp
TEST_CASE("Editor: World is empty on construction", "[editor][scene_state]") {
    buddd::editor::Editor editor;

    REQUIRE(editor.world().entity_count() == 0);
    REQUIRE(editor.world().root_entity_count() == 0);
}
```

**Verification:** All 4 new test cases compile and pass with `ctest --preset debug` or `buddd_tests`. ASan/Valgrind detect no leaks.

### Step 4: Verify zero-warnings build (AC-006)

The `[[nodiscard]]` warning fixes are already applied in the codebase:
- `src/editor/editor.cpp`: 3 sites use `[[maybe_unused]] auto _` (lines 93, 96, 99)
- `src/editor/panels/menu_bar.h`: 2 sites use `[[maybe_unused]] auto _` (lines 43, 46)
- `tests/editor_tests.cpp`: 6 sites consume return values via `REQUIRE()`/`REQUIRE_FALSE()` or use `[[maybe_unused]]`

After implementing Steps 1-3, verify:
1. `cmake --build --preset debug` succeeds with zero warnings from `src/editor/` and `tests/`.
2. No new `[[nodiscard]]` warnings are introduced by the new code changes (the `world()` accessor is properly marked `[[nodiscard]]`).

## Required tests

### Unit tests

All unit tests go in `tests/editor_tests.cpp`. They must be tagged with `[editor][scene_state]`.

| Test | AC covered | What it verifies |
|---|---|---|
| `Editor: world() returns valid empty World before setup` | AC-001, AC-002, AC-004 | Construct Editor without `setup()`, `world()` returns valid `World&`, `entity_count() == 0`, `root_entity_count() == 0` |
| `Editor: world() valid after setup+shutdown` | AC-005, AC-008, AC-010 | Construct Editor, call `setup()`, `shutdown()`, then verify `world().entity_count() == 0` (World still accessible and empty) |
| `Editor: world() valid after setup failure` | AC-009 | Construct Editor, call `setup()` in headless mode (fails), verify `world()` still returns valid reference with `entity_count() == 0` |
| `Editor: World is empty on construction` | AC-004 | Construct Editor, verify `entity_count() == 0` and `root_entity_count() == 0` |

### E2E / Integration verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Build with `BUDDD_HAS_DISPLAY=OFF`. Run `buddd_tests`. Verify `[editor][scene_state]` tagged tests pass. |
| **Clean build verification (CI)** | Run `cmake --build --preset debug` and verify zero warnings from `src/editor/` and `tests/`. |
| **ASan/Valgrind** | Run the test suite under AddressSanitizer or Valgrind and verify no memory leaks from `Editor` lifecycle (World creation/destruction). Existing CI infrastructure handles this. |
| **Manual smoke test (display)** | Run `buddd edit`. Verify editor launches and behaves identically to before (menus, panels all work). No user-visible change expected. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **`world()` called before `setup()`** | Returns valid `World&` to an empty World. Always safe — World was created in constructor. Verified by AC-001/AC-002 test. |
| **`world()` called after `shutdown()`** | Returns valid `World&` to an empty World. Always safe — World lives for the Editor's entire lifetime. Verified by AC-005/AC-010 test. |
| **`setup()` returns an error (ImGui not initialized)** | `setup()` returns error. World persists (created in constructor) and is cleaned up by destructor. Verified by AC-009 test. |
| **Editor destroyed without calling `shutdown()`** | Destructor calls `shutdown()` (existing pattern) which cleans up `engine_`/`window_` pointers. `unique_ptr` handles World destruction automatically. No leak. |
| **Editor destroyed with World still holding entities** | `unique_ptr<World>::reset()` destroys the World and all its entities. No leak. |
| **`world()` on a moved-from Editor** | Editor is not movable (no move constructor/assignment declared). Consistent with current Editor design. |
| **World construction failure (OOM)** | `std::make_unique<World>` throws `std::bad_alloc`. Exception propagates from Editor constructor — callers should not catch it. Consistent with existing project patterns. |
| **`setup()` called twice without `shutdown()`** | Unprotected (consistent with current Editor patterns — double-setup is undefined behaviour). Not a supported use case. |
| **`shutdown()` called twice** | Idempotent. Second call is a no-op (existing behavior). World remains valid. |
| **New `[[nodiscard]]` warnings introduced by this phase** | Zero warnings must be maintained. If any new warning sites are introduced, they must be resolved using the same `[[maybe_unused]] auto _` or `REQUIRE()` patterns. |

## Security impact

No security impact. The World is owned entirely within the `Editor` class — no file I/O, no network access, no elevated privileges. No sensitive data is involved.

## Data and migration impact

None. No schema changes, no data migrations, no seed data, no data loss risks.

## API compatibility impact

- **New public method**: `Editor::world()` is added. This is an additive change — all existing code continues to compile without modification.
- **No existing API is changed**: `Editor::setup()`, `Editor::shutdown()`, `Editor::update()`, `Editor::draw_ui()` signatures remain unchanged.
- **No backward compatibility concerns**: The `world()` method is a pure addition; no existing callers are affected.

## Documentation impact

- **README**: None. The README does not document editor internals.
- **Wiki pages** (to be created/updated by wiki-agent):
  - `docs/wiki/architecture/module-map.md` — Add Editor class entry: `world_` member and `world()` accessor.
  - `docs/wiki/editor/editor-panels.md` — Note that Editor now owns a `World` via `unique_ptr`, created in the constructor and available via `editor.world()`.
  - `docs/wiki/editor/scene-management.md` — Update to reflect World lifecycle in Editor: World is created in the constructor, valid for entire Editor lifetime, destroyed in destructor.
- **Other specs**: None.

## ADR impact

No new ADR needed. The implementation follows existing ADRs:
- ADR-027 (direct member variables, no PIMPL)
- ADR-011 (`[[nodiscard]]` conventions)
- ADR-001 (Result/Error pattern for fallible APIs — constructor throws on OOM which is consistent)

No existing ADR is deprecated or amended.

## Done criteria

The Code Agent must satisfy all of the following:

- [ ] **DC-01**: `src/editor/editor.h` includes `#include "scene/world.h"` and declares `[[nodiscard]] auto world() -> buddd::engine::World&;` in the public section and `std::unique_ptr<buddd::engine::World> world_;` in the private section.
- [ ] **DC-02**: `src/editor/editor.cpp` constructor creates `world_` via `std::make_unique<be::World>()` and logs `BUDDD_LOG_DEBUG("Editor: created empty World")`.
- [ ] **DC-03**: `src/editor/editor.cpp` destructor logs `BUDDD_LOG_DEBUG("Editor: destroyed World")` before calling `shutdown()`.
- [ ] **DC-04**: `src/editor/editor.cpp` implements `auto Editor::world() -> be::World& { return *world_; }`.
- [ ] **DC-05**: `src/editor/editor.cpp` `shutdown()` does NOT reset or modify `world_`.
- [ ] **DC-06**: `tests/editor_tests.cpp` contains 4 new `[editor][scene_state]` test cases:
  - [ ] `"Editor: world() returns valid empty World before setup"` — verifies `entity_count() == 0` without calling `setup()`.
  - [ ] `"Editor: World is empty on construction"` — verifies `entity_count() == 0` and `root_entity_count() == 0`.
  - [ ] `"Editor: world() valid after setup+shutdown"` — verifies `world().entity_count() == 0` after full lifecycle.
  - [ ] `"Editor: world() valid after setup failure"` — verifies world is valid after setup() returns an error.
- [ ] **DC-07**: `cmake --build --preset debug` succeeds with **zero warnings** from `src/editor/` and `tests/`.
- [ ] **DC-08**: All `[editor][scene_state]` tests pass: `ctest --preset debug` or `buddd_tests`.
- [ ] **DC-09**: No memory leaks detected by ASan/Valgrind in the Editor lifecycle tests.
- [ ] **DC-10**: No changes to `src/engine/scene/world.h`, `src/editor/panels/menu_bar.h`, `src/cmd/apps/editor_app.*`, `src/cmd/app.*`, or any file not listed in "Files allowed to change".

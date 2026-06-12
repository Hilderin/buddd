# IMPL-F-02 — Scene Panel — Entity Tree

## Source spec

- `.specs/sprint-2026-06/scene-panel-entity-tree/spec.md`

## Goal

Create `struct EditorContext` (with `Editor& editor` and `EngineContext const& engine`), change `EditorPanel` and `EditorMenu` virtual method signatures from `EngineContext const&` to `EditorContext const&`, update all 5 existing panels + MenuBar to use the new signature, and implement `ScenePanel::draw_ui()` to render the entity hierarchy tree from `ctx.editor.world()` using ImGui `TreeNodeEx` with recursive parent-child traversal, leaf/non-leaf distinction, empty-state fallback, and ID collision prevention.

## Non-goals

- No entity selection (deferred to F-03).
- No entity creation, deletion, renaming, drag-and-drop, search/filter, context menu, or virtualized tree.
- No changes to `Editor` public API (`update()`, `draw_ui()` keep `EngineContext const&` signatures).
- No changes to engine files (`src/engine/`), including `World`, `Entity`, `EntityId`, `EngineContext`.
- No changes to `CMakeLists.txt` files (all changed files are header-only except `editor.cpp` which already exists in the build target).
- No changes to private `Editor` methods (`draw_about_popup`, `draw_pending_op_modal`, `execute_pending_op`, etc.) — their signatures stay as-is.
- No changes to `ShortcutRegistry` callback signatures (they remain `(EngineContext const&)`).
- No changes to `tests/editor_tests.cpp` for F-02-specific tests (tests for F-02 will be in a separate test file or new test section; see "Required tests").

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-027 (Editor Architecture) | Editor library with panels and menus registered via `add_panel()`/`add_menu()`. `EditorContext` is a new aggregate following direct-member conventions. |
| ADR-029 (Editor UX Decisions) | Scene Panel (Hierarchy) is the entity tree — this F-02 implements the first working panel. |
| ADR-026 (Dear ImGui Integration) | ImGui docking branch used for TreeNodeEx, PushID, PopID, Text, TreePop — all standard ImGui widgets. |
| ADR-019 (Architecture Boundaries) | No SDL3/OpenGL/GLM headers outside `src/engine/`. `EditorContext.h` forward-declares engine types. |
| ADR-011 (Ownership/Nullability/NoDiscard) | `EditorContext` holds references (always valid), no null checks needed. |

## Files to inspect

| File | Reason |
|---|---|
| `src/editor/editor.h` | Current class declaration — `Editor` has `world()` accessor, no direct reference to `EditorContext`. Verify no changes needed. |
| `src/editor/editor.cpp` | Current dispatch loop — `ctx` (`EngineContext const&`) passed directly to `menu->update(ctx)`, `panel->update(ctx)`, `menu->draw_ui(ctx)`, `panel->draw_ui(ctx)`. Must change to pass `EditorContext{*this, ctx}`. |
| `src/editor/editor_panel.h` | Base class — `update()` and `draw_ui()` take `EngineContext const&`. Must change to `EditorContext const&`. |
| `src/editor/editor_menu.h` | Base class — same signature change as `EditorPanel`. |
| `src/editor/panels/scene_panel.h` | Current empty placeholder. Must implement full entity tree in `draw_ui()`. |
| `src/editor/panels/properties_panel.h` | Signature change only (`EngineContext const&` → `EditorContext const&`). |
| `src/editor/panels/console_panel.h` | Signature change only. |
| `src/editor/panels/project_panel.h` | Signature change only. |
| `src/editor/panels/assets_panel.h` | Signature change only. |
| `src/editor/panels/menu_bar.h` | Signature change + internal `on_quit_(ctx)` call must become `on_quit_(ctx.engine)`. |
| `src/engine/scene/entity.h` | Entity API: `name()`, `child_count()`, `get_child()`, `id()`, `EntityId::index`. Verify available methods. |
| `src/engine/scene/entity_id.h` | `EntityId` struct: `uint32_t index`, `uint32_t generation`. `PushID` uses `index`. |
| `src/engine/scene/world.h` | World API: `entity_count()`, `root_entity_count()`, `get_root_entity()`. Verify signatures. |
| `src/engine/engine_context.h` | Current `EngineContext` struct (for reference — not changed). |
| `tests/editor_tests.cpp` | Existing test patterns — `HeadlessTestContext` helper, `[editor]`/`[f01]` tags, `REQUIRE` style. |

## Files allowed to change

| File | Change type |
|---|---|
| `src/editor/editor_context.h` | **create** — new struct definition |
| `src/editor/editor_panel.h` | **modify** — parameter type change only |
| `src/editor/editor_menu.h` | **modify** — parameter type change only |
| `src/editor/editor.cpp` | **modify** — construct `EditorContext` and pass to panels/menus |
| `src/editor/panels/scene_panel.h` | **modify** — signature change + entity tree implementation |
| `src/editor/panels/properties_panel.h` | **modify** — signature change only |
| `src/editor/panels/console_panel.h` | **modify** — signature change only |
| `src/editor/panels/project_panel.h` | **modify** — signature change only |
| `src/editor/panels/assets_panel.h` | **modify** — signature change only |
| `src/editor/panels/menu_bar.h` | **modify** — signature change + `ctx.engine` usage in quit callback |

## Files forbidden to change

- Any file under `src/engine/` — no changes to `World`, `Entity`, `EntityId`, `EngineContext`, or any other engine file.
- `src/editor/editor.h` — no public API changes needed; `Editor` class is fine as-is.
- `src/cmd/apps/editor_app.*` — no lifecycle changes.
- `src/cmd/app.*` — no changes.
- `tests/editor_tests.cpp` — existing tests untouched (F-02 tests will be in a new file or conditional section).
- `tests/CMakeLists.txt` — no changes needed (`GLOB_RECURSE` picks up new test files automatically).
- Any `CMakeLists.txt` — no build system changes.
- Any wiki or ADR files (wiki-agent handles documentation).

## Existing conventions to follow

1. **Include style**: `#include "..."` for project headers, `<...>` for system/external headers. Relative to `src/engine/`, `src/editor/`.
2. **Namespace**: `buddd::editor` for editor code. Unindented namespace blocks.
3. **`#pragma once`**: All new headers.
4. **Forward declarations preferred**: Use forward declarations in base class headers where possible to minimize include dependencies.
5. **`type aliases`**: `editor.cpp` uses `namespace be = buddd::engine;`.
6. **Recursive lambda in headers**: Use C++20 generic recursive lambda for tree traversal (existing pattern allows `requires` constraints).
7. **Header-only panels**: All 5 panel files are header-only (no `.cpp`). `scene_panel.h` will remain header-only with the tree inline.
8. **Include order**: Project headers first (alphabetical), then external headers.
9. **ImGui include**: `<imgui.h>` for all ImGui types. Already used in several panel headers.
10. **`PushID`/`PopID` pairing**: Each `PushID` must have a matching `PopID` — standard RAII-like pattern with manual pop.
11. **Entity iteration**: Use `root_entity_count()`/`get_root_entity()` for roots, `child_count()`/`get_child()` for children, matching the existing engine API.

## Required implementation behavior

### Step 1: Create `src/editor/editor_context.h`

New file with exact content:

```cpp
#pragma once

#include <cstdint>  // not strictly needed for forward declarations, but kept for consistency

namespace buddd::editor { class Editor; }
namespace buddd::engine { struct EngineContext; }

namespace buddd::editor {

/// Lightweight aggregate providing panels and menus with access to
/// both the Editor (for editor-specific state, e.g. editor.world())
/// and the EngineContext (for engine services, e.g. ctx.engine).
struct EditorContext {
    Editor& editor;
    buddd::engine::EngineContext const& engine;
};

} // namespace buddd::editor
```

- `Editor` is forward-declared (complete type not needed for reference member).
- `EngineContext` is forward-declared.
- No `#include` of `editor.h` or `engine_context.h` — forward declarations suffice.
- No constructors, destructors, or virtual methods — pure aggregate.

### Step 2: Modify `src/editor/editor_panel.h`

Changes:
1. Remove forward declaration `namespace buddd::engine { struct EngineContext; }` (no longer used).
2. Add forward declaration `struct EditorContext;` (same namespace as `EditorPanel`).
3. Change parameter types:
   - `virtual auto update(buddd::engine::EngineContext const& /*ctx*/) -> void {}`
     → `virtual auto update(EditorContext const& /*ctx*/) -> void {}`
   - `virtual auto draw_ui(buddd::engine::EngineContext const& /*ctx*/) -> void {}`
     → `virtual auto draw_ui(EditorContext const& /*ctx*/) -> void {}`

The forward declaration is sufficient because the method declarations only use `EditorContext const&` (reference to incomplete type is valid for declarations). No `#include "editor_context.h"` is needed in the base class header.

### Step 3: Modify `src/editor/editor_menu.h`

Identical changes as `editor_panel.h`:
1. Remove `namespace buddd::engine { struct EngineContext; }`.
2. Add `struct EditorContext;` forward declaration.
3. Change both `update()` and `draw_ui()` parameter types from `buddd::engine::EngineContext const&` to `EditorContext const&`.

### Step 4: Modify `src/editor/editor.cpp`

Changes:
1. Add `#include "editor_context.h"` in alphabetically sorted position among project headers (between `#include "editor.h"` and `#include "engine_context.h"` — actually after `editor.h` and before `panels/menu_bar.h` per alphabetical order: `"editor_context.h"` comes before `"engine_context.h"` and `"panels/..."`).
2. In `Editor::update(be::EngineContext const& ctx)`:
   - Create `auto editor_ctx = EditorContext{*this, ctx};` before the menu/panel loops.
   - Change `menu->update(ctx)` to `menu->update(editor_ctx)`.
   - Change `panel->update(ctx)` to `panel->update(editor_ctx)`.
3. In `Editor::draw_ui(be::EngineContext const& ctx)`:
   - Create `auto editor_ctx = EditorContext{*this, ctx};` before the menu/panel loops.
   - Change `menu->draw_ui(ctx)` to `menu->draw_ui(editor_ctx)`.
   - Change `panel->draw_ui(ctx)` to `panel->draw_ui(editor_ctx)`.
4. No changes to public signatures: `update(be::EngineContext const& ctx)` and `draw_ui(be::EngineContext const& ctx)` stay unchanged.
5. No changes to private methods (`draw_about_popup`, `draw_pending_op_modal`, `execute_pending_op`, etc.).

### Step 5: Modify 4 placeholder panels (signature-only)

In each of:
- `src/editor/panels/properties_panel.h`
- `src/editor/panels/console_panel.h`
- `src/editor/panels/project_panel.h`
- `src/editor/panels/assets_panel.h`

Change `draw_ui(buddd::engine::EngineContext const& /*ctx*/)` to `draw_ui(EditorContext const& /*ctx*/)`.

No `#include "editor_context.h"` is needed — the forward declaration is inherited through `editor_panel.h`. Method bodies remain empty (placeholders for future features).

### Step 6: Modify `src/editor/panels/menu_bar.h`

Changes:
1. Add `#include "editor_context.h"` after `"command_stack.h"` (alphabetical: `editor_context.h` after `editor_menu.h`... wait, `editor_menu.h` is already included at the top: `#include "editor_menu.h"`. So place after `"command_stack.h"` — actually, project includes are: `"editor_menu.h"`, `"command_stack.h"`, then `"editor_context.h"` would go between `"command_stack.h"` and `<functional>`. Let me verify alphabetical: `c` before `e`, `e` before `f`. Yes: `"command_stack.h"`, then `"editor_context.h"`, then `"editor_menu.h"`, then `<functional>`.

Actually, `editor_menu.h` is already first. And `command_stack.h` is second. So `editor_context.h` would go between `command_stack.h` and `<functional>`:
```
#include "editor_menu.h"
#include "command_stack.h"
#include "editor_context.h"
```

Wait, that's alphabetical: `editor_menu.h` (e), `command_stack.h` (c) ... no, `c` comes before `e`. Let me look at the actual order in the file:
```cpp
#include "editor_menu.h"
#include "command_stack.h"
```

This is not alphabetical. But that's the existing order. For consistency, I'll add `editor_context.h` after `command_stack.h`:
```cpp
#include "editor_menu.h"
#include "command_stack.h"
#include "editor_context.h"
```

2. Change `draw_ui(buddd::engine::EngineContext const& ctx)` to `draw_ui(EditorContext const& ctx)`.
3. Inside `draw_ui`, change the quit callback invocation from `on_quit_(ctx)` to `on_quit_(ctx.engine)`.
4. The `on_quit_` callback type remains `std::function<void(buddd::engine::EngineContext const&)>` — no change needed.

### Step 7: Implement `src/editor/panels/scene_panel.h`

1. Add `#include <imgui.h>` after `"editor_panel.h"` and `#include "editor_context.h"` (for complete type, needed for `ctx.editor.world()` access).
2. Add `#include "scene/world.h"` to access World API (`entity_count`, `root_entity_count`, `get_root_entity`).
3. Change `draw_ui()` signature to take `EditorContext const& ctx`.
4. Implement the entity tree rendering as follows:

```cpp
auto draw_ui(EditorContext const& ctx) -> void override {
    auto& world = ctx.editor.world();

    // Empty state
    if (world.entity_count() == 0) {
        ImGui::Text("No entities");
        return;
    }

    // Recursive helper to render entity subtree
    auto render_entity = [&](auto& self, buddd::engine::Entity entity) -> void {
        ImGui::PushID(static_cast<int>(entity.id().index));

        auto flags = ImGuiTreeNodeFlags_SpanAvailWidth
                   | ImGuiTreeNodeFlags_DefaultOpen;

        if (entity.child_count() == 0) {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        auto name = entity.name();
        if (name.empty()) {
            name = "(unnamed)";
        }

        bool expanded = ImGui::TreeNodeEx(name.c_str(), flags);
        if (expanded) {
            for (size_t i = 0; i < entity.child_count(); ++i) {
                self(self, entity.get_child(i));
            }
            ImGui::TreePop();
        }

        ImGui::PopID();
    };

    // Iterate root entities
    for (size_t i = 0; i < world.root_entity_count(); ++i) {
        auto entity = world.get_root_entity(i);
        if (entity.id() != buddd::engine::EntityId::none()) {
            render_entity(render_entity, entity);
        }
    }
}
```

Key behaviors:
- **Empty state**: `world.entity_count() == 0` → `ImGui::Text("No entities")` and early return. No `TreeNodeEx` calls.
- **Leaf nodes**: `child_count() == 0` → `ImGuiTreeNodeFlags_Leaf` added. No expand arrow.
- **Non-leaf nodes**: No `Leaf` flag, default-expanded via `ImGuiTreeNodeFlags_DefaultOpen`.
- **Width**: `ImGuiTreeNodeFlags_SpanAvailWidth` on all nodes.
- **Name fallback**: `name() == ""` → `"(unnamed)"`.
- **ID collision prevention**: `PushID(static_cast<int>(entity.id().index))` / `PopID` wraps each node.
- **Recursive traversal**: Generic recursive lambda calls `self(self, child)` for each child.
- **Frame-accurate reads**: Tree reads `ctx.editor.world()` every frame — no caching.
- **Null entity guard**: `get_root_entity()` may return `Entity{}` at boundary; `entity.id() != EntityId::none()` guard prevents rendering null entities.

## Required tests

### Unit tests in `tests/editor_tests.cpp` (or new `tests/f02_scene_panel_tests.cpp`)

New test file `tests/f02_scene_panel_tests.cpp` will be created (auto-discovered by CMake `GLOB_RECURSE`). Tests tagged with `[editor][scene_panel]`.

| Test | AC covered | What it verifies |
|---|---|---|
| `F-02: EditorContext struct definition` | AC-07 | Includes `editor_context.h`, verifies struct has `Editor& editor` and `EngineContext const& engine` members, is trivially constructible. |
| `F-02: EditorPanel signatures changed` | AC-08 | Static assertion that `&EditorPanel::update` and `&EditorPanel::draw_ui` accept `EditorContext const&`. Compile-time check — file must compile. |
| `F-02: EditorMenu signatures changed` | AC-09 | Same static check for `EditorMenu`. |
| `F-02: ScenePanel compiles with EditorContext` | AC-10 | Verify `ScenePanel::draw_ui(EditorContext const&)` compiles and is callable. Smoke test: construct `ScenePanel`, create `EditorContext` from `Editor` + headless engine `EngineContext`, call `draw_ui()` — verify no crash (ImGui may not be initialized, but the function body is guarded by the compile check). |
| `F-02: All 5 panels + MenuBar compile with EditorContext` | AC-11 | Include all panel headers + menu_bar.h, verify each compiles with `EditorContext const&` override signatures. Build-level verification (compilation succeeds). |
| `F-02: Zero warnings from src/editor/ and tests/` | AC-12 | Build with `cmake --build --preset debug` — verify zero new warnings from `src/editor/` and `tests/`. |
| `F-02: All existing tests pass` | AC-13 | Run `buddd_tests` after changes — all previously passing tests continue to pass. |

**Note on AC-01 through AC-06 (ImGui call verification):** These acceptance criteria require verifying `ImGui::TreeNodeEx` flag values, `ImGui::Text` call, and entity name formatting. The spec-critic notes that test infrastructure for ImGui call capture is not identified and is an implementation detail. For this phase, AC-01 through AC-06 are verified via:

1. **Code review**: Tree implementation is inspected for correct flag usage (`Leaf`, `DefaultOpen`, `SpanAvailWidth`), `PushID`/`PopID` pairing, empty state early-return, and `"(unnamed)"` fallback. The implementation in this contract specifies exact flag values — code review confirms they match.
2. **Manual smoke test (display)**: Run `buddd edit`, verify Scene panel shows "No entities" on empty World, and visual behaviour is correct.
3. **Display-dependent automated test (future)**: When `imgui_test_engine` or equivalent ImGui capture infrastructure is added, AC-01 through AC-06 can be automated. For now, the build-compile tests (AC-07 through AC-14) cover structural correctness.

### E2E / Integration verification

| Method | Description |
|---|---|
| **Build verification (CI)** | `cmake --build --preset debug` succeeds with zero new warnings from `src/editor/` and `tests/`. |
| **Test suite pass (CI)** | `buddd_tests` — all existing tests pass. No new test failures. |
| **Manual smoke test (display)** | Run `buddd edit`. Verify Scene panel shows "No entities" text (empty World). If a scene is loaded (via F-01 scene load), entity tree displays root entities, parent-child hierarchy is expandable/collapsible, leaf nodes show no expand arrow, unnamed entities show "(unnamed)". |
| **Code review** | Verify `ImGuiTreeNodeFlags_Leaf`, `_DefaultOpen`, `_SpanAvailWidth`, `PushID(entity.id().index)`, empty state, and `"(unnamed)"` fallback are all correctly implemented per the spec. |

## Edge cases

| Case | Expected behavior | Verified in |
|---|---|---|
| **Empty World (0 entities)** | Panel shows "No entities" text. No `TreeNodeEx` calls. | Code review of early return path. |
| **World with root entities only (no children)** | All root entities render as leaf nodes (`ImGuiTreeNodeFlags_Leaf`). No expand arrows. | Code review of `child_count() == 0` branch. |
| **Single root entity with deep children (10 levels)** | All levels render recursively. Each level expandable/collapsible, default expanded. | Code review of recursive lambda. |
| **Entity with empty name** | Displayed as "(unnamed)". | Code review of `name.empty()` check. |
| **Entity with very long name (>1000 chars)** | ImGui handles text truncation natively. No special handling. | N/A — ImGui behavior. |
| **Entity with special characters (Unicode, emoji)** | ImGui renders UTF-8 text correctly. | N/A — ImGui behavior. |
| **Two entities with identical names** | `PushID(entity.id().index)` prevents ImGui ID collision. | Code review of `PushID`/`PopID` wrapper. |
| **Single root entity with child that also has children** | All levels expandable. Root → child → grandchild indented correctly. | Code review — recursive lambda handles arbitrary depth. |
| **World changes mid-frame** | Not possible — World not modified during `draw_ui()`. | N/A — spec assumption. |
| **`get_root_entity(i)` returns `Entity{}`** | Guard `entity.id() != EntityId::none()` prevents rendering null entity. | Code review of null-entity guard in root iteration loop. |
| **`Entity` default-constructed (null)** | `.name()`, `.child_count()` on null `Entity` is UB — guard prevents reaching code that dereferences. | Code review. |

## Security impact

None. No new input parsing, file I/O, network access, or sensitive data exposure. The entity tree reads from the Editor's in-memory World which is fully owned by the editor process.

## Data and migration impact

None. No schema changes, no data migrations, no seed data, no data loss risks. The tree reads the World every frame but does not modify it.

## API compatibility impact

- **`EditorPanel` and `EditorMenu`**: Virtual method signatures change from `EngineContext const&` to `EditorContext const&`. This is a **breaking change** for any code outside this sprint that defines `EditorPanel` or `EditorMenu` subclasses. All existing subclasses (5 panels + MenuBar) are updated in this sprint.
- **`Editor::update()` and `Editor::draw_ui()`**: No external API change — public signatures remain `(EngineContext const&)`.
- **`ShortcutRegistry` callbacks**: No change — callbacks remain `(EngineContext const&)`.
- **No new public API**: `EditorContext` is an internal implementation detail (forward-declared in base class headers, defined in `editor_context.h`). It is not part of `Editor`'s public API.

## Documentation impact

- **README**: None.
- **Wiki pages** (to be updated by wiki-agent):
  - `docs/wiki/editor/editor-panels.md` — Update: `EditorPanel` and `EditorMenu` base class signatures change from `EngineContext const&` to `EditorContext const&`. Document new `EditorContext` struct. Note `ScenePanel` now renders entity tree (no longer empty placeholder). Update "v1 foundation" section.
  - `docs/wiki/editor/scene-management.md` — Update: Document `EditorContext` pattern for panels to access editor's `World` via `ctx.editor.world()`. Verify World-access section is consistent.
- **Other specs**: None.

## ADR impact

No new ADR needed. `EditorContext` is an implementation detail (pure aggregate, no ownership, no lifecycle), consistent with existing ADR-027 (direct member variables) and ADR-011 (reference members are always valid). No existing ADR is deprecated or amended.

## Done criteria

The Code Agent must satisfy all of the following:

- [ ] **DC-01**: `src/editor/editor_context.h` created with `struct EditorContext { Editor& editor; EngineContext const& engine; };` in namespace `buddd::editor`, using forward declarations for `Editor` and `EngineContext`.
- [ ] **DC-02**: `src/editor/editor_panel.h` — `EngineContext` forward declaration removed, `EditorContext` forward declaration added, `update()` and `draw_ui()` parameter types changed to `EditorContext const&`.
- [ ] **DC-03**: `src/editor/editor_menu.h` — same changes as DC-02.
- [ ] **DC-04**: `src/editor/editor.cpp` — `#include "editor_context.h"` added; `Editor::update()` constructs `EditorContext{*this, ctx}` and passes to `menu->update()` / `panel->update()`; `Editor::draw_ui()` constructs `EditorContext{*this, ctx}` and passes to `menu->draw_ui()` / `panel->draw_ui()`. Public signatures remain `(EngineContext const&)`.
- [ ] **DC-05**: All 4 placeholder panels (`properties_panel.h`, `console_panel.h`, `project_panel.h`, `assets_panel.h`) have `draw_ui` signature changed to `EditorContext const&`. No additional includes needed.
- [ ] **DC-06**: `src/editor/panels/menu_bar.h` — `#include "editor_context.h"` added; `draw_ui` signature changed to `EditorContext const&`; quit callback call uses `ctx.engine` instead of `ctx`. `on_quit_` type unchanged.
- [ ] **DC-07**: `src/editor/panels/scene_panel.h` — `#include <imgui.h>`, `#include "editor_context.h"`, and `#include "scene/world.h"` added; `draw_ui` takes `EditorContext const&` and renders entity tree using:
  - Empty state check: `if (world.entity_count() == 0) { ImGui::Text("No entities"); return; }`
  - Root entity iteration via `world.root_entity_count()` and `world.get_root_entity()`
  - Recursive child traversal via `entity.child_count()` and `entity.get_child()`
  - `ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen` on all nodes
  - `ImGuiTreeNodeFlags_Leaf` when `child_count() == 0`
  - `PushID(static_cast<int>(entity.id().index))` / `PopID` per entity
  - `name` fallback to `"(unnamed)"` when `name().empty()`
  - Null entity guard: `entity.id() != EntityId::none()` check.
- [ ] **DC-08**: `tests/f02_scene_panel_tests.cpp` created with tests tagged `[editor][scene_panel]` covering:
  - EditorContext struct definition (AC-07).
  - Compilation verification for all panel/menu signatures (AC-08, AC-09, AC-10, AC-11).
- [ ] **DC-09**: `cmake --build --preset debug` succeeds with **zero warnings** from `src/editor/` and `tests/`.
- [ ] **DC-10**: All existing tests pass: `buddd_tests` run with zero failures.
- [ ] **DC-11**: No changes to files under `src/engine/`, `src/cmd/`, `tests/CMakeLists.txt`, or any `CMakeLists.txt` files.

# IMPL-2026-06-EDITOR-FOUNDATION — Editor Foundation

## Source spec

- `.specs/sprint-2026-06/editor-foundation/spec.md`

## Goal

Build on the editor scaffolding to add: a Command pattern system (`Command` base class + `CommandStack` with bounded 128-entry undo/redo history), `EditorMenu`/`EditorPanel` abstraction classes for registering overlays and dockable panels, a `MenuBar` (EditorMenu subclass) with File > Quit, Edit > Undo/Redo, Help > About menus, keyboard shortcuts (Ctrl+Q, Ctrl+Z, Ctrl+Shift+Z, Ctrl+Y) processed via `ShortcutRegistry` (bind/process pattern) gated by `WantCaptureKeyboard`, five concrete `EditorPanel` subclasses (Scene, Properties, Console, Project, Assets) with 100×100 minimum size, docking persistence via `buddd_editor.ini`, an About modal popup showing the engine name and version, and a two-phase update/render lifecycle (`App::update()`, `Editor::update()`, `Editor::draw_ui()` split) with `add_menu()`/`add_panel()` registration in `Editor::setup()`. All command infrastructure is independently testable without ImGui.

## Non-goals

- No functional panel content (no entity list, no property editors, no console log, no file browser, no asset thumbnails).
- No scene viewport rendering inside ImGui.
- No gizmos, entity selection, drag-and-drop, or in-viewport interaction.
- No dynamic plugin/panel discovery system. Panels are registered via `Editor::add_panel()` in `Editor::setup()` — no runtime DLL loading or config-file-based discovery.
- No changes to engine core (`src/engine/`) — the ini filename is set by the editor in its own `setup()`, not in engine code.
- `App` base class change is limited to adding one virtual `update()` method; `run_app()` change is limited to one `app.update(ctx)` call. No other changes to `run_app()`.
- No undo/redo of actual editor operations (transform, delete, etc.) — only the infrastructure and menu/shortcut dispatch.
- No persistent storage for user preferences beyond the docking layout file.
- No `UndoCommand` or `RedoCommand` subclasses — Undo/Redo are direct calls to `CommandStack`.
- No quit confirmation dialog.
- No multi-viewport ImGui support (`ImGuiConfigFlags_ViewportsEnable`).
- No SDL3/OpenGL/GLM headers outside `src/engine/` (ADR-019).

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-027 (Editor Architecture) | Editor class with direct member variables, `buddd::editor` namespace, `buddd_editor` static library. No PIMPL. |
| ADR-019 (Architecture Boundaries) | No SDL3/OpenGL/GLM headers outside `src/engine/`. |
| ADR-026 (Dear ImGui Integration) | ImGui frame lifecycle is automated. `io.IniFilename` is set by editor's setup. Shortcuts check `ImGui::GetIO().WantCaptureKeyboard`. |
| ADR-012 (Navigable Object Graph) | InputSystem access path: `ctx.services.platform().input_system()`. |
| ADR-001 (Result/Error Pattern) | All fallible APIs return `Result<void>`. `[[nodiscard]]` on Result-returning functions. |
| ADR-011 (Ownership/Nullability/NoDiscard) | `[[nodiscard]]` conventions. |
| ADR-014 (CLI App System) | `App::update()` lifecycle method added. Default empty impl ensures existing apps unaffected. |

## Files to inspect

| File | Reason |
|---|---|
| `src/editor/editor.h` | Existing Editor class — must add new members and methods. |
| `src/editor/editor.cpp` | Existing Editor impl — must restructure `draw_ui()`, add `setup()` changes. |
| `src/editor/CMakeLists.txt` | Must add new `.cpp` source files. |
| `src/engine/version.h` | `buddd::engine::version()` returns `std::string_view` — used in About dialog. |
| `src/engine/engine_context.h` | `EngineContext` struct — `request_exit()`, `services` field for `InputSystem` access. |
| `src/engine/error.h` | `Result<T>`, `make_error()`, `Error::Category::InitFailed`. |
| `src/engine/input/key_code.h` | `KeyCode` enum values: `ControlLeft(224)`, `ControlRight(228)`, `ShiftLeft(225)`, `ShiftRight(229)`, `Q(20)`, `Z(29)`, `Y(28)`. |
| `src/engine/input/input_system.h` | `InputSystem` — `is_down()`, `is_pressed()` methods. |
| `src/engine/platform/platform.h` | `Platform::input_system()` accessor. |
| `src/engine/engine_service.h` | `EngineService::platform()` accessor. |
| `src/engine/engine_imgui.h` | `engine_imgui::is_initialized()`. |
| `src/engine/log/log.h` | `BUDDD_LOG_*` macros and `BUDDD_LOG_TAG` pattern. |
| `tests/editor_tests.cpp` | Existing editor test — must add command/stack tests. |
| `.specs/sprint-2026-06/editor-scaffolding/implementation-contract.md` | Reference for format and conventions. |

## Files allowed to change

- `src/editor/command.h` — **create**
- `src/editor/command.cpp` — **create**
- `src/editor/command_stack.h` — **create**
- `src/editor/command_stack.cpp` — **create**
- `src/editor/commands/quit_command.h` — **create**
- `src/editor/shortcut_registry.h` — **create**
- `src/editor/editor_menu.h` — **create** (EditorMenu abstract base class)
- `src/editor/editor_panel.h` — **create** (EditorPanel abstract base class)
- `src/editor/panels/menu_bar.h` — **create** (MenuBar concrete class, header-only inline)
- `src/editor/panels/scene_panel.h` — **create** (ScenePanel, header-only inline)
- `src/editor/panels/properties_panel.h` — **create** (PropertiesPanel, header-only inline)
- `src/editor/panels/console_panel.h` — **create** (ConsolePanel, header-only inline)
- `src/editor/panels/project_panel.h` — **create** (ProjectPanel, header-only inline)
- `src/editor/panels/assets_panel.h` — **create** (AssetsPanel, header-only inline)
- `src/editor/editor.h` — **modify** (add menus_/panels_/shortcuts_ members, add_menu/add_panel methods, remove individual panel method declarations)
- `src/editor/editor.cpp` — **modify** (restructure to use ShortcutRegistry, menus_/panels_ vectors, 4-phase rendering, register shortcuts/panels in setup())
- `src/editor/CMakeLists.txt` — **modify** (add command.cpp, command_stack.cpp)
- `src/cmd/app.h` — **modify** (add virtual `update()` method with default empty impl)
- `src/cmd/app.cpp` — **modify** (add `app.update(ctx)` call in render loop)
- `src/cmd/apps/editor_app.h` — **modify** (add `update()` override declaration)
- `src/cmd/apps/editor_app.cpp` — **modify** (add `update()` override calling `editor_->update(ctx)`)
- `tests/editor_tests.cpp` — **modify** (add tests for CommandStack, command lifecycle, headless safety)

## Files forbidden to change

- Any file under `src/engine/` — no engine changes for this feature.
- `src/cmd/main.cpp` — No changes needed.
- `src/cmd/CMakeLists.txt` — No changes needed.
- `tests/CMakeLists.txt` — No changes needed (already links `buddd_editor`).
- Any existing `.h`/`.cpp` files not listed in "Files allowed to change".
- Any wiki or ADR files.

## Existing conventions to follow

1. **Include style**: `#include "..."` for project headers (relative to `src/engine/`, `src/editor/`); `<...>` for system/external headers.
2. **Namespace**: `buddd::editor` for all editor code. Use unindented namespace blocks (project style).
3. **`#pragma once`**: All new headers must use `#pragma once` as the include guard.
4. **`[[nodiscard]]`**: All `Result<T>`-returning and boolean-query functions must be marked `[[nodiscard]]`.
5. **Direct member variables**: No PIMPL — direct members for simplicity (per ADR-027).
6. **ImGui include**: Include `<imgui.h>` directly in `.cpp` files that use `ImGui::*` functions. Do NOT include `<imgui.h>` in editor headers.
7. **Logging**: Use `BUDDD_LOG_TAG("Editor")` at top of `editor.cpp`. Use `BUDDD_LOG_INFO` for ini file path, `BUDDD_LOG_DEBUG` for command/menu actions, `BUDDD_LOG_TRACE` for shortcut suppression.
8. **Error construction**: Use `make_error(Error::Category::..., "message")` for returning errors.
9. **CMake**: New `.cpp` files must be explicitly listed in `add_library(buddd_editor STATIC ...)`. No glob.
10. **Test pattern**: Catch2 `TEST_CASE("name", "[tag]")` with `#include <catch2/catch_test_macros.hpp>`.
11. **Forward declarations**: Prefer forward declarations to includes in headers where possible.
12. **Include order**: Project headers first (alphabetical), then external headers.
13. **String literals for IniFilename**: Use `"buddd_editor.ini"` string literal — has static storage duration, safe for ImGui's pointer lifetime.

## Required implementation behavior

### Step 1: Create `src/editor/command.h`

**File**: `src/editor/command.h` (new)

Declare the `Command` abstract base class in `namespace buddd::editor`:

```cpp
#pragma once

#include <string_view>

namespace buddd::editor {

/// Abstract base for all editor commands.
/// Lifecycle: construct → execute() → (undo() | execute())...
class Command {
public:
    virtual ~Command() = default;

    /// Execute (or re-execute) the command.
    virtual auto execute() -> void = 0;

    /// Undo the command. Only called after execute().
    virtual auto undo() -> void = 0;

    /// Human-readable name for menu display (e.g., "Quit").
    [[nodiscard]] virtual auto name() const -> std::string_view = 0;
};

} // namespace buddd::editor
```

- `name()` returns `std::string_view` — the returned string must remain valid for the command's lifetime (use a `static constexpr` string or a member `std::string`).
- No `#include` of engine headers or ImGui headers.

**Verification**: File compiles with no errors. `#pragma once` is present. The three pure virtual methods are declared.

---

### Step 2: Create `src/editor/command.cpp`

**File**: `src/editor/command.cpp` (new)

```cpp
#include "command.h"

namespace buddd::editor {

// Empty file — vtable emission for Command's destructor is handled by the compiler
// in every translation unit that includes command.h. This file exists for build
// consistency (every .h gets a .cpp) and future non-inline additions.

} // namespace buddd::editor
```

- The file is intentionally empty (all methods are pure virtual or defaulted). It exists so `CMakeLists.txt` can list it and future commands can add non-inline implementations.

**Verification**: File compiles and links.

---

### Step 3: Create `src/editor/command_stack.h`

**File**: `src/editor/command_stack.h` (new)

Declare the `CommandStack` class in `namespace buddd::editor`:

```cpp
#pragma once

#include "command.h"

#include <memory>
#include <string_view>
#include <vector>

namespace buddd::editor {

/// Bounded undo/redo stack for editor commands.
/// Thread-compatible: not thread-safe — used from the main thread only.
class CommandStack {
public:
    /// @param max_history Maximum number of undoable commands to retain.
    ///                    Default 128. Must be clamped to minimum 1.
    explicit CommandStack(size_t max_history = 128);

    /// Execute a command and push it onto the undo stack.
    /// Clears the redo stack (any previously undone commands are discarded).
    auto execute(std::unique_ptr<Command> command) -> void;

    /// Undo the most recent command. Returns false if undo stack is empty.
    [[nodiscard]] auto undo() -> bool;

    /// Redo the most recently undone command. Returns false if redo stack is empty.
    [[nodiscard]] auto redo() -> bool;

    /// Returns true if there is at least one command to undo.
    [[nodiscard]] auto can_undo() const -> bool;

    /// Returns true if there is at least one command to redo.
    [[nodiscard]] auto can_redo() const -> bool;

    /// Name of the command at the top of the undo stack (empty view if empty).
    [[nodiscard]] auto undo_name() const -> std::string_view;

    /// Name of the command at the top of the redo stack (empty view if empty).
    [[nodiscard]] auto redo_name() const -> std::string_view;

    /// Clear all stacks.
    auto clear() -> void;

private:
    std::vector<std::unique_ptr<Command>> undo_stack_;
    std::vector<std::unique_ptr<Command>> redo_stack_;
    size_t max_history_;
};

} // namespace buddd::editor
```

- Include `"command.h"` for the `Command` base class.
- Include `<memory>` for `std::unique_ptr`, `<string_view>` for return types, `<vector>` for storage.
- No engine or ImGui includes.

**Verification**: File compiles with no errors. All 9 methods declared.

---

### Step 4: Create `src/editor/command_stack.cpp`

**File**: `src/editor/command_stack.cpp` (new)

Implement the `CommandStack` class:

```cpp
#include "command_stack.h"

#include <algorithm>  // for std::min

namespace buddd::editor {

CommandStack::CommandStack(size_t max_history)
    : max_history_(std::max<size_t>(max_history, 1))  // clamp to minimum 1
{
}

auto CommandStack::execute(std::unique_ptr<Command> command) -> void {
    command->execute();
    undo_stack_.push_back(std::move(command));
    redo_stack_.clear();

    // Enforce max_history bound: if undo stack exceeds limit, drop oldest command
    if (undo_stack_.size() > max_history_) {
        undo_stack_.erase(undo_stack_.begin());
    }
}

auto CommandStack::undo() -> bool {
    if (undo_stack_.empty()) {
        return false;
    }
    auto command = std::move(undo_stack_.back());
    undo_stack_.pop_back();
    command->undo();
    redo_stack_.push_back(std::move(command));
    return true;
}

auto CommandStack::redo() -> bool {
    if (redo_stack_.empty()) {
        return false;
    }
    auto command = std::move(redo_stack_.back());
    redo_stack_.pop_back();
    command->execute();  // re-execute, not undo
    undo_stack_.push_back(std::move(command));
    return true;
}

auto CommandStack::can_undo() const -> bool {
    return !undo_stack_.empty();
}

auto CommandStack::can_redo() const -> bool {
    return !redo_stack_.empty();
}

auto CommandStack::undo_name() const -> std::string_view {
    if (undo_stack_.empty()) {
        return {};
    }
    return undo_stack_.back()->name();
}

auto CommandStack::redo_name() const -> std::string_view {
    if (redo_stack_.empty()) {
        return {};
    }
    return redo_stack_.back()->name();
}

auto CommandStack::clear() -> void {
    undo_stack_.clear();
    redo_stack_.clear();
}

} // namespace buddd::editor
```

Key behaviors:
- `max_history_` is clamped to minimum 1 in the constructor.
- `execute()` calls `command->execute()`, pushes to undo stack, clears redo stack. If undo stack exceeds `max_history_`, the oldest command is erased.
- `undo()` pops from undo stack, calls `command->undo()`, pushes to redo stack.
- `redo()` pops from redo stack, calls `command->execute()`, pushes to undo stack.
- `undo_name()` / `redo_name()` return `std::string_view` — the view references the command's internal string which lives as long as the command is in the stack.

**Edge cases**:
- Undo on empty stack → returns false, no-op.
- Redo on empty stack → returns false, no-op.
- `max_history = 0` clamped to 1.
- Stack exactly at max_history: execute one more → oldest is dropped (stack stays at max_history).
- `clear()` on empty stacks: no-op.

**Verification**: File compiles. Will be verified by unit tests (Step 13).

---

### Step 5: Create `src/editor/commands/quit_command.h`

**File**: `src/editor/commands/quit_command.h` (new, directory `src/editor/commands/` must be created)

```cpp
#pragma once

#include "command.h"

#include <string_view>

namespace buddd::editor {

/// Command that requests the engine to exit.
/// execute() calls ctx->request_exit(). undo() is a no-op.
class QuitCommand final : public Command {
public:
    explicit QuitCommand(buddd::engine::EngineContext const& ctx)
        : ctx_(&ctx) {}

    auto execute() -> void override {
        ctx_->request_exit();
    }

    auto undo() -> void override {
        // No-op: cannot un-request exit
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Quit";
    }

private:
    buddd::engine::EngineContext const* ctx_;
};

} // namespace buddd::editor
```

- Forward-declare `buddd::engine::EngineContext` at the top of the file (include `<engine_context.h>` from `editor.cpp`, not from this header — but since this is a header-only inline command, it needs the full definition. Use `#include "engine_context.h"`).
- Actually: since the inline implementation calls `ctx_->request_exit()`, the full definition is needed. Include `"engine_context.h"`.
- `undo()` is a no-op — this is acceptable per spec (Q-05 confirmed no quit confirmation).
- `name()` returns a literal string view (static storage duration).
- `[[nodiscard]]` on `name()` (inherited from base).

**Verification**: File compiles. No SDL3/OpenGL/GLM includes.

---

### Step 6: Create `src/editor/shortcut_registry.h`

**File**: `src/editor/shortcut_registry.h` (new)

```cpp
#pragma once

#include "input/key_code.h"
#include "input/input_system.h"

#include <functional>
#include <vector>

namespace buddd::editor {

/// Binds keyboard shortcuts to callback actions.
/// Processed via process() each frame using the engine InputSystem.
class ShortcutRegistry {
public:
    struct Modifiers {
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
    };

    /// Register a shortcut. `action` is called when the key+modifiers combo is pressed.
    auto bind(buddd::engine::KeyCode key, Modifiers mods, std::function<void()> action) -> void;

    /// Process all bindings using the engine InputSystem.
    /// `want_capture` gates processing — no action fires when true.
    auto process(buddd::engine::InputSystem const& input, bool want_capture) -> void;

private:
    struct Binding {
        buddd::engine::KeyCode key;
        Modifiers mods;
        std::function<void()> action;
    };
    std::vector<Binding> bindings_;
};

} // namespace buddd::editor
```

- `bind()` stores the key+modifier+action triple.
- `process()` iterates bindings. If `want_capture` is true, returns immediately (no action fires).
- For each binding: check modifiers with `is_down()` (both left and right variants), then check the action key with `is_pressed()` (edge-triggered, single frame).
- Modifier checking: `input.is_down(KeyCode::ControlLeft) || input.is_down(KeyCode::ControlRight)` for Ctrl; same pattern for Shift (left/right).
- Uses `is_pressed()` (not `is_down()`) for the action key to fire once per press.
- No engine includes beyond `key_code.h` and `input_system.h` (which are needed for the parameter types).

**Implementation outline** (in header, since it's small and header-only):

```cpp
inline auto ShortcutRegistry::bind(buddd::engine::KeyCode key, Modifiers mods, std::function<void()> action) -> void {
    bindings_.push_back({key, mods, std::move(action)});
}

inline auto ShortcutRegistry::process(buddd::engine::InputSystem const& input, bool want_capture) -> void {
    if (want_capture) {
        return;
    }

    for (auto const& b : bindings_) {
        // Check modifiers (both left/right variants)
        bool ctrl_down = input.is_down(buddd::engine::KeyCode::ControlLeft)
                      || input.is_down(buddd::engine::KeyCode::ControlRight);
        bool shift_down = input.is_down(buddd::engine::KeyCode::ShiftLeft)
                       || input.is_down(buddd::engine::KeyCode::ShiftRight);
        bool alt_down = input.is_down(buddd::engine::KeyCode::AltLeft)
                     || input.is_down(buddd::engine::KeyCode::AltRight);

        if (b.mods.ctrl != ctrl_down) continue;
        if (b.mods.shift != shift_down) continue;
        if (b.mods.alt != alt_down) continue;

        // Action key: edge-triggered
        if (input.is_pressed(b.key)) {
            b.action();
        }
    }
}
```

- `process()` checks `WantCaptureKeyboard` first; if true, return without processing any binding.
- Both left and right modifier keys are checked (per spec A-09).
- Use `is_pressed()` (not `is_down()`) for the action key to fire once per press.
- Use `is_down()` for modifiers (must be held).
- The implementation is inline in the header — no `.cpp` file needed.

**Verification**: File compiles with no errors. Both `bind()` and `process()` methods declared.

---

### Step 7: Create `src/editor/editor_menu.h`

**File**: `src/editor/editor_menu.h` (new)

Declare the `EditorMenu` abstract base class in `namespace buddd::editor`:

```cpp
#pragma once

#include <string_view>

namespace buddd::editor {

/// Base class for editor overlay elements rendered before the dockspace.
class EditorMenu {
public:
    virtual ~EditorMenu() = default;

    /// Unique identifier (e.g., "menu_bar").
    [[nodiscard]] virtual auto id() const -> std::string_view = 0;

    /// Per-frame logic. Called every frame from Editor::update().
    virtual auto update(buddd::engine::EngineContext const& /*ctx*/) -> void {}

    /// Per-frame UI rendering. Called every frame from Editor::draw_ui()
    /// before ImGui::DockSpaceOverlay().
    virtual auto draw_ui(buddd::engine::EngineContext const& /*ctx*/) -> void {}
};

} // namespace buddd::editor
```

- `id()` returns a unique string identifier (e.g., `"menu_bar"`).
- `update()` and `draw_ui()` have default empty implementations — subclasses override as needed.
- No `#include` of ImGui headers in the header file.
- Use forward declaration of `buddd::engine::EngineContext` to avoid engine include.

**Verification**: File compiles with no errors. `#pragma once` is present. Three virtual methods declared.

---

### Step 8: Create `src/editor/editor_panel.h`

**File**: `src/editor/editor_panel.h` (new)

Declare the `EditorPanel` abstract base class in `namespace buddd::editor`:

```cpp
#pragma once

#include <string_view>

namespace buddd::editor {

/// Base class for dockable editor panels.
class EditorPanel {
public:
    virtual ~EditorPanel() = default;

    /// Unique identifier (e.g., "scene", "properties").
    [[nodiscard]] virtual auto id() const -> std::string_view = 0;

    /// ImGui window title displayed in the panel title bar.
    [[nodiscard]] virtual auto title() const -> std::string_view = 0;

    /// Per-frame logic. Called every frame from Editor::update().
    virtual auto update(buddd::engine::EngineContext const& /*ctx*/) -> void {}

    /// Per-frame UI rendering. Called every frame from Editor::draw_ui()
    /// inside the dockspace, between ImGui::Begin(title) and ImGui::End().
    virtual auto draw_ui(buddd::engine::EngineContext const& /*ctx*/) -> void {}
};

} // namespace buddd::editor
```

- `id()` returns a unique identifier (e.g., `"scene"`).
- `title()` returns the ImGui window title string (e.g., `"Scene"`).
- `update()` and `draw_ui()` have default empty implementations — subclasses override as needed.
- No `#include` of ImGui headers in the header file.

**Verification**: File compiles with no errors. `#pragma once` is present. Four virtual methods declared.

---

### Step 9: Create `src/editor/panels/menu_bar.h`

**File**: `src/editor/panels/menu_bar.h` (new, directory `src/editor/panels/` must be created)

Declare the `MenuBar` concrete class extending `EditorMenu`:

```cpp
#pragma once

#include "editor_menu.h"
#include "command_stack.h"
#include "commands/quit_command.h"

#include <functional>
#include <imgui.h>
#include <string_view>

namespace buddd::editor {

/// Main menu bar rendered via ImGui::BeginMainMenuBar().
/// Displays File > Quit, Edit > Undo/Redo, Help > About menus.
/// About action is dispatched via a callback (set_on_about), not a command.
class MenuBar final : public EditorMenu {
public:
    explicit MenuBar(CommandStack& command_stack)
        : command_stack_(command_stack)
    {}

    /// Set the callback invoked when Help > About is clicked.
    auto set_on_about(std::function<void()> callback) -> void {
        on_about_ = std::move(callback);
    }

    [[nodiscard]] auto id() const -> std::string_view override {
        return "menu_bar";
    }

    auto draw_ui(buddd::engine::EngineContext const& ctx) -> void override {
        if (ImGui::BeginMainMenuBar()) {
            // ── File menu ──
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
                    command_stack_.execute(std::make_unique<QuitCommand>(ctx));
                }
                ImGui::EndMenu();
            }

            // ── Edit menu ──
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, command_stack_.can_undo())) {
                    command_stack_.undo();
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z", false, command_stack_.can_redo())) {
                    command_stack_.redo();
                }
                ImGui::EndMenu();
            }

            // ── Help menu ──
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
                    if (on_about_) {
                        on_about_();
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

private:
    CommandStack& command_stack_;
    std::function<void()> on_about_;
};

} // namespace buddd::editor
```

- Constructor takes only `CommandStack&` — no `bool& show_about_` parameter.
- `set_on_about(std::function<void()>)` sets the callback for the About action, invoked when Help > About is clicked.
- `id()` returns `"menu_bar"`.
- `draw_ui()` is inline and header-only, matching the pattern used by concrete panel classes.
- Menu item labels are just "Undo" / "Redo" (no command names, per Q-06).
- Disabled state is driven by `command_stack_.can_undo()` / `command_stack_.can_redo()`.
- Quit goes through `command_stack_.execute()` with `QuitCommand`.
- Undo/Redo call `command_stack_.undo()` / `command_stack_.redo()` directly.
- About invokes the `on_about_` callback (set by Editor in `setup()`).
- Includes `<imgui.h>` directly for ImGui calls.
- No `ShowAboutCommand` include — About is handled via callback, not a command.
- Note: `menu_bar.h` will need `#include "log/log.h"` for `BUDDD_LOG_DEBUG` — but the logging for About open is handled in Editor, not MenuBar. Logging for Undo/Redo is handled in the shortcuts path via `Editor::update()`, not in MenuBar. If the implementer wants logging in MenuBar, add `#include "log/log.h"` and log at BUDDD_LOG_DEBUG. The spec is silent on this — the implementer can choose.

**Verification**: File compiles with no errors.

---

### Step 10: Create concrete panel classes (five files)

**Files** (all new, all header-only inline):

| File | Class | Id | Title |
|---|---|---|---|
| `src/editor/panels/scene_panel.h` | `ScenePanel` | `"scene"` | `"Scene"` |
| `src/editor/panels/properties_panel.h` | `PropertiesPanel` | `"properties"` | `"Properties"` |
| `src/editor/panels/console_panel.h` | `ConsolePanel` | `"console"` | `"Console"` |
| `src/editor/panels/project_panel.h` | `ProjectPanel` | `"project"` | `"Project"` |
| `src/editor/panels/assets_panel.h` | `AssetsPanel` | `"assets"` | `"Assets"` |

Each file follows the same pattern (example using `ScenePanel`):

```cpp
#pragma once

#include "editor_panel.h"

#include <string_view>

namespace buddd::editor {

class ScenePanel final : public EditorPanel {
public:
    [[nodiscard]] auto id() const -> std::string_view override { return "scene"; }
    [[nodiscard]] auto title() const -> std::string_view override { return "Scene"; }

    auto draw_ui(buddd::engine::EngineContext const& /*ctx*/) -> void override {
        ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin("Scene");
        // (empty — placeholder for future content)
        ImGui::End();
    }
};

} // namespace buddd::editor
```

Key points:
- Each panel sets a 100×100 minimum size constraint via `ImGui::SetNextWindowSizeConstraints()`.
- No `p_open` parameter — ImGui manages close state internally; panels always re-open on next launch.
- `update()` is not overridden (uses default no-op from base).
- No functional content inside any panel.
- Each file includes `<imgui.h>` for the ImGui calls (in the `.h` file, since these are header-only inline panels — acceptable per spec as they are implementation headers).

**Verification**: All five files compile with no errors. No SDL3/OpenGL/GLM includes.

---

### Step 11: Update `src/editor/editor.h`

**File**: `src/editor/editor.h` (modified)

Add includes and new members after existing includes. Remove individual panel drawing methods — replaced by `menus_`/`panels_` vectors.

```cpp
#pragma once

#include "error.h"

#include "command_stack.h"      // CommandStack member
#include "shortcut_registry.h"  // ShortcutRegistry member
#include "editor_menu.h"        // EditorMenu base
#include "editor_panel.h"       // EditorPanel base

#include <memory>               // std::unique_ptr
#include <vector>               // std::vector

namespace buddd::engine { struct EngineContext; class EngineService; class Window; }

namespace buddd::editor {

class Editor {
public:
    Editor();
    ~Editor();

    [[nodiscard]] auto setup(buddd::engine::EngineContext const& ctx)
        -> buddd::engine::Result<void>;

    /// Process editor logic: keyboard shortcuts, state updates.
    /// Called every frame from EditorApp::update(), after world->update_updatables().
    auto update(buddd::engine::EngineContext const& ctx) -> void;

    /// Register a menu overlay (takes ownership).
    auto add_menu(std::unique_ptr<EditorMenu> menu) -> void;

    /// Register a dockable panel (takes ownership).
    auto add_panel(std::unique_ptr<EditorPanel> panel) -> void;

    auto draw_ui(buddd::engine::EngineContext const& ctx) -> void;

    auto shutdown() -> void;

private:
    // ── About popup ──
    auto draw_about_popup(buddd::engine::EngineContext const& ctx) -> void;

    // ── State ──
    bool initialized_ = false;
    buddd::engine::EngineService* engine_ = nullptr;
    buddd::engine::Window* window_ = nullptr;

    // Command system
    CommandStack command_stack_;

    // Shortcut registry
    ShortcutRegistry shortcuts_;

    // Registered overlays (drawn before dockspace via menus_ iteration in draw_ui)
    std::vector<std::unique_ptr<EditorMenu>> menus_;

    // Registered panels (drawn inside dockspace via panels_ iteration in draw_ui)
    std::vector<std::unique_ptr<EditorPanel>> panels_;

    // Panel state flags
    bool show_about_ = false;
};

} // namespace buddd::editor
```

- Include `"command_stack.h"`, `"shortcut_registry.h"`, `"editor_menu.h"`, `"editor_panel.h"` for member types.
- Include `<memory>` and `<vector>` for `unique_ptr` and vector storage.
- No ImGui includes in the header.
- `add_menu()` and `add_panel()` are new public methods for registration.
- Individual `draw_scene_panel()`, `draw_properties_panel()`, etc. methods are removed — replaced by `panels_` vector iteration.
- `draw_menu_bar()` and `draw_panels()` helper methods are also removed — the iteration logic lives directly in `draw_ui()`.
- No `process_shortcuts()` private method — replaced by `ShortcutRegistry::process()`.
- `ShortcutRegistry shortcuts_` member replaces the manual `process_shortcuts()` method.

**Verification**: File compiles with no errors. Two new public registration methods, three new member vectors (`menus_`, `panels_`, `shortcuts_`), no individual panel method declarations, no `process_shortcuts()` declaration.

---

### Step 12: Update `src/editor/editor.cpp`

**File**: `src/editor/editor.cpp` (modified)

This is the largest change. Replace the existing implementation with:

**Includes**:
```cpp
#include "editor.h"

#include "commands/quit_command.h"
#include "panels/menu_bar.h"
#include "panels/scene_panel.h"
#include "panels/properties_panel.h"
#include "panels/console_panel.h"
#include "panels/project_panel.h"
#include "panels/assets_panel.h"
#include "engine_context.h"
#include "engine_service.h"
#include "error.h"
#include "imgui/engine_imgui.h"
#include "input/key_code.h"
#include "input/input_system.h"
#include "log/log.h"
#include "platform/platform.h"
#include "version.h"

#include <imgui.h>
```

Note: No `"commands/show_about_command.h"` include — About is handled via callback, not command. The include for `version.h` is needed for `buddd::engine::version()`. The include for `input_system.h` is for `InputSystem`, `key_code.h` for `KeyCode`, `platform/platform.h` for `Platform`.

**Log tag** (add at top of .cpp after includes, before namespace):
```cpp
BUDDD_LOG_TAG("Editor");
```

**Namespace alias** (keep existing):
```cpp
namespace be = buddd::engine;
```

**Constructor/Destructor** (unchanged):
```cpp
Editor::Editor() = default;
Editor::~Editor() {
    shutdown();
}
```

**`Editor::setup()`** — add `IniFilename`, register shortcuts/panels/MenuBar with callback:

```cpp
auto Editor::setup(be::EngineContext const& ctx) -> be::Result<void> {
    engine_ = &ctx.services;
    window_ = &ctx.window;
    initialized_ = true;

    if (!be::engine_imgui::is_initialized()) {
        return make_error(be::Error::Category::InitFailed,
            "ImGui is not initialized. The editor requires a display with working ImGui.");
    }

    // Enable docking layout persistence
    ImGui::GetIO().IniFilename = "buddd_editor.ini";
    BUDDD_LOG_INFO("Editor: layout file: buddd_editor.ini");

    // ── Create menu bar ──
    auto menu_bar = std::make_unique<MenuBar>(command_stack_);
    menu_bar->set_on_about([this]() {
        show_about_ = true;
    });
    add_menu(std::move(menu_bar));

    // ── Register panels ──
    add_panel(std::make_unique<ScenePanel>());
    add_panel(std::make_unique<PropertiesPanel>());
    add_panel(std::make_unique<ConsolePanel>());
    add_panel(std::make_unique<ProjectPanel>());
    add_panel(std::make_unique<AssetsPanel>());

    // ── Register shortcuts ──
    auto& input = ctx.services.platform().input_system();
    (void)input; // input used by bind callbacks at runtime via shortcuts_.process()

    shortcuts_.bind(be::KeyCode::Q, {.ctrl = true}, [this, &ctx]() {
        command_stack_.execute(std::make_unique<QuitCommand>(ctx));
    });
    shortcuts_.bind(be::KeyCode::Z, {.ctrl = true}, [this]() {
        command_stack_.undo();
    });
    shortcuts_.bind(be::KeyCode::Z, {.ctrl = true, .shift = true}, [this]() {
        command_stack_.redo();
    });
    shortcuts_.bind(be::KeyCode::Y, {.ctrl = true}, [this]() {
        command_stack_.redo();
    });

    return {};
}
```

- `ImGui::GetIO().IniFilename = "buddd_editor.ini";` — string literal has static storage duration, pointer remains valid for ImGui's lifetime.
- Log the ini file path at Info level.
- MenuBar created with `CommandStack&` only, then callback set via `set_on_about()`.
- Shortcuts registered via `shortcuts_.bind()` — no separate `process_shortcuts()` method needed.
- `QuitCommand` captures `ctx` by reference (valid for editor lifetime).

**`Editor::update()`** — process keyboard shortcuts via ShortcutRegistry (logic only, no ImGui rendering):

```cpp
auto Editor::update(be::EngineContext const& ctx) -> void {
    if (!initialized_) {
        return;
    }

    // ═══════════════════════════════════════════════
    // Keyboard shortcuts (gated by WantCaptureKeyboard)
    // ═══════════════════════════════════════════════
    auto& input = ctx.services.platform().input_system();
    shortcuts_.process(input, ImGui::GetIO().WantCaptureKeyboard);

    // ═══════════════════════════════════════════════
    // Delegate to registered menus and panels
    // ═══════════════════════════════════════════════
    for (auto& menu : menus_) {
        menu->update(ctx);
    }
    for (auto& panel : panels_) {
        panel->update(ctx);
    }
}
```

**`Editor::draw_ui()`** — 4-phase rendering according to the spec:

```
Phase 1 — Overlays (before dockspace): menus_ → menu->draw_ui(ctx)
Phase 2 — Dockspace: DockSpaceOverViewport + default layout
Phase 3 — Panels (inside dockspace): panels_ → Begin/End around panel->draw_ui(ctx)
Phase 4 — About popup (if show_about_)
```

```cpp
auto Editor::draw_ui(be::EngineContext const& ctx) -> void {
    if (!initialized_) {
        return;
    }

    // ═══════════════════════════════════════════════
    // Phase 1: Overlays (menus) — drawn before dockspace
    // ═══════════════════════════════════════════════
    for (auto& menu : menus_) {
        menu->draw_ui(ctx);
    }

    // ═══════════════════════════════════════════════
    // Phase 2: Dockspace + default layout
    // ═══════════════════════════════════════════════
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

    static bool first_layout = true;
    if (first_layout) {
        first_layout = false;

        // Check if a saved layout already exists (ini loaded by ImGui on NewFrame)
        ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspace_id);
        // If the node has no children, no layout was loaded → create default
        if (node && node->ChildNodes[0] == nullptr && node->ChildNodes[1] == nullptr) {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

            // Split: right 25% for Properties
            ImGuiID dock_right;
            ImGuiID dock_main = dockspace_id;
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.25f, &dock_right, &dock_main);

            // Split bottom 25% for Console (under the center+right)
            ImGuiID dock_bottom;
            ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.25f, &dock_bottom, &dock_main);

            // Split bottom area: left 50% for Project, right 50% for Assets
            ImGuiID dock_bottom_left;
            ImGuiID dock_bottom_right;
            ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.5f, &dock_bottom_left, &dock_bottom_right);

            // Dock windows to the splits
            ImGui::DockBuilderDockWindow("Scene", dock_main);
            ImGui::DockBuilderDockWindow("Properties", dock_right);
            ImGui::DockBuilderDockWindow("Console", dock_bottom);
            ImGui::DockBuilderDockWindow("Project", dock_bottom_left);
            ImGui::DockBuilderDockWindow("Assets", dock_bottom_right);

            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    // ═══════════════════════════════════════════════
    // Phase 3: Dockable panels (inside dockspace)
    // ═══════════════════════════════════════════════
    for (auto& panel : panels_) {
        ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin(panel->title().data());
        panel->draw_ui(ctx);
        ImGui::End();
    }

    // ═══════════════════════════════════════════════
    // Phase 4: About popup (rendered every frame if show_about_ is true)
    // ═══════════════════════════════════════════════
    draw_about_popup(ctx);
}
```

**Menu bar** — The menu bar is now implemented by the `MenuBar` class (see Step 9). `MenuBar::draw_ui()` uses `ImGui::BeginMainMenuBar()` and renders File > Quit, Edit > Undo/Redo, Help > About with the same command dispatch logic. The menu bar is invoked in Phase 1 of `Editor::draw_ui()` via `menus_` iteration. The About action invokes the `on_about_` callback (set in `setup()`), which sets `show_about_ = true`.

**Shortcut processing** — No separate `process_shortcuts()` method. Shortcuts are registered via `shortcuts_.bind()` in `setup()` and processed via `shortcuts_.process()` in `update()`.

**Panels** — Individual panel drawing methods (`draw_scene_panel()`, etc.) are removed. Each concrete panel is now an `EditorPanel` subclass (see Step 10). In Phase 3 of `Editor::draw_ui()`, the `panels_` vector is iterated, calling `ImGui::Begin(panel->title().data())`, `panel->draw_ui(ctx)`, and `ImGui::End()` for each panel. The 100×100 minimum size constraint is set per-panel via `ImGui::SetNextWindowSizeConstraints()` in each panel's `draw_ui()`.

**`Editor::draw_about_popup()`**:

```cpp
auto Editor::draw_about_popup(be::EngineContext const& /*ctx*/) -> void {
    if (!show_about_) {
        return;
    }

    ImGui::OpenPopup("About Buddd Editor");

    // Modal popup
    if (ImGui::BeginPopupModal("About Buddd Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Buddd Engine v%s", be::version().data());

        ImGui::Separator();

        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
            show_about_ = false;
        }

        ImGui::EndPopup();
    } else {
        // Popup was dismissed by Escape or click-outside
        show_about_ = false;
    }
}
```

- `ImGui::OpenPopup()` is called every frame while `show_about_` is true. ImGui makes this idempotent if the popup is already open.
- The popup is modal — blocks interaction with other windows until dismissed.
- "Close" button and clicking outside the popup dismiss it.
- `be::version()` returns `std::string_view` — `.data()` gives the C string for `ImGui::Text`.
- When dismissed (Close button, Escape, or clicking outside), `show_about_` is reset to false via the else branch of `BeginPopupModal`.

**`Editor::shutdown()`** — unchanged:

```cpp
auto Editor::shutdown() -> void {
    initialized_ = false;
    engine_ = nullptr;
    window_ = nullptr;
}
```

**Verification**: File compiles. All new methods are defined. No ImGui includes in the header. No `process_shortcuts()` method defined. No `show_about_command.h` include.

---

### Step 13: Update `src/editor/CMakeLists.txt`

**File**: `src/editor/CMakeLists.txt` (modified)

Add new source files:

```cmake
add_library(buddd_editor STATIC
    editor.cpp
    command.cpp
    command_stack.cpp
)

target_link_libraries(buddd_editor
    PUBLIC
        buddd_engine
)

target_include_directories(buddd_editor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
```

- `command.cpp` and `command_stack.cpp` are explicitly listed.
- The `commands/`, `panels/`, and `shortcut_registry.h` are header-only (`.h` files) — no `.cpp` files needed since all concrete commands, panels, MenuBar, and ShortcutRegistry are header-only inline.
- No changes to `target_link_libraries` or `target_include_directories`.

**Verification**: Build succeeds: `cmake --build --preset debug`.

---

### Step 14: Add tests to `tests/editor_tests.cpp`

**File**: `tests/editor_tests.cpp` (modified)

Add new `TEST_CASE` entries for the command system. The existing `[editor]` tagged test for Editor lifecycle is kept unchanged.

**Test: CommandStack — execute, undo, redo cycle**

```cpp
#include "command.h"
#include "command_stack.h"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string_view>

namespace be = buddd::engine;
namespace ed = buddd::editor;

// ── Helper: a test command that toggles a bool ──
class ToggleCommand final : public ed::Command {
public:
    explicit ToggleCommand(bool* target, std::string_view name)
        : target_(target), name_(name) {}

    auto execute() -> void override { *target_ = true;  }
    auto undo()    -> void override { *target_ = false; }
    [[nodiscard]] auto name() const -> std::string_view override { return name_; }

private:
    bool* target_;
    std::string_view name_;
};
```

Then add test cases:

```cpp
TEST_CASE("CommandStack: fresh stack has no undo/redo", "[editor][command]") {
    ed::CommandStack stack;
    REQUIRE_FALSE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());
    REQUIRE(stack.undo_name().empty());
    REQUIRE(stack.redo_name().empty());
    REQUIRE_FALSE(stack.undo());
    REQUIRE_FALSE(stack.redo());
}

TEST_CASE("CommandStack: execute pushes to undo stack", "[editor][command]") {
    ed::CommandStack stack;
    bool flag = false;
    stack.execute(std::make_unique<ToggleCommand>(&flag, "Toggle"));
    REQUIRE(flag == true);               // execute() was called
    REQUIRE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());     // redo cleared
    REQUIRE(stack.undo_name() == "Toggle");
}

TEST_CASE("CommandStack: undo then redo cycle", "[editor][command]") {
    ed::CommandStack stack;
    bool flag = false;
    stack.execute(std::make_unique<ToggleCommand>(&flag, "Toggle"));

    // Undo
    flag = false;  // reset for verification
    REQUIRE(stack.undo());
    REQUIRE_FALSE(flag);                  // undo() was called
    REQUIRE_FALSE(stack.can_undo());
    REQUIRE(stack.can_redo());
    REQUIRE(stack.redo_name() == "Toggle");

    // Redo
    REQUIRE(stack.redo());
    REQUIRE(flag);                        // execute() called again
    REQUIRE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());
}

TEST_CASE("CommandStack: new command clears redo stack", "[editor][command]") {
    ed::CommandStack stack;
    bool flag1 = false, flag2 = false;

    stack.execute(std::make_unique<ToggleCommand>(&flag1, "C1"));
    stack.undo();                              // redo stack has C1
    REQUIRE(stack.can_redo());

    stack.execute(std::make_unique<ToggleCommand>(&flag2, "C2"));
    REQUIRE_FALSE(stack.can_redo());           // redo cleared
    REQUIRE(stack.can_undo());
    REQUIRE(stack.undo_name() == "C2");        // C2 is now on top
}

TEST_CASE("CommandStack: max_history bound enforced", "[editor][command]") {
    ed::CommandStack stack(2);  // max 2
    bool f1 = false, f2 = false, f3 = false;

    stack.execute(std::make_unique<ToggleCommand>(&f1, "C1"));
    stack.execute(std::make_unique<ToggleCommand>(&f2, "C2"));
    stack.execute(std::make_unique<ToggleCommand>(&f3, "C3"));

    // Stack has 2 entries: C2 (bottom) and C3 (top)
    REQUIRE(stack.can_undo());
    stack.undo();
    REQUIRE(stack.undo_name() == "C2");  // C2 should be on top after undoing C3
    stack.undo();
    REQUIRE_FALSE(stack.can_undo());     // Both undone
}

TEST_CASE("CommandStack: clear empties both stacks", "[editor][command]") {
    ed::CommandStack stack;
    bool flag = false;

    stack.execute(std::make_unique<ToggleCommand>(&flag, "C1"));
    stack.undo();                        // Now both stacks have entries
    REQUIRE(stack.can_undo() == false);  // Wait, after undo, undo is empty, redo has C1
    // Actually: execute → undo pushes to undo, then undo pops from undo, pushes to redo
    // So after execute + undo: undo is empty, redo has C1
    // Let's re-do to test clear
    REQUIRE(stack.can_redo());
    stack.clear();
    REQUIRE_FALSE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());
}

TEST_CASE("CommandStack: max_history clamped to minimum 1", "[editor][command]") {
    ed::CommandStack stack(0);  // Should clamp to 1
    bool f1 = false, f2 = false;

    stack.execute(std::make_unique<ToggleCommand>(&f1, "C1"));
    stack.execute(std::make_unique<ToggleCommand>(&f2, "C2"));

    // With max_history=1, only C2 should remain
    REQUIRE(stack.can_undo());
    stack.undo();
    // After undoing C2, stack should be empty (C1 was dropped)
    REQUIRE_FALSE(stack.can_undo());
}

TEST_CASE("CommandStack: undo_name returns empty on empty stack", "[editor][command]") {
    ed::CommandStack stack;
    REQUIRE(stack.undo_name().empty());
    REQUIRE(stack.redo_name().empty());
}

TEST_CASE("CommandStack: redo_name returns command name after undo", "[editor][command]") {
    ed::CommandStack stack;
    bool flag = false;
    stack.execute(std::make_unique<ToggleCommand>(&flag, "MyCommand"));
    stack.undo();
    REQUIRE(stack.redo_name() == "MyCommand");
}
```

**AC-032: Headless Editor lifecycle test** — keep the existing test (it already verifies crash-free setup/shutdown). No changes needed to it.

**Important**: The existing test file may need additional includes. Ensure the existing `#include` list is preserved and add `"command_stack.h"` and the test helper `ToggleCommand`.

**Verification**: Tests compile and pass in both display and headless modes. Use `ctest --preset debug` (or build and run `buddd_tests`).

---

### Step 15: Build and verify

**Verification checklist**:
1. `cmake --build --preset debug` succeeds with no warnings.
2. `buddd_tests` passes all command stack tests.
3. No SDL3/OpenGL/GLM headers leak: `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/` → zero matches.
4. `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/commands/` → zero matches.
5. `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/panels/` → zero matches.
6. `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/shortcut_registry.h` → zero matches.
7. Manual: Run `buddd edit`, verify menu bar, panels, shortcuts, about popup, ini file creation.
8. `grep -rn 'virtual auto update.*EngineContext' src/cmd/app.h` → one match with default `{}`.
9. `grep -rn 'app\.update(ctx)' src/cmd/app.cpp` → one match after `update_updatables` and before `render_scene`.
10. `buddd run triangle` renders correctly (verifies no regression from App::update() default impl).

---

### Step 16: Add `App::update()` to base class

**File**: `src/cmd/app.h` (modified)

Add a virtual `update()` method to `class App` after the existing lifecycle methods:

```cpp
    /// Called once per frame after world->update_updatables(ctx),
    /// between on_frame_begin() and on_render(), before render_scene().
    /// Dedicated to per-frame logic (shortcuts, state updates) separate from rendering.
    /// Default no-op. Override to add per-frame logic.
    virtual auto update(buddd::engine::EngineContext const& ctx) -> void {}
```

**Placement**: Insert this method after `on_frame_begin()` and before `on_render()`. Update the lifecycle comment at the top of the class to include `update()`:

```
/// Lifecycle: config() -> setup() -> on_frame_begin() x N -> update() x N -> on_render() x N -> shutdown().
```

**File**: `src/cmd/app.cpp` (modified)

Add `app.update(ctx)` call in the render loop, after `world->update_updatables(ctx)` and before `render_system->render_scene()`:

```cpp
        // ── Updatable auto-dispatch ──
        world->update_updatables(ctx);
        if (ctx.is_exit_requested()) {
            aborted_by_user = true;
            BUDDD_LOG_INFO("Scene aborted by user (frame {})", frame + 1);
            eng.device().end_frame();
            break;
        }

        // ── App per-frame logic (shortcuts, state updates) ──
        app.update(ctx);
        if (ctx.is_exit_requested()) {
            aborted_by_user = true;
            BUDDD_LOG_INFO("Scene aborted by user (frame {})", frame + 1);
            eng.device().end_frame();
            break;
        }

        // ── Automatic scene render ──
        render_system->render_scene();
```

**Exit check pattern**: The `app.update(ctx)` call is followed by an exit check identical to the pattern used after `update_updatables()` and `on_frame_begin()`. If a command dispatched during `update()` calls `ctx.request_exit()`, the frame is terminated early.

**Verification**: File compiles. `buddd run triangle` renders correctly (all 14 existing demo scenes unaffected by default empty impl).

---

### Step 17: Update `EditorApp` to override `update()`

**File**: `src/cmd/apps/editor_app.h` (modified)

Add `update()` override declaration:

```cpp
    auto on_render(buddd::engine::EngineContext const& ctx) -> void override;

    auto update(buddd::engine::EngineContext const& ctx) -> void override;

    auto shutdown() -> void override;
```

**File**: `src/cmd/apps/editor_app.cpp` (modified)

Add `update()` implementation that calls `editor_->update(ctx)`:

```cpp
auto buddd::cmd::app::EditorApp::update(be::EngineContext const& ctx) -> void {
    if (editor_) {
        editor_->update(ctx);
    }
}
```

Modify `on_render()` to call `editor_->draw_ui(ctx)` (keep existing — it already does this):

```cpp
auto buddd::cmd::app::EditorApp::on_render(be::EngineContext const& ctx) -> void {
    if (editor_) {
        editor_->draw_ui(ctx);
    }
}
```

The two-phase split ensures:
- `EditorApp::update()` → `editor_->update(ctx)` → `ShortcutRegistry::process()` + menu/panel update — logic phase
- `EditorApp::on_render()` → `editor_->draw_ui(ctx)` → menu bar, panels, popups — render phase

**Verification**: File compiles. `buddd edit` runs and shortcuts work (Ctrl+Q exits, Ctrl+Z undoes, etc.).

---

## Required tests

### Unit tests

| Test | File | What it verifies | Spec AC |
|---|---|---|---|
| Fresh stack state | `tests/editor_tests.cpp` | `can_undo()==false`, `can_redo()==false`, `undo()`/`redo()` return false, `undo_name()`/`redo_name()` empty | AC-004, AC-005 |
| Execute + undo + redo cycle | `tests/editor_tests.cpp` | `execute()` calls command and pushes to undo; `undo()` pops, calls undo, pushes to redo; `redo()` pops, calls execute, pushes to undo | AC-003, AC-006, AC-007 |
| New command clears redo | `tests/editor_tests.cpp` | After undo + new execute, `can_redo()==false` | AC-008 |
| Max_history bound | `tests/editor_tests.cpp` | Stack with max=2, execute 3 → oldest dropped. Undoing shows only latest 2 | AC-009 |
| Clear empties both stacks | `tests/editor_tests.cpp` | Execute + undo → clear → both stacks empty | AC-010 |
| Max_history clamped to 1 | `tests/editor_tests.cpp` | Constructor with 0 clamps to 1 | Edge case |
| Undo_name/redo_name on empty | `tests/editor_tests.cpp` | Empty stack returns empty string_view | Edge case |
| Undo_name returns command name | `tests/editor_tests.cpp` | After undo, `redo_name()` returns command's name | AC-006 |
| Editor headless lifecycle | `tests/editor_tests.cpp` | Construct, setup (may fail), shutdown, no crash | AC-032, AC-033 |
| EditorMenu abstract class exists | `src/editor/editor_menu.h` inspection | Class has virtual `id()`, `update()`, `draw_ui()` methods | AC-011 |
| EditorPanel abstract class exists | `src/editor/editor_panel.h` inspection | Class has virtual `id()`, `title()`, `update()`, `draw_ui()` methods | AC-012 |
| Editor has add_menu/add_panel | `src/editor/editor.h` inspection | Public `add_menu(unique_ptr<EditorMenu>)` and `add_panel(unique_ptr<EditorPanel>)` declared | AC-013 |
| Editor::draw_ui() 4-phase order | `src/editor/editor.cpp` review | Phase 1: menus, Phase 2: dockspace, Phase 3: panels, Phase 4: about | AC-014 |
| Editor::update() delegates to menus/panels | `src/editor/editor.cpp` review | Calls `menu->update(ctx)` and `panel->update(ctx)` for each registered part | AC-015 |
| ShortcutRegistry exists | `src/editor/shortcut_registry.h` inspection | Class with `bind()` and `process()` methods in `namespace buddd::editor` | AC-016 |
| New panel via subclass and registration | Code review | A new `EditorPanel` subclass can be added by registering via `add_panel()` in setup() | AC-039 |
| New menu via subclass and registration | Code review | A new `EditorMenu` subclass can be added by registering via `add_menu()` in setup() | AC-040 |
| New shortcut via bind() | Code review | A new shortcut can be added via `shortcuts_.bind()` in setup() | AC-041 |
| process() respects WantCaptureKeyboard | Code review | `process()` returns early when `want_capture` is true | AC-042 |
| process() uses is_pressed() not is_down() | Code review | `process()` uses `input.is_pressed()` for action key (edge-triggered) | AC-043 |
| App::update() default empty impl | `src/cmd/app.h` inspection | Virtual `update()` method declared with default empty body | AC-034 |
| run_app() calls app.update() | `src/cmd/app.cpp` inspection | `app.update(ctx)` called after `world->update_updatables(ctx)` and before `render_system->render_scene()` | AC-035 |
| EditorApp::update() override | `src/cmd/apps/editor_app.*` inspection | EditorApp overrides `update()` and calls `editor_->update(ctx)` | AC-036 |
| Editor::update() processes shortcuts via ShortcutRegistry | Code review + manual | `Editor::update()` calls `shortcuts_.process()`; shortcuts fire correctly | AC-037 |
| Existing 14 demo scenes unaffected | Build + manual run | `cmake --build --preset debug` succeeds; `buddd run triangle` renders correctly | AC-038 |

### E2E / Integration verification

| Method | What it verifies | Spec AC |
|---|---|---|
| Manual: `buddd edit` | Menu bar with File/Edit/Help visible | (manual sub-check of AC-018/AC-019/AC-022) |
| Manual: File > Quit | Editor exits cleanly | AC-018 |
| Manual: Edit > Undo/Redo disabled state | On fresh launch, Undo and Redo are greyed out | AC-019, AC-020 |
| Manual: Edit > Undo/Redo enabled after command | After Help > About, Undo becomes enabled, Redo disabled | AC-021 |
| Manual: Help > About | Modal popup shows "About Buddd Editor" with engine version | AC-022 |
| Manual: About popup Close button | Opens About, clicks Close, popup dismisses | AC-023 |
| Manual: Ctrl+Q | Editor exits | AC-024 |
| Manual: Ctrl+Z (with non-empty stack) | Undo fires (menu state changes) | AC-025 |
| Manual: Ctrl+Shift+Z / Ctrl+Y | Redo fires | (covered by AC-025) |
| Manual: Shortcuts suppressed during modal | Ctrl+Q while About open does nothing; after close, Ctrl+Q exits | AC-042 |
| Manual: all 14 demo scenes work | `buddd run triangle`, `buddd run cube`, etc. render correctly | AC-038 |
| Manual: 5 panels visible | Scene, Properties, Console, Project, Assets titles visible | AC-018 (manual sub-check) |
| Manual: Panels dockable | Drag and rearrange panels | AC-018 (manual sub-check) |
| Manual: `buddd_editor.ini` created | After closing editor, ini file exists in CWD | AC-026 |
| Manual: Layout restored | Rearrange, close, reopen → layout persists | AC-027 |
| Manual: Default layout on fresh start | Delete ini, launch → default distribution (Scene center, Properties right, Console bottom, Project bottom-left, Assets bottom-right) | AC-028 |
| Build: Architecture boundary | `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/` → zero | AC-029, AC-030 |
| Build: `cmake --build --preset debug` | No compile or link errors | AC-031 |

## Edge cases

| Case | Expected behavior | Where handled |
|---|---|---|
| Undo with empty stack | `undo()` returns false. Edit > Undo disabled. Ctrl+Z no-op. | `CommandStack::undo()` return check, `can_undo()` in menu |
| Redo with empty stack | `redo()` returns false. Edit > Redo disabled. Ctrl+Y / Ctrl+Shift+Z no-op. | `CommandStack::redo()` return check, `can_redo()` in menu |
| Undo beyond stack bounds | Repeated undo eventually empties stack, then returns false. | `CommandStack::undo()` empty check |
| Redo after new command | Executing new command clears redo stack. Previously undone commands lost. | `CommandStack::execute()` clears `redo_stack_` |
| Ctrl+Z while ImGui captures keyboard | Shortcut suppressed; keystroke passes to ImGui. | `ShortcutRegistry::process()` WantCaptureKeyboard guard |
| First launch — no ini file | Default layout via DockBuilder. No crash. | `draw_ui()` first_layout block |
| Corrupt ini file | ImGui silently ignores corrupt ini and uses default layout. No crash. | ImGui internal behavior |
| Layout file deleted between sessions | Default layout on next launch. New ini created on shutdown. | ImGui internal behavior |
| Help > About opened twice quickly | `ImGui::OpenPopup` is idempotent — second call is no-op. | ImGui internal behavior |
| Ctrl+Q while About popup open | Modal captures keyboard → WantCaptureKeyboard true → shortcut suppressed. | `ShortcutRegistry::process()` WantCaptureKeyboard guard |
| max_history = 0 | Clamped to minimum 1 in constructor. | `CommandStack` constructor `std::max` |
| ImGui not initialized (headless) | `Editor::setup()` returns error. `draw_ui()` and `update()` are no-ops. CommandStack testable headlessly. | `setup()` guard, `draw_ui()`/`update()` `initialized_` guard |
| `app.update(ctx)` during frame exit | If `update()` calls `ctx.request_exit()`, the exit check after `update()` breaks out of the render loop cleanly. | App::update() exit check pattern in `run_app()` |
| Existing App subclasses not overriding `update()` | Default empty implementation — no behavior change, no compile error. | App::update() default `{}` |
| Shortcuts processed in `update()` vs `draw_ui()` | No visual difference to user; logic runs before `render_scene()` and before `draw_ui()`. | Two-phase separation |
| QuitCommand undo called | No-op — exit flag cannot be cleared. | `QuitCommand::undo()` empty body |
| ShortcutRegistry::process() called with key held for multiple frames | `is_pressed()` returns true only on the first frame the key is down. Subsequent frames return false until released and pressed again. Action fires only once per press. | `process()` uses `input.is_pressed()` |

## Security impact

None. The editor writes a layout file (`buddd_editor.ini`) to the current working directory containing only ImGui docking state (window positions, sizes, docking splits). No sensitive data, no elevated privileges, no network access. Architecture boundary (ADR-019) preserved.

## Data and migration impact

None. The `buddd_editor.ini` file is created on first shutdown and read on subsequent launches. No schema changes, existing data migrations, or seed data. If the file is deleted, a new default layout is used.

## API compatibility impact

- **`buddd_editor` library**: New public API — `Command`, `CommandStack`, `ShortcutRegistry`, `EditorMenu`, `EditorPanel`, `MenuBar`, `ScenePanel`, `PropertiesPanel`, `ConsolePanel`, `ProjectPanel`, `AssetsPanel`, `QuitCommand` in `namespace buddd::editor`. The `Editor` class gains public methods `update()`, `add_menu()`, `add_panel()`. Individual private panel drawing methods removed (replaced by `panels_` vector iteration). `ShowAboutCommand` removed (About handled via callback).
- **`App` base class** (`buddd::cmd::App`): New virtual method `update(EngineContext const&)` with default empty implementation. All existing subclasses unaffected (no override needed).
- **Engine API**: Unchanged.
- **CLI API**: Unchanged — `buddd edit` works the same way but now shows menus/panels.
- **Test API**: New `[editor][command]` tagged tests. Existing headless tests unchanged.

## Documentation impact

- **Wiki pages to update** (wiki-agent handles these, not the implementer):
  - `docs/wiki/architecture/module-map.md` — Add entries for new files: command system (`command.h/.cpp`, `command_stack.h/.cpp`, `commands/` subdirectory), ShortcutRegistry (`shortcut_registry.h`), EditorMenu/EditorPanel abstractions (`editor_menu.h`, `editor_panel.h`), and panel subdirectory (`panels/` with `menu_bar.h`, `scene_panel.h`, `properties_panel.h`, `console_panel.h`, `project_panel.h`, `assets_panel.h`).
  - `docs/wiki/architecture/dependency-map.md` — Update `buddd_editor` dependencies if any change (likely none — remains `buddd_editor ──PUBLIC──► buddd_engine`).
  - `docs/wiki/architecture/overview.md` — Update `src/editor/` directory listing to reflect new subdirectories (`commands/`, `panels/`) and all new files.
- **Specs**: None.
- **README**: None.

## ADR impact

- **ADR-027** (Editor Architecture): This contract implements the editor architecture defined in ADR-027. No changes to the ADR.
- **ADR-026** (Dear ImGui Integration): The `io.IniFilename` setting in `Editor::setup()` is a new consumer of ADR-026's API. No ADR change needed.
- **ADR-014** (CLI App System): `App::update()` lifecycle method added with default empty impl. No existing subclass affected. The ADR's render loop description is extended but not contradicted.
- New ADR may be warranted in the future if the command system needs significant extension, but not for this feature.

## Done criteria

The implementation is complete when ALL of the following are verifiable:

- [ ] **DC-001**: `src/editor/command.h` exists and declares `class Command` with virtual `execute()`, `undo()`, and `name()` methods in `namespace buddd::editor`.
- [ ] **DC-002**: `src/editor/command.cpp` exists (may be empty, for build consistency).
- [ ] **DC-003**: `src/editor/command_stack.h` declares `class CommandStack` with all 9 methods (`execute`, `undo`, `redo`, `can_undo`, `can_redo`, `undo_name`, `redo_name`, `clear`) plus constructor taking `size_t max_history = 128`.
- [ ] **DC-004**: `src/editor/command_stack.cpp` implements all `CommandStack` methods. `max_history` is clamped to minimum 1. `undo()`/`redo()` return false when respective stacks are empty.
- [ ] **DC-005**: `src/editor/commands/quit_command.h` declares `QuitCommand` with `execute()` calling `ctx.request_exit()` and `undo()` as a no-op. `name()` returns `"Quit"`.
- [ ] **DC-006**: `src/editor/shortcut_registry.h` declares `class ShortcutRegistry` with `bind()` and `process()` methods in `namespace buddd::editor`. `process()` uses `is_pressed()` (edge-triggered) for action keys, checks both left/right modifier variants, and gates on `want_capture`.
- [ ] **DC-007**: `src/editor/editor_menu.h` exists and declares `class EditorMenu` with virtual `id()`, `update()`, and `draw_ui()` methods in `namespace buddd::editor`. `update()` and `draw_ui()` have default empty implementations.
- [ ] **DC-008**: `src/editor/editor_panel.h` exists and declares `class EditorPanel` with virtual `id()`, `title()`, `update()`, and `draw_ui()` methods. `update()` and `draw_ui()` have default empty implementations.
- [ ] **DC-009**: `src/editor/panels/menu_bar.h` exists and declares `class MenuBar` extending `EditorMenu`. Takes `CommandStack&` in constructor. Has `set_on_about(std::function<void()>)` method. Implementation of `draw_ui()` renders File/Edit/Help menus via `BeginMainMenuBar()`. About action invokes callback (not a Command). No `bool& show_about_` parameter.
- [ ] **DC-010**: Five panel header files exist in `src/editor/panels/`: `scene_panel.h`, `properties_panel.h`, `console_panel.h`, `project_panel.h`, `assets_panel.h`. Each declares a class extending `EditorPanel` with correct `id()` and `title()`. Each sets 100×100 minimum size constraint with no functional content.
- [ ] **DC-011**: `src/editor/editor.h` declares public `update(EngineContext const& ctx) -> void` method. Includes `"editor_menu.h"`, `"editor_panel.h"`, `"command_stack.h"`, `"shortcut_registry.h"`. Declares `add_menu(unique_ptr<EditorMenu>)` and `add_panel(unique_ptr<EditorPanel>)` public methods. Adds `shortcuts_` (ShortcutRegistry), `menus_` and `panels_` vectors. No individual panel drawing method declarations.
- [ ] **DC-012**: `src/editor/editor.cpp` sets `ImGui::GetIO().IniFilename = "buddd_editor.ini"` in `setup()` after ImGui init check. `setup()` creates MenuBar with `CommandStack&` only, sets About callback via `set_on_about()`, registers all five panels via `add_panel()`, and registers shortcuts via `shortcuts_.bind()`. `Editor::update()` calls `shortcuts_.process()`, then iterates `menus_` and `panels_` calling `update(ctx)` on each. `draw_ui()` has 4-phase rendering: Phase 1 menus iteration → Phase 2 DockSpaceOverViewport + default layout → Phase 3 panels iteration with Begin/End → Phase 4 about popup.
- [ ] **DC-013**: `Editor::update()` processes keyboard shortcuts via `ShortcutRegistry::process()` which gates on `WantCaptureKeyboard`. Four shortcuts registered: Ctrl+Q (quit), Ctrl+Z (undo), Ctrl+Shift+Z (redo), Ctrl+Y (redo).
- [ ] **DC-014**: `draw_about_popup()` shows modal "About Buddd Editor" with `"Buddd Engine v{version}"` text and Close button. Flag `show_about_` is reset when popup is dismissed.
- [ ] **DC-015**: `src/editor/CMakeLists.txt` lists `editor.cpp`, `command.cpp`, `command_stack.cpp` as sources. Notes that `commands/` and `panels/` contain header-only files.
- [ ] **DC-016**: `tests/editor_tests.cpp` has `TEST_CASE` entries for `[editor][command]` covering: fresh stack, execute/undo/redo cycle, new command clears redo, max_history bound, max_history clamping, clear, and empty name views.
- [ ] **DC-017**: Build succeeds: `cmake --build --preset debug` — no compile or link errors. All 14 existing demo scenes (`triangle`, `cube`, `phong`, etc.) compile without changes.
- [ ] **DC-018**: Test passes: `buddd_tests` passes all command stack tests.
- [ ] **DC-019**: No SDL3/OpenGL/GLM headers leak: `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/` → zero matches.
- [ ] **DC-020**: No SDL3/OpenGL/GLM headers leak from commands: `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/commands/` → zero matches. Also check `src/editor/panels/` and `src/editor/shortcut_registry.h`.
- [ ] **DC-021** (Manual): `buddd edit` shows menu bar with File/Edit/Help, five dockable panels visible, About popup works.
- [ ] **DC-022** (Manual): `buddd_editor.ini` created after closing editor. Layout persists across sessions.
- [ ] **DC-023**: `src/cmd/app.h` declares `virtual auto update(EngineContext const& ctx) -> void {}` with default empty implementation, placed after `on_frame_begin()` and before `on_render()`.
- [ ] **DC-024**: `src/cmd/app.cpp` calls `app.update(ctx)` after `world->update_updatables(ctx)` and before `render_system->render_scene()`, with exit check after the call.
- [ ] **DC-025**: `src/cmd/apps/editor_app.h` declares `auto update(EngineContext const& ctx) -> void override;`.
- [ ] **DC-026**: `src/cmd/apps/editor_app.cpp` implements `EditorApp::update()` calling `editor_->update(ctx)`. `EditorApp::on_render()` continues to call `editor_->draw_ui(ctx)`.
- [ ] **DC-027**: All 14 existing demo scenes (`buddd run triangle`, `buddd run cube`, etc.) build and run unchanged after adding `App::update()`. Verify by building `--preset debug` and running `buddd run triangle`.

# SPEC-028 — Editor Foundation

## Problem

The current editor (built in the scaffolding sprint) is an empty 1280×800 ImGui dockspace with no infrastructure for interactive features:

- **No command system**: There is no way to model user actions as undoable operations. Every future editor feature (transform, delete, rename) would need to invent its own undo/redo mechanism from scratch.
- **No main menu**: The editor has no menu bar. Users cannot discover features through traditional File/Edit/Help menus. There is no way to quit except closing the window.
- **No keyboard shortcuts**: Context-sensitive keys (Ctrl+Z for undo, Ctrl+Q for quit, etc.) are not wired. Users must use the mouse for every interaction.
- **No panels**: There are no dockable windows (Scene, Properties, Console, Project, Assets) — only the bare dockspace. Every panel feature would need to create its own windowing setup.
- **No docking persistence**: `io.IniFilename` is explicitly set to `nullptr` — layout is lost between sessions. Users must rearrange panels every time.
- **No input routing**: `ImGui::GetIO().WantCaptureKeyboard` and `WantCaptureMouse` are never checked. Keyboard shortcuts cannot safely coexist with ImGui text input.

Without this foundation, every subsequent editor feature would need to solve the same infrastructure problems (command dispatch, keyboard handling, menu registration, window layout) independently — duplicating effort and producing inconsistent user experiences.

## Goals

- **G-01 — Command infrastructure**: Create a `Command` abstract base class with `execute()` / `undo()`, and a `CommandStack` class with bounded undo/redo history. All editor menu actions are dispatched through commands.
- **G-02 — Main menu bar**: Add an ImGui main menu bar with three menus:
  - **File**: Quit (Ctrl+Q)
  - **Edit**: Undo (Ctrl+Z), Redo (Ctrl+Shift+Z / Ctrl+Y)
  - **Help**: About (opens modal popup with engine name + version)
- **G-03 — Keyboard shortcuts**: All menu items have working keyboard shortcuts. Shortcuts are gated by `!ImGui::GetIO().WantCaptureKeyboard` to avoid firing during text input.
- **G-04 — Placeholder dockable panels**: Five empty ImGui windows dockable in the workspace: Scene, Properties, Console, Project, Assets. Each panel is an `ImGui::Begin()` / `End()` block with no functional content.
- **G-05 — Docking persistence**: Enable `io.IniFilename` in the editor so that panel layout (docking arrangement, positions, sizes) is saved between sessions. On restart, the previous layout is restored.
- **G-06 — About popup**: Help > About opens a modal popup showing engine name ("Buddd Engine") and version string (from `buddd::engine::version()`).
- **G-07 — Architecture boundary preserved**: No SDL3, OpenGL, or GLM headers appear in `src/editor/` or `src/cmd/apps/`. Verified by grep.
- **G-08 — Headless safety**: All ImGui-dependent code is guarded. Unit tests can construct the `Editor` and exercise command/stack logic without a display.
- **G-09 — App::update() lifecycle**: Add a virtual `update(EngineContext const&)` method to the `App` base class, called once per frame just after `world->update_updatables()`, for non-rendering per-frame logic (shortcut processing, editor state updates). Default empty implementation so existing App subclasses are unaffected.

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | No functional panel content. Scene panel does not show entities, Properties does not show properties, Console does not log messages, Project does not browse files, Assets does not show assets. |
| NG-02 | No scene viewport rendering inside ImGui (or anywhere else in the editor). |
| NG-03 | No gizmos, entity selection, drag-and-drop, or in-viewport interaction. |
| NG-04 | No dynamic plugin/panel discovery system. Panels are registered via `Editor::add_panel()` in `Editor::setup()` — no runtime DLL loading or config-file-based discovery. |
| NG-05 | No changes to engine core (`src/engine/`). Changes to `src/cmd/` are limited to: (1) one virtual `update()` method added to `App` base class in `src/cmd/app.h`, (2) one `app.update(ctx)` call in `run_app()` in `src/cmd/app.cpp`. Engine core (`src/engine/`) is untouched. |
| NG-06 | No preferences system, theme customisation, or colour scheme editor. |
| NG-07 | No multi-viewport ImGui support (`ImGuiConfigFlags_ViewportsEnable`). |
| NG-08 | No undo/redo of actual editor operations — only the infrastructure and menu/shortcut dispatch. Undo/redo of real operations (transform, delete, etc.) is future work. |
| NG-09 | No persistent storage for user preferences beyond the docking layout file. |
| NG-10 | No IMGUI_DISABLE / headless-mode compilation of ImGui-dependent editor code — ImGui code is guarded at runtime via `if (initialized_)`, not removed at compile time from the editor library. |

## Actors

| Actor | Description |
|---|---|
| **Developer** | A human running `buddd edit`. Interacts with the menu bar, keyboard shortcuts, and dockable panels. |
| **Editor developer** | A developer adding new editor features. Uses the command infrastructure (`Command` subclasses, `CommandStack`) and extends `Editor::draw_ui()` with new panels. |

## User-visible behavior

### Menu bar

The editor window has a main menu bar at the top with three menus:

```
File    Edit    Help
├─ Quit  ├─ Undo  ├─ About
         └─ Redo
```

- **File > Quit**: Exits the editor (calls `ctx.request_exit()`). Always enabled. Keyboard shortcut: `Ctrl+Q`.
- **Edit > Undo**: Calls `CommandStack::undo()`. Disabled (greyed out) when the undo stack is empty. Keyboard shortcuts: `Ctrl+Z`.
- **Edit > Redo**: Calls `CommandStack::redo()`. Disabled when the redo stack is empty. Keyboard shortcuts: `Ctrl+Shift+Z`, `Ctrl+Y`.
- **Help > About**: Opens a modal popup titled "About Buddd Editor" showing:
  - "Buddd Engine v{version}" (where `{version}` is the string returned by `buddd::engine::version()`)
  - A "Close" button to dismiss the popup
  - Keyboard shortcut: none (menu-only).

### Keyboard shortcuts

| Shortcut | Action | Condition |
|---|---|---|
| `Ctrl+Q` | Quit | `!ImGui::GetIO().WantCaptureKeyboard` |
| `Ctrl+Z` | Undo | `!ImGui::GetIO().WantCaptureKeyboard` |
| `Ctrl+Shift+Z` | Redo | `!ImGui::GetIO().WantCaptureKeyboard` |
| `Ctrl+Y` | Redo (secondary) | `!ImGui::GetIO().WantCaptureKeyboard` |

Shortcuts are processed via `ShortcutRegistry::process()` in `Editor::update()`, using the engine's `InputSystem` to detect key/modifier state. The registry iterates registered bindings and fires the associated callback when a key+modifier combination is pressed (edge-triggered via `is_pressed()`, not continuous `is_down()`). Processing is gated by `!ImGui::GetIO().WantCaptureKeyboard` to avoid firing shortcuts during text input or while a modal popup is active.

### Dockable panels

After the menu bar, the workspace area contains five dockable ImGui windows. On first launch (no ini file), they arrange as:

| Panel | Default location | Default size |
|---|---|---|
| Scene | Center (main viewport) | Fills remaining space |
| Properties | Right side | ~300px wide |
| Console | Bottom | ~200px tall |
| Project | Bottom-left (tabbed with Assets or separate) | ~250×200px |
| Assets | Bottom-right (tabbed with Project or separate) | ~250×200px |

Each panel has:
- A title bar with the panel name
- A close button (hides the panel for the current session; on next launch they reappear)
- Dockable: can be moved, resized, tabbed, split
- Minimum size constraint of 100×100 pixels via `ImGui::SetNextWindowSizeConstraints()` to prevent accidental zero-size panels

Panels are empty (no widgets inside except the title bar).

### Docking persistence

- On editor startup, ImGui reads `buddd_editor.ini` from the current working directory (or `ImGui::GetIO().IniFilename` path).
- On editor shutdown, ImGui writes the docking layout to the same file.
- On first launch (no ini file), ImGui uses the built-in default layout.

### About popup

The About dialog is an ImGui modal popup:

```
┌─────────────────────────────────────┐
│  About Buddd Editor                 │
│                                     │
│  Buddd Engine v0.1.0               │
│                                     │
│           [ Close ]                 │
└─────────────────────────────────────┘
```

- Opened via Help > About menu item.
- Closed by clicking the Close button, pressing Escape, or clicking outside the modal.
- Non-blocking for the rest of the editor (it's a modal, but user can interact with other parts of the editor? No — ImGui modal blocks interaction with other windows until dismissed).

## Key entities

### `App` class — new `update()` method (`src/cmd/app.h`)

A new virtual method is added to the `App` base class:

```cpp
/// Called once per frame just after world->update_updatables(ctx),
/// between begin_frame() and end_frame(), before render_scene().
/// Dedicated to per-frame logic (shortcuts, state updates) separate from rendering.
/// Default no-op. Override to add per-frame logic.
virtual auto update(buddd::engine::EngineContext const& ctx) -> void {}
```

The render loop in `run_app()` gains one call between `update_updatables` and `render_scene`:

```
world->update_updatables(ctx);
app.update(ctx);           // NEW
render_system->render_scene();
```

The default empty implementation ensures all 14 existing `App` subclasses are unaffected — they continue to work without overriding `update()`.

### `Editor` class — new `update()` method

The `Editor` class splits its per-frame work into two methods:

- **`update(ctx)`**: Non-rendering logic (keyboard shortcut processing, command dispatch). Called from `EditorApp::update()`.
- **`draw_ui(ctx)`**: UI rendering only (menu bar, panels, popups). Called from `EditorApp::on_render()`.

```cpp
/// Process editor logic: keyboard shortcuts, state updates.
/// Called every frame from EditorApp::update().
auto update(buddd::engine::EngineContext const& ctx) -> void;

/// Draw the ImGui UI: menu bar, dockable panels, popups.
/// Called every frame from EditorApp::on_render().
auto draw_ui(buddd::engine::EngineContext const& ctx) -> void;
```

### `Command` class (`src/editor/command.h`)

```cpp
namespace buddd::editor {

/// Abstract base for all editor commands.
/// Lifecycle: construct → execute() → (undo() | execute())...
/// A command that has been executed can be undone. A command that has been undone
/// can be re-executed via execute() (not redo() — redo means undo the undo).
class Command {
public:
    virtual ~Command() = default;

    /// Execute (or re-execute) the command.
    virtual auto execute() -> void = 0;

    /// Undo the command. Only called after execute().
    virtual auto undo() -> void = 0;

    /// Human-readable name for menu display (e.g., "Undo Quit").
    [[nodiscard]] virtual auto name() const -> std::string_view = 0;
};

} // namespace buddd::editor
```

- `execute()` performs the action and stores enough state to undo it.
- `undo()` reverses the action using stored state.
- `name()` returns a short description (e.g., "Quit", "Undo", "Redo", "About").
- Concrete commands must handle double-execute safely (execute after execute without undo in between is allowed for actions like Quit which are one-shot).

### `CommandStack` class (`src/editor/command_stack.h`)

```cpp
namespace buddd::editor {

/// Bounded undo/redo stack for editor commands.
/// Thread-compatible: not thread-safe — used from the main thread only.
class CommandStack {
public:
    /// @param max_history Maximum number of undoable commands to retain.
    ///                     Default 128. Must be >= 1.
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

- `execute()` calls `command->execute()`, pushes to undo stack, clears redo stack.
- `undo()` pops from undo stack, calls `undo()`, pushes to redo stack.
- `redo()` pops from redo stack, calls `execute()`, pushes to undo stack.
- When undo stack exceeds `max_history_`, the oldest command is discarded.
- `max_history_` is a soft bound: the stack can exceed it by 1 briefly during `execute()` before the oldest entry is trimmed.

### Concrete commands (`src/editor/commands/`)

**`QuitCommand`**: Stores a pointer to `EngineContext`. `execute()` calls `ctx->request_exit()`. `undo()` is a no-op (cannot un-request exit — once the exit flag is set, the app will shut down). `name()` returns `"Quit"`.

Note: `Undo` and `Redo` menu items do NOT use `Command` subclasses. They invoke `CommandStack::undo()` and `CommandStack::redo()` directly from the menu handler. This avoids the conceptual oddity of undoable undo commands and keeps the stack simple.

Note: The About action is handled via a direct callback on the MenuBar (not a Command), as showing a modal popup is a UI operation too trivial for the undo stack.

### `ShortcutRegistry` class (`src/editor/shortcut_registry.h`)

A registry mapping keyboard shortcuts to callbacks. Simplifies per-frame shortcut processing and keeps `Editor::update()` clean.

```cpp
namespace buddd::editor {

/// Binds keyboard shortcuts to callback actions.
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
    /// `want_capture` gates processing (no action fires when ImGui captures keyboard).
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
- `process()` iterates bindings and calls `action()` when the shortcut is pressed, unless `want_capture` is true (ImGui has keyboard focus).
- Modifier checking: `input.is_down(KeyCode::ControlLeft) || input.is_down(KeyCode::ControlRight)` for Ctrl, same for Shift (left/right).
- `process()` uses `input.is_pressed()` (single-frame edge-triggered), not `is_down()` (continuous), to avoid repeated firing.

### `EditorMenu` abstract class (`src/editor/editor_menu.h`)

Base class for editor overlay elements drawn before the ImGui dockspace (menu bar, future toolbar and status bar).

```cpp
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
    /// before ImGui::DockSpaceOverViewport().
    virtual auto draw_ui(buddd::engine::EngineContext const& /*ctx*/) -> void {}
};

} // namespace buddd::editor
```

### `EditorPanel` abstract class (`src/editor/editor_panel.h`)

Base class for dockable editor panels drawn inside the ImGui dockspace.

```cpp
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

### MenuBar concrete class (`src/editor/panels/menu_bar.h`)

Extends `EditorMenu` with the main menu bar:

```
File    Edit    Help
├─ Quit  ├─ Undo  ├─ About
         └─ Redo
```

- Uses `ImGui::BeginMainMenuBar()` / `EndMainMenuBar()`.
- Takes a `CommandStack&` reference for executing commands and checking undo/redo availability.
- Uses `std::function<void()>` callbacks for actions that are not simple command-stack operations (e.g., About dialog).
- `draw_ui()` renders each menu with `ImGui::BeginMenu()` / `MenuItem()`.
- Menu items for Quit, Undo, Redo operate directly on the CommandStack or EngineContext.
- File > Quit: `stack_.execute(std::make_unique<QuitCommand>(ctx))`
- Edit > Undo/Redo: `stack_.undo()` / `stack_.redo()`, with enabled/disabled state from `stack_.can_undo()` / `stack_.can_redo()`
- Help > About: invokes a callback (`on_about_()`) set by Editor.
- Keyboard shortcuts are processed in `Editor::update()` via `ShortcutRegistry`, not in the MenuBar.

### Concrete panel classes

Five concrete `EditorPanel` subclasses, one per placeholder panel:

| Class | File | Id | Title |
|---|---|---|---|
| `ScenePanel` | `src/editor/panels/scene_panel.h` | `"scene"` | `"Scene"` |
| `PropertiesPanel` | `src/editor/panels/properties_panel.h` | `"properties"` | `"Properties"` |
| `ConsolePanel` | `src/editor/panels/console_panel.h` | `"console"` | `"Console"` |
| `ProjectPanel` | `src/editor/panels/project_panel.h` | `"project"` | `"Project"` |
| `AssetsPanel` | `src/editor/panels/assets_panel.h` | `"assets"` | `"Assets"` |

Each panel v1 implementation contains only a `draw_ui()` that opens an ImGui window with its title and sets a minimum size constraint of 100×100 via `ImGui::SetNextWindowSizeConstraints()`. No functional content. The `update()` method is a no-op.

### `Editor` class changes (`src/editor/editor.h`)

New public methods:

```cpp
/// Process editor logic: keyboard shortcuts, state updates.
/// Called every frame from EditorApp::update(), after world->update_updatables().
auto update(buddd::engine::EngineContext const& ctx) -> void;

/// Register a menu overlay (takes ownership).
auto add_menu(std::unique_ptr<EditorMenu> menu) -> void;

/// Register a dockable panel (takes ownership).
auto add_panel(std::unique_ptr<EditorPanel> panel) -> void;
```

New private members:

```cpp
// Command system
CommandStack command_stack_;

// Shortcut registry
ShortcutRegistry shortcuts_;

// Registered overlays (drawn before dockspace)
std::vector<std::unique_ptr<EditorMenu>> menus_;

// Registered panels (drawn inside dockspace)
std::vector<std::unique_ptr<EditorPanel>> panels_;

// Panel state flags
bool show_about_ = false;
```

`Editor::setup()` creates and registers the default menus, panels, and shortcuts:

```
Editor::setup(ctx):
  1. command_stack_ = CommandStack(128)
  
  2. // Create menu bar
     auto menu_bar = std::make_unique<MenuBar>(command_stack_);
     menu_bar->set_on_about([this]() { show_about_ = true; });
     add_menu(std::move(menu_bar));
  
  3. // Register panels
     add_panel(std::make_unique<ScenePanel>());
     add_panel(std::make_unique<PropertiesPanel>());
     add_panel(std::make_unique<ConsolePanel>());
     add_panel(std::make_unique<ProjectPanel>());
     add_panel(std::make_unique<AssetsPanel>());
  
  4. // Register shortcuts
     shortcuts_.bind(KeyCode::Q, {.ctrl = true}, [this, &ctx]() {
         quit(ctx);
     });
     shortcuts_.bind(KeyCode::Z, {.ctrl = true}, [this]() {
         command_stack_.undo();
     });
     shortcuts_.bind(KeyCode::Z, {.ctrl = true, .shift = true}, [this]() {
         command_stack_.redo();
     });
     shortcuts_.bind(KeyCode::Y, {.ctrl = true}, [this]() {
         command_stack_.redo();
     });
  
  5. // Enable ini persistence
     ImGui::GetIO().IniFilename = "buddd_editor.ini";
```

Where `quit(ctx)` is a private helper that calls `command_stack_.execute(std::make_unique<QuitCommand>(ctx))`.

### Two-phase update/render separation

The editor separates logic from rendering into two distinct phases each frame:

```
Frame:
  │
  ├─ on_frame_begin(ctx)                — App hook (hot-reload, etc.)
  ├─ update_updatables(ctx)             — ECS Updatable components
  ├─ Editor::update(ctx)                — logic: shortcuts, dispatch commands
  │   └─ InputSystem + !WantCaptureKeyboard
  │       ├─ for menus_  → menu->update(ctx)
  │       ├─ for panels_ → panel->update(ctx)
  │       └─ dispatch to command_stack_.execute(...) or .undo()/.redo()
  ├─ render_scene()                     — 3D scene rendering
  ├─ Editor::draw_ui(ctx)               — UI rendering
  │   ├─ Phase 1 — Overlays (before dockspace):
  │   │   └─ for menus_ → menu->draw_ui(ctx)
  │   ├─ Phase 2 — Dockspace:
  │   │   └─ ImGui::DockSpaceOverViewport(...)
  │   ├─ Phase 3 — Panels (inside dockspace):
  │   │   └─ for panels_:
  │   │       ImGui::Begin(panel->title().data());
  │   │       panel->draw_ui(ctx);
  │   │       ImGui::End();
  │   └─ Phase 4 — About popup (if show_about_):
  │       └─ ImGui::BeginPopupModal("About Buddd Editor")
  └─ render_ui()                        — ImGui::Render()
```

**`Editor::update(ctx)`** — called from `EditorApp::update()`:
- Processes keyboard shortcuts via `shortcuts_.process(input, ImGui::GetIO().WantCaptureKeyboard)`
- Calls `menu->update(ctx)` and `panel->update(ctx)` on all registered parts

**`Editor::draw_ui(ctx)`** — called from `EditorApp::on_render()`:
- Phase 1: Renders all menus (MenuBar via `BeginMainMenuBar`)
- Phase 2: Creates the ImGui dockspace
- Phase 3: Renders all panels as dockable ImGui windows
- Phase 4: Renders the About modal popup if `show_about_` is true

When a menu item or shortcut triggers a command, it calls `command_stack_.execute(make_unique<ConcreteCommand>(...))`. The CommandStack calls `command->execute()` internally.

Commands like `QuitCommand` call `ctx.request_exit()`. Commands like `ShowAboutCommand` set `show_about_ = true`.

Undo and Redo are exceptions: they call `command_stack_.undo()` and `command_stack_.redo()` directly, bypassing the command stack to avoid circular undoable meta-commands.

The About popup is rendered every frame if `show_about_` is true. When the user dismisses it, ImGui calls the popup close callback which resets the flag.

## File changes

### Created

| File | Purpose |
|---|---|
| `src/editor/command.h` | `Command` abstract base class declaration. |
| `src/editor/command.cpp` | `Command` virtual destructor (empty, but needed for vtable emission). |
| `src/editor/command_stack.h` | `CommandStack` class declaration. |
| `src/editor/command_stack.cpp` | `CommandStack` implementation (execute, undo, redo, clear, bounds). |
| `src/editor/commands/quit_command.h` | `QuitCommand` declaration + inline implementation (trivial). |
| `src/editor/shortcut_registry.h` | `ShortcutRegistry` class — bind shortcuts to callbacks, process input. |
| `src/editor/editor_menu.h` | `EditorMenu` abstract base class for overlay elements (menu bar). |
| `src/editor/editor_panel.h` | `EditorPanel` abstract base class for dockable panels. |
| `src/editor/panels/menu_bar.h` | `MenuBar` concrete class extending `EditorMenu`. |
| `src/editor/panels/scene_panel.h` | `ScenePanel` — placeholder dockable panel. |
| `src/editor/panels/properties_panel.h` | `PropertiesPanel` — placeholder dockable panel. |
| `src/editor/panels/console_panel.h` | `ConsolePanel` — placeholder dockable panel. |
| `src/editor/panels/project_panel.h` | `ProjectPanel` — placeholder dockable panel. |
| `src/editor/panels/assets_panel.h` | `AssetsPanel` — placeholder dockable panel. |

### Modified

| File | Change |
|---|---|
| `src/editor/editor.h` | Add `#include`s for editor_menu.h, editor_panel.h, command_stack.h, shortcut_registry.h; add `update()`, `add_menu()`, `add_panel()` methods; add `menus_`, `panels_`, `command_stack_`, `shortcuts_` members. |
| `src/editor/editor.cpp` | Restructure `setup()` to register shortcuts, menus, panels; `update()` processes shortcuts via ShortcutRegistry + delegates to menus/panels; `draw_ui()` has 4-phase rendering (menus → dockspace → panels → about). Set `io.IniFilename` in `setup()`. |
| `src/editor/CMakeLists.txt` | Add new `.cpp` source files (command.cpp, command_stack.cpp, panel .cpp files). |
| `src/cmd/app.h` | Add virtual `update(EngineContext const& ctx) -> void {}` method (default empty). Update lifecycle comment. |
| `src/cmd/app.cpp` | Add `app.update(ctx)` call in render loop just after `world->update_updatables(ctx)`. |
| `src/cmd/apps/editor_app.h` / `.cpp` | EditorApp overrides `update()` to call `editor_->update(ctx)`. |
| `tests/editor_tests.cpp` | Add tests for `CommandStack`, command lifecycle, headless safety. |
| `docs/wiki/architecture/module-map.md` | Add entries for command system files (`command.h/.cpp`, `command_stack.h/.cpp`, `commands/` subdirectory). |
| `docs/wiki/architecture/dependency-map.md` | Add command/stack internal dependencies within `buddd_editor`. |
| `docs/wiki/architecture/overview.md` | Update `src/editor/` directory listing to reflect new subdirectories and files. |

### Unchanged

| File | Reason |
|---|---|
| `src/engine/imgui/engine_imgui.cpp` | The editor sets `io.IniFilename` in its own `setup()`; engine default stays `nullptr`. |
| `src/cmd/main.cpp` | No changes — `buddd edit` dispatch unchanged. |
| All other engine files | No engine changes needed for this feature. |

## User stories

### Story 1 — Command infrastructure (Priority: P1)

As an editor developer, I want a `Command` base class and a `CommandStack` with bounded undo/redo, so that future features can implement undoable operations without reinventing the dispatch mechanism.

**Given** a `CommandStack` with max_history of 5
**When** I execute 3 commands, undo 1, then execute a 4th command
**Then** the redo stack is cleared, the undo stack has 3 entries (including the new one), and I can undo all 3.

### Story 2 — Main menu with keyboard shortcuts (Priority: P1)

As a developer using `buddd edit`, I want a menu bar with File > Quit, Edit > Undo/Redo, and Help > About, each with working keyboard shortcuts, so that I can discover and invoke editor features efficiently.

**Given** the editor window is open
**When** I click "Help" then "About"
**Then** a modal popup appears titled "About Buddd Editor" showing "Buddd Engine v0.1.0" and a Close button.
**When** I press Ctrl+Q
**Then** the editor exits.

### Story 3 — Dockable placeholder panels (Priority: P1)

As a developer, I want five dockable panels (Scene, Properties, Console, Project, Assets) visible in the editor workspace, so that I can arrange my layout and have panels ready for future features.

**Given** the editor window is open
**Then** I see panels titled "Scene", "Properties", "Console", "Project", and "Assets" docked in the workspace.
**When** I drag a panel by its title bar
**Then** I can dock it to different positions, tab it with other panels, or float it.

### Story 4 — Docking persistence (Priority: P2)

As a developer, I want the editor to remember my panel layout between sessions, so that I don't have to rearrange panels every time I launch the editor.

**Given** the editor window is open
**When** I rearrange panels, close the editor, and relaunch
**Then** the panels appear in the same arrangement as when I closed the editor.

### Story 5 — About dialog (Priority: P2)

As a developer, I want to see the engine version in the editor, so that I can verify which build I'm running.

**Given** the editor window is open
**When** I click Help > About
**Then** a modal popup shows the engine name and version.

### Story 6 — Shortcuts respect ImGui capture (Priority: P2)

As a developer, I want keyboard shortcuts to be suppressed when I'm typing in a text input field, so that I don't accidentally trigger undo/redo while editing text.

**Given** the editor window is open with a future text input field (simulated via `ImGui::InputText`)
**When** the text field has keyboard focus and I press Ctrl+Z
**Then** the shortcut is not triggered (the text field receives the Ctrl+Z instead).

### Story 7 — Headless command system test (Priority: P2)

As an editor developer, I want to verify `CommandStack` logic in headless unit tests, so that I can test undo/redo without a display.

**Given** a `CommandStack` constructed in headless unit test
**When** I execute, undo, and redo dummy commands
**Then** the stack state matches expected values (can_undo, can_redo, undo_name, redo_name).

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `src/editor/command.h` declares `class Command` with virtual `execute()`, `undo()`, `name()` methods in `namespace buddd::editor`. | Inspect file; verify three virtual methods and `std::string_view` return for `name()`. |
| AC-002 | `src/editor/command_stack.h` declares `class CommandStack` with `execute(unique_ptr<Command>)`, `undo() -> bool`, `redo() -> bool`, `can_undo()`, `can_redo()`, `undo_name()`, `redo_name()`, `clear()`. | Inspect file; verify all methods declared. |
| AC-003 | `CommandStack::execute()` calls `command->execute()`, pushes to undo stack, clears redo stack. | Unit test: execute command, verify `can_undo() == true`, `can_redo() == false`. |
| AC-004 | `CommandStack::undo()` returns false when undo stack is empty. | Unit test: on fresh stack, `undo()` returns false. |
| AC-005 | `CommandStack::redo()` returns false when redo stack is empty. | Unit test: on fresh stack, `redo()` returns false. |
| AC-006 | `CommandStack::undo()` pops from undo stack, calls `undo()`, pushes to redo stack. Returns true on success. | Unit test: execute C1, undo → `can_undo() == false`, `can_redo() == true`, `redo_name()` returns C1's name. |
| AC-007 | `CommandStack::redo()` pops from redo stack, calls `execute()`, pushes to undo stack. Returns true on success. | Unit test: execute C1, undo, redo → `can_undo() == true`, `can_redo() == false`. |
| AC-008 | Executing a new command clears the redo stack. | Unit test: execute C1, undo (redo stack has C1), execute C2 → `can_redo() == false`. |
| AC-009 | `CommandStack` enforces max_history bound (oldest command discarded when stack exceeds limit). | Unit test: max_history = 2, execute 3 commands, undo 2, verify `undo_name()` returns second-to-last command's name. |
| AC-010 | `CommandStack::clear()` empties both stacks. | Unit test: execute C1, undo (so both stacks non-empty), clear → `can_undo() == false`, `can_redo() == false`. |
| AC-011 | `src/editor/editor_menu.h` declares `class EditorMenu` with virtual `id()`, `update()`, `draw_ui()`. | Inspect file; verify abstract class with three virtual methods. |
| AC-012 | `src/editor/editor_panel.h` declares `class EditorPanel` with virtual `id()`, `title()`, `update()`, `draw_ui()`. | Inspect file; verify abstract class with four virtual methods. |
| AC-013 | `Editor` has `add_menu(unique_ptr<EditorMenu>)` and `add_panel(unique_ptr<EditorPanel>)` methods. | Inspect `editor.h`; verify both registration methods declared. |
| AC-014 | `Editor::draw_ui()` draws menus before `DockSpaceOverViewport`, then dockable panels inside the dockspace. | Inspect `editor.cpp`; verify 4-phase rendering order. |
| AC-015 | `Editor::update()` calls `menu->update(ctx)` and `panel->update(ctx)` on all registered parts. | Inspect `editor.cpp`; verify delegation to registered parts. |
| AC-016 | `src/editor/shortcut_registry.h` declares `class ShortcutRegistry` with `bind()` and `process()` methods. | Inspect file; verify both methods declared. |
| AC-018 | `File > Quit` exits the editor (window closes, process exits with code 0). | Manual: run `buddd edit`, click File > Quit, verify clean exit. |
| AC-019 | `Edit > Undo` is greyed out (disabled) when undo stack is empty. | Manual: on fresh launch, verify Edit > Undo is disabled (grey text, not clickable). |
| AC-020 | `Edit > Redo` is greyed out when redo stack is empty. | Manual: on fresh launch, verify Edit > Redo is disabled. |
| AC-021 | `Edit > Undo` / `Redo` are enabled/disabled dynamically based on stack state. | Manual: after any command execution (e.g., Help > About), both Undo and Redo state update — Undo becomes enabled, Redo stays disabled. |
| AC-022 | `Help > About` opens a modal popup titled "About Buddd Editor" showing engine name and version. | Manual: click Help > About, verify popup title and content contains "Buddd Engine v0.1.0" (or whatever `buddd::engine::version()` returns). |
| AC-023 | The About popup has a "Close" button that dismisses it. | Manual: open About, click Close, verify popup closes. |
| AC-024 | Pressing `Ctrl+Q` exits the editor (same as File > Quit). | Manual: run `buddd edit`, press Ctrl+Q, verify clean exit. |
| AC-025 | Pressing `Ctrl+Z` performs undo (when undo stack is non-empty). | Manual: execute a command (e.g., open/close About), press Ctrl+Z, verify command is undone (re-enables disabled criterion can be checked via menu state). |
| AC-026 | A `buddd_editor.ini` file is created in the current working directory after running `buddd edit` and closing. | Manual: run `buddd edit`, close it, verify `buddd_editor.ini` exists in CWD. |
| AC-027 | Panel layout is restored on next editor launch. | Manual: rearrange panels (e.g., move Properties to left), close editor, relaunch, verify Properties is still on the left. |
| AC-028 | On first launch (no ini file), a sensible default layout is shown (panels distributed across workspace, not overlapping). | Manual: delete any existing ini file, run `buddd edit`, verify panels are visible and reasonably arranged (Scene centermost, Properties right, Console bottom, etc.). |
| AC-029 | No SDL3, OpenGL, or GLM headers are included from any file under `src/editor/`. | Run `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/` — zero matches. |
| AC-030 | No SDL3, OpenGL, or GLM headers are included from any file under `src/editor/` or `src/cmd/apps/`. | Run `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/ src/cmd/apps/` — zero matches. |
| AC-031 | Build succeeds with `cmake --build --preset debug` after all changes. | Run `cmake --build --preset debug`; verify no compile or link errors. |
| AC-032 | Headless unit test constructs `Editor`, calls `setup()` (which may fail due to no ImGui), calls `shutdown()`, and doesn't crash. | Run `buddd_tests` with `-DBUDDD_HAS_DISPLAY=OFF` build; test passes without crash or ASan error. |
| AC-033 | Headless unit test exercises `CommandStack` (execute, undo, redo, clear, bounds) without any ImGui dependency. | Run `buddd_tests`; verify command stack tests pass. |
| AC-034 | `App` base class declares `virtual auto update(EngineContext const& ctx) -> void {}` with default empty implementation. | Inspect `src/cmd/app.h`; verify method declaration. |
| AC-035 | `run_app()` calls `app.update(ctx)` after `world->update_updatables(ctx)` and before `render_system->render_scene()`. | Inspect `src/cmd/app.cpp` render loop; verify call order. |
| AC-036 | `EditorApp` overrides `update()` and calls `editor_->update(ctx)`. | Inspect `editor_app.h` / `.cpp`; verify override. |
| AC-037 | `Editor::update()` processes keyboard shortcuts via `ShortcutRegistry::process()` + `!WantCaptureKeyboard`. | Manual: run `buddd edit`, press Ctrl+Q, verify exit. Press Ctrl+Z when undo stack is empty, verify no crash. |
| AC-038 | All existing 14 demo scenes (`buddd run triangle`, `buddd run cube`, etc.) build and run unchanged after adding `App::update()`. | Build `--preset debug` and run `buddd run triangle` — renders correctly. |
| AC-039 | A new dockable panel can be added by creating an `EditorPanel` subclass and calling `editor.add_panel(...)` in `Editor::setup()`, without modifying engine code or `EditorApp`. | Developer creates a "TestPanel" extending `EditorPanel`, registers in setup(), rebuilds, runs `buddd edit`, sees the new panel dockable. |
| AC-040 | A new menu overlay can be added by creating an `EditorMenu` subclass and calling `editor.add_menu(...)` in `Editor::setup()`. | Developer creates a "TestMenu" extending `EditorMenu`, registers in setup(), rebuilds, runs `buddd edit`, sees the new menu rendered. |
| AC-041 | A new shortcut can be added by calling `shortcuts_.bind()` in `Editor::setup()` without modifying `Editor::update()`. | Developer binds a new shortcut in setup(), rebuilds, verifies the shortcut fires correctly. |
| AC-042 | `ShortcutRegistry::process()` respects `WantCaptureKeyboard` — suppresses all shortcuts when true. | Manual: open About popup, press Ctrl+Q while modal is open, verify editor does NOT exit. Dismiss popup, press Ctrl+Q, verify exit. |
| AC-043 | `ShortcutRegistry` does not persist key state between frames (`is_pressed()` used, not `is_down()`). | Unit test: create ShortcutRegistry, call process() twice with same key held down; action fires only once. |

## E2E Verification

| Method | Description |
|---|---|
| **Manual (display)** | Run `buddd edit`. Verify menu bar, all three menus. Verify File > Quit exits. Verify Help > About shows popup. Verify panels visible and dockable. Rearrange panels, close and reopen, verify layout persisted. Press Ctrl+Q, verify exit. Press Ctrl+Z / Ctrl+Y (with non-empty stacks via Help > About then undo), verify menu state changes. |
| **Headless unit test (CI)** | Build with `BUDDD_HAS_DISPLAY=OFF`. Run `buddd_tests`. Verify `[editor]` tagged tests pass (CommandStack logic, Editor lifecycle without display). |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A new undoable editor action can be added by creating a `Command` subclass and calling `command_stack_.execute(...)` in the appropriate handler, without modifying `Command`, `CommandStack`, or engine code. | Developer creates a minimal "HelloCommand" that toggles a bool, adds it to a menu item, verifies undo/redo works. |
| SC-002 | A new dockable panel can be added by creating an `EditorPanel` subclass and calling `editor.add_panel(...)` in `Editor::setup()`, without modifying engine code, CMake files, or the `EditorApp`. | Developer creates a "TestPanel" extending `EditorPanel`, registers in setup(), rebuilds, runs `buddd edit`, sees the new panel dockable in the workspace. |
| SC-003 | No SDL3, OpenGL, or GLM headers are included outside `src/engine/` after all changes. | `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/ src/cmd/apps/` — zero matches. |
| SC-004 | All existing apps (triangle, cube, phong, etc.) continue to build and run unchanged despite the new `App::update()` method. | Build `--preset debug` succeeds; `buddd run triangle` renders correctly. |
| SC-005 | An editor developer can add per-frame logic to the editor without modifying the render loop or engine code — just by adding code to `Editor::update()`. | Developer adds a log statement in `Editor::update()`, rebuilds, runs `buddd edit`, sees the log output each frame. |

## Edge cases

| Case | Expected behavior |
|---|---|
| **Undo with empty stack** | `CommandStack::undo()` returns false. Edit > Undo is disabled (greyed out). Ctrl+Z is a no-op. |
| **Redo with empty stack** | `CommandStack::redo()` returns false. Edit > Redo is disabled. Ctrl+Y / Ctrl+Shift+Z are no-ops. |
| **Undo beyond stack bounds** | After emptying the undo stack via repeated undo, further undo calls return false and are no-ops. |
| **Redo after new command** | Executing a new command clears the redo stack. Previously undone commands are lost. |
| **Ctrl+Z while ImGui captures keyboard** | Shortcut is suppressed. The keystroke passes through to ImGui (e.g., text input receives Ctrl+Z as expected). |
| **First launch — no ini file** | ImGui creates default layout (built-in layout from `DockBuilder` or default window positions). No crash, no error message. |
| **Corrupt ini file** | ImGui silently ignores a corrupt ini file and falls back to default layout. No crash. |
| **Layout file deleted between sessions** | On next launch, ImGui creates default layout (same as first launch). A new ini file is created on shutdown. |
| **Window resize with docked panels** | ImGui's dockspace fills the viewport automatically (`DockSpaceOverViewport`). Docked panels resize proportionally. |
| **Multiple `buddd edit` instances** | Each instance writes/reads its own ini file (at CWD). If both run from the same directory, the last one to close overwrites the ini file. This is acceptable for v1. |
| **Help > About opened twice quickly** | ImGui's `BeginPopupModal` will return false if already open — second open is a no-op. |
| **Ctrl+Q pressed while About popup open** | Modal captures keyboard → `WantCaptureKeyboard` is true → shortcut suppressed. User must dismiss popup first. |
| **max_history clamped to 0 or 1** | Constructor clamps `max_history` to minimum 1. Stack always retains at least the most recent command. |
| **Editor::setup() called after shutdown()** | Editor follows scaffolding lifecycle (setup once, draw N, shutdown once). After shutdown, re-setup is undefined behaviour — not a supported use case. |
| **ImGui not initialized (headless test)** | `Editor::setup()` returns an error. `draw_ui()` is no-op (guarded by `initialized_`). CommandStack operations are independent of ImGui and can be tested headlessly. |

## Error cases

| Case | Expected behavior |
|---|---|
| `CommandStack` constructed with `max_history = 0` | Clamped to 1 (minimum). The stack always remembers at least the most recent command. |
| `Command::execute()` throws an exception | Not expected. Commands should not throw. If a command fails, it stores an error state and `execute()` returns void (failure is communicated via other means). Future versions may add `Result<void>` return type. |
| `buddd_editor.ini` file is read-only | ImGui logs a warning internally (via `ImGui::ErrorLog`) and continues with default layout. No crash. |
| Disk full during ini file write | ImGui internally handles write failure (error logged, file may be truncated or left in inconsistent state). No crash. |

## Permissions and security

- The editor writes a layout file (`buddd_editor.ini`) to the current working directory. This contains no sensitive data — only ImGui docking state (window positions, sizes, docking splits).
- No elevated privileges are required.
- No network access is required.
- No secrets, credentials, or environment variables are consumed.
- Architecture boundary (ADR-019) is preserved: no SDL3/OpenGL/GLM headers outside `src/engine/`.

## Observability

| Signal | Source |
|---|---|
| Editor ini file path | `ImGui::GetIO().IniFilename` set in `Editor::setup()`. Log via `BUDDD_LOG_INFO("Editor: layout file: {}", ...)`. |
| Menu action executed | Log `BUDDD_LOG_DEBUG("Editor: executing command: {}", cmd->name())` for each command execution. |
| Undo/Redo performed | Log `BUDDD_LOG_DEBUG("Editor: undo '{}'", name)` / `"Editor: redo '{}'", name`. |
| About dialog opened | Log `BUDDD_LOG_DEBUG("Editor: showing About dialog")`. |
| Shortcut suppressed by ImGui capture | Log `BUDDD_LOG_TRACE("Editor: shortcut suppressed (ImGui captures keyboard)")`. |
| Ini file loaded | ImGui internally logs ini file info. If missing, default layout used (silent). |

## Out of scope

- Functional panel content (entity list in Scene, property editors, console log, file browser, asset thumbnails).
- Scene viewport / 3D rendering inside ImGui.
- Gizmos, entity selection, in-viewport interaction.
- Undo/redo of actual editor operations beyond the menu actions themselves.
- Plugin or dynamic panel registration.
- Preferences system, theme, colour scheme.
- Multi-viewport ImGui (ImGuiConfigFlags_ViewportsEnable).
- Non-rectangular panel shapes or custom panel decorations.
- Panel close/hide state persistence (panels are always shown on launch; closing a panel only hides it for that session).
- Input filtering for mouse capture (deferred to future work — only keyboard capture is handled).
- Text input fields in placeholder panels (panels are empty).
- Help menu items beyond About.
- Localisation/i18n.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | The `App` base class gains one new virtual `update()` method with a default empty implementation. `run_app()` adds one call to `app.update(ctx)` after `world->update_updatables()`. All 14 existing App subclasses are unaffected (they don't override `update()`). |
| A-02 | `ImGui::GetIO().WantCaptureKeyboard` is accurate and reliable. When an ImGui modal or text input is active, this flag is true. |
| A-03 | The engine `InputSystem` provides `is_down(KeyCode::ControlLeft)` (or `ControlRight`) and `is_down(KeyCode::Z)`, etc. Both left and right modifier keys are checked. |
| A-04 | ImGui docking branch features (`DockSpaceOverViewport`, `BeginMenuBar`, `BeginMenu`, `MenuItem`, `BeginPopupModal`) are available (engine fetches `v1.91.8-docking`). |
| A-05 | The ini file is named `buddd_editor.ini` and written to the current working directory. The path is set via `ImGui::GetIO().IniFilename` in `Editor::setup()`. |
| A-06 | `buddd::engine::version()` returns a non-empty string view containing the version (e.g., `"0.1.0"` or `"0.1.0-dirty"`). |
| A-07 | The editor's `draw_ui()` is called once per frame after the engine's `ImGui::NewFrame()`. The menu bar is rendered inside the dockspace viewport (not as a separate window). |
| A-08 | `Command` subclasses are trivially copyable/movable only via `unique_ptr`. Commands are polymorphic and heap-allocated. |
| A-09 | `ctrl_is_down` is determined by `input_system->is_down(KeyCode::ControlLeft) || input_system->is_down(KeyCode::ControlRight)`. `shift_is_down` is determined similarly for `KeyCode::ShiftLeft / ShiftRight`. |
| A-10 | The `src/editor/commands/` directory is created as a subdirectory of `src/editor/`. New files are added to `CMakeLists.txt` explicitly (not via glob). |
| A-11 | Panel default layout: center for Scene, right dock for Properties, bottom dock for Console, bottom-left for Project, bottom-right for Assets. Actual initial arrangement is set via `ImGui::DockBuilder` calls in `setup()` or first-frame `draw_ui()`. |
| A-12 | Undo and Redo are invoked directly via `CommandStack::undo()` / `CommandStack::redo()`, not through Command subclasses. They are not undoable themselves. |

## Open questions

| ID | Question | Resolution |
|---|---|---|---|
| Q-01 | Should the ini file be named `buddd_editor.ini` or just `imgui.ini`? | **`buddd_editor.ini`** — specific to the editor, avoids conflicts with other ImGui tools. |
| Q-02 | Should `UndoCommand` and `RedoCommand` be real undoable commands or direct stack calls? | **Direct calls (non-undoable).** Undo/Redo in the menu invoke `CommandStack::undo()` / `CommandStack::redo()` directly, not through the command stack. |
| Q-03 | What should happen to the ini file path in headless mode? | **Skip.** `IniFilename` is only set if `Editor::setup()` succeeds (which fails in headless). No change needed — `draw_ui()` is already a no-op when not initialized. |
| Q-04 | Should panels have a minimum size constraint? | **Yes.** Minimum size ~100x100 via `ImGui::SetNextWindowSizeConstraints()` for each placeholder panel. |
| Q-05 | Should Quit show a confirmation dialog? | **No.** Quit immediately — no confirmation dialog. Matches modern IDE conventions. |
| Q-06 | Should undo/redo menu items show the command name? | **No.** Just "Undo" / "Redo". Simple labels without command names. |
| Q-07 | How to define default panel layout? | **`ImGui::DockBuilder`** — create a polished default layout in `Editor::setup()`: Scene center, Properties right, Console bottom, Project bottom-left, Assets bottom-right. |
| Q-08 | What is the `max_history` default? | **128** — conservative default, prevents unbounded memory growth. |

Priority legend: **Scope** = affects design/capability; **UX** = affects user experience; **Edge case** = affects edge conditions; **Technical** = affects implementation only.

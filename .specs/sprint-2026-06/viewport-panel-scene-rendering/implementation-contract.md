# IMPL-F-07 — Viewport Panel — Scene Rendering

## Source spec

- `.specs/sprint-2026-06/viewport-panel-scene-rendering/spec.md`

## Goal

Implement a dockable `ViewportPanel` that renders the editor's 3D scene into an ImGui window using an FBO-backed render pipeline with a static editor camera. Also rework the default dock layout to the north-star layout (Scene tree left 25%, Viewport center, Properties right 25%, bottom tabs unchanged). The existing `RenderSystem` gains a `render_scene_with_camera()` method that renders any world into any FBO with an explicit view-projection matrix and camera position.

## Non-goals

- No editor camera controls (fly/look/pan/dolly/orbit/zoom) — deferred to follow-up.
- No gizmo, grid overlay, selection highlight, click-to-select, or input routing in the viewport.
- No multi-viewport or split-screen support.
- No play-mode viewport integration.
- No prefab tab viewport.
- No View menu visibility toggle.
- No modifications to `src/engine/scene/` — the editor camera is purely an editor-level concept stored as a struct (no entity or component in any World).
- No modifications to the main loop's `RenderSystem` instance (bound to `ctx.world`). The viewport creates its own `RenderSystem` bound to `editor.world()`.
- No new CMake targets or dependencies.
- No changes to existing `App` subclasses, `App` base class, or `run_app()`.
- No changes to any existing test file — only new test files may be created.

## Relevant ADRs

| ADR | Relevance |
|---|---|
| ADR-019 (Architecture Boundaries) | No SDL3/OpenGL/GLM headers outside `src/engine/`. `src/editor/panels/viewport_panel.*` must include only engine abstractions (`render/render_system.h`, `render/frame_buffer.h`, `math/mat4.h`, `math/vec3.h`). |
| ADR-027 (Editor Architecture) | Editor panels follow `EditorPanel` base class pattern. All editor code in `namespace buddd::editor`. |
| ADR-026 (ImGui Integration) | ImGui frame lifecycle is managed by `RenderDevice::begin_frame()` / `RenderDevice::render_ui()` / `RenderDevice::end_frame()`. Panel uses standard `ImGui::Begin`/`End` inside the dockspace loop. |
| ADR-003 (Render Pipeline) | Draw methods are `void`, not `Result<void>`. The new `render_scene_with_camera()` follows the same pattern. |
| ADR-024 (Camera Transform) | `CameraComponent` exists in engine but the editor camera is NOT a `CameraComponent` — it is a plain struct. `render_scene_with_camera()` bypasses `CameraComponent` lookup entirely. |

## Files to inspect

| File | Reason |
|---|---|
| `src/editor/editor_panel.h` | Base class to inherit — virtual `id()`, `title()`, `update()`, `draw_ui()`. |
| `src/editor/editor_context.h` | `EditorContext` struct — provides `ctx.editor` and `ctx.engine` to panels. |
| `src/engine/engine_context.h` | `EngineContext` — provides `device`, `world`, `render_system`, `delta_time`. |
| `src/engine/render/render_device.h` | Abstract `RenderDevice` — will add `clear()` method. Includes `create_frame_buffer()` signature. |
| `src/engine/render/render_device_opengl.h` | OpenGL backend class declaration — will add `clear()` override. |
| `src/engine/render/render_device_opengl.cpp` | OpenGL backend — `begin_frame()` shows `glClearColor` + `glClear` pattern for FBO clearing reference. |
| `src/engine/render/render_device_headless.h` | Headless backend class declaration — will add `clear()` override (no-op). |
| `src/engine/render/render_device_headless.cpp` | Headless backend — trace logs only. |
| `src/engine/render/render_system.h` | Existing class to extend — signatures of `render()`, `render_scene()`, `render_scene(FrameBuffer&)`. |
| `src/engine/render/render_system.cpp` | Existing implementation — full light collection + MeshRenderer iteration pattern to reuse. |
| `src/engine/render/frame_buffer.h` | FBO abstraction — `bind()`, `unbind()`, `resize()`, `color_texture()`, `width()`, `height()`. |
| `src/engine/render/texture.h` | `gl_handle()` for getting the GL texture ID to pass to `ImGui::Image()`. |
| `src/engine/math/mat4.h` | `Mat4::perspective()`, `Mat4::look_at()` signatures. Both return `Mat4`. |
| `src/engine/math/vec3.h` | `Vec3` struct used for camera position, target, up. |
| `src/editor/editor.h` | `Editor` class — `world()`, `setup()`, `add_panel()`, member declaration order (panels_ before world_). |
| `src/editor/editor.cpp` | Lines 183–190: panel registration. Lines 383–418: existing dock layout code to modify. |
| `src/editor/panels/scene_panel.h` | Reference panel implementation — same base class pattern. |
| `src/editor/CMakeLists.txt` | Uses `GLOB_RECURSE` — no changes needed. |
| `tests/CMakeLists.txt` | Uses `GLOB_RECURSE` with `*_tests.cpp` pattern — new test file is auto-discovered. |
| `tests/editor/editor_tests.cpp` | Reference for test structure (`CmdTestCtx` pattern, Catch2). |

## Files allowed to change

- `src/engine/render/render_device.h` — **modify**: add `virtual auto clear() -> void = 0;`
- `src/engine/render/render_device_opengl.h` — **modify**: declare `auto clear() -> void override;`
- `src/engine/render/render_device_opengl.cpp` — **modify**: implement `clear()` (glClearColor + glClear)
- `src/engine/render/render_device_headless.h` — **modify**: declare `auto clear() -> void override;`
- `src/engine/render/render_device_headless.cpp` — **modify**: implement `clear()` (no-op)
- `src/engine/render/render_system.h` — **modify**: add `render_scene_with_camera()`, add private `render_impl()`
- `src/engine/render/render_system.cpp` — **modify**: implement `render_scene_with_camera()`, refactor `render_scene()` to use `render_impl()`
- `src/editor/editor.cpp` — **modify**: register ViewportPanel, update dock layout to north-star
- `src/editor/panels/viewport_panel.h` — **create**
- `src/editor/panels/viewport_panel.cpp` — **create**
- `tests/editor/viewport_panel_tests.cpp` — **create**

## Files forbidden to change

- `src/engine/scene/` — no changes to any component, entity, or world code.
- `src/cmd/` — no changes to CLI dispatch, app classes, or main loop.
- `src/editor/CMakeLists.txt` — already uses GLOB_RECURSE.
- `src/editor/editor.h` — no changes to the Editor class interface.
- Any existing test file — only new test files.
- Any existing panel `.h`/`.cpp` — no changes to ScenePanel, PropertiesPanel, ConsolePanel, ProjectPanel, AssetsPanel.

## Existing conventions to follow

- **Namespace**: All editor code in `namespace buddd::editor`. Engine code in `namespace buddd::engine`. Math types in `buddd::engine::math`.
- **Panel class**: Inherit `EditorPanel`, implement `id()`, `title()`, `draw_ui()`. Override `update()` only if needed (no-op by default).
- **Naming**: `id()` returns snake_case string (`"viewport"`). `title()` returns display name (`"Viewport"`).
- **Member order in class**: public first, then private. Constructors first.
- **Logging**: Use `BUDDD_LOG_TAG("Editor")` in `.cpp` files for the editor. Use `BUDDD_LOG_TAG("Render")` in render system files.
- **Resource creation**: `Result<std::unique_ptr<T>>` pattern — check error, handle gracefully.
- **No raw `new`/`delete`**: Use `std::make_unique<>` and move semantics.
- **Architecture boundaries**: No `#include <SDL3/...>`, no `#include <GL/...>`, no `#include <glm/...>` in `src/editor/`. The panel must use engine abstractions only.
- **Tests**: Use Catch2 `TEST_CASE` macros. Place editor tests in `tests/editor/` subdirectory (but follow the file naming pattern `*_tests.cpp` for auto-discovery).

## Required implementation behavior

### 1. `RenderDevice::clear()` — new virtual method

Add to `RenderDevice` base class in `render_device.h`:

```cpp
/// Clears the currently bound framebuffer (color + depth buffers)
/// using the engine's default clear color.
/// Behaviour is undefined if no framebuffer is bound.
virtual auto clear() -> void = 0;
```

**OpenGL backend** (`render_device_opengl.h` / `render_device_opengl.cpp`):
```cpp
auto clear() -> void override {
    glClearColor(0.02f, 0.02f, 0.05f, 1.0f);  // match begin_frame() clear color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
```

**Headless backend** (`render_device_headless.h` / `render_device_headless.cpp`):
```cpp
auto clear() -> void override {
    // No-op in headless mode — no GPU framebuffer to clear.
}
```

### 2. `RenderSystem::render_scene_with_camera()` — new public method

Add to `render_system.h`:

```cpp
/// Render the scene using an explicit camera (view-projection matrix and position).
/// Binds the target FBO before rendering, clears it, and unbinds it after.
/// The camera parameters are provided explicitly — no CameraComponent lookup is performed.
/// @param target      The FrameBuffer to render into.
/// @param vp          Combined view-projection matrix (projection * view).
/// @param camera_pos  Camera world-space position (for lighting uniforms).
/// Behaviour is undefined if called from within a render_scene() call.
auto render_scene_with_camera(FrameBuffer& target, math::Mat4 const& vp,
                              math::Vec3 const& camera_pos) -> void;
```

### 3. `RenderSystem::render_impl()` — private helper (extract from `render_scene()`)

Add to `render_system.h` private section:

```cpp
/// Shared implementation: collects lights and iterates MeshRenderers
/// using the given view-projection matrix and camera position.
/// Does NOT bind/unbind any framebuffer — the caller is responsible.
auto render_impl(math::Mat4 const& vp, math::Vec3 const& camera_pos) -> void;
```

**Implementation in `render_system.cpp`:**

- `render_impl()` contains the exact light collection (directional/point/spot) + MeshRenderer iteration currently in `render_scene()` (lines 42–158 of existing `render_system.cpp`), parameterized by `vp` and `camera_pos` instead of looking up a `CameraComponent`.
- `render_scene()` calls `render_impl(vp, camera_pos)` after computing `vp` and `camera_pos` from `world_->active_camera()`. The camera-lookup early-return guard must remain.
- `render_scene(FrameBuffer& target)` stays as-is: `target.bind()` → `render_scene()` → `target.unbind()`.
- `render_scene_with_camera()` does: `target.bind()` → `device_->clear()` → `render_impl(vp, camera_pos)` → `target.unbind()`.

**Critical detail**: The light data array size `detail::k_max_lights` and the `detail::LightData` type are already used in `render_system.cpp` via included headers. The refactored `render_impl()` must include the same headers and uses the same types.

### 4. `ViewportCamera` — private struct in `viewport_panel.h`

Defined in the private section of `ViewportPanel` or as a private struct in the header inside `namespace buddd::editor`:

```cpp
struct ViewportCamera {
    math::Vec3 position{3.0f, 3.0f, 3.0f};
    math::Vec3 target{0.0f, 0.0f, 0.0f};
    math::Vec3 up{0.0f, 1.0f, 0.0f};
    float fov_y = 1.0471975512f;  // 60° in radians (π/3)
    float near_plane = 0.1f;
    float far_plane = 100.0f;

    [[nodiscard]] auto view_projection(float aspect) const noexcept -> math::Mat4 {
        if (aspect <= 0.0f) return math::Mat4::identity();  // guard against zero
        auto proj = math::Mat4::perspective(fov_y, aspect, near_plane, far_plane);
        auto view = math::Mat4::look_at(position, target, up);
        return proj * view;
    }
};
```

Note: use `1.0471975512f` directly (matching `CameraComponent`'s default for `fov_y_`) instead of `std::numbers::pi_v<float>` to avoid `<numbers>` include and stay consistent with engine conventions.

### 5. `ViewportPanel` class — new files

**`viewport_panel.h`:**

```cpp
#pragma once

#include "editor_panel.h"
#include "editor_context.h"

#include "render/render_system.h"
#include "render/frame_buffer.h"
#include "math/mat4.h"
#include "math/vec3.h"

#include <memory>
#include <string_view>

namespace buddd::editor {

class ViewportPanel final : public EditorPanel {
public:
    explicit ViewportPanel(buddd::engine::RenderDevice& device,
                           buddd::engine::World& editor_world);

    [[nodiscard]] auto id() const -> std::string_view override;
    [[nodiscard]] auto title() const -> std::string_view override;
    auto draw_ui(EditorContext const& ctx) -> void override;

private:
    struct ViewportCamera {
        // ... as specified above
    };

    buddd::engine::RenderDevice* device_;
    buddd::engine::World* editor_world_;
    std::unique_ptr<buddd::engine::FrameBuffer> fbo_;
    buddd::engine::RenderSystem render_system_;  // bound to *editor_world_
    ViewportCamera camera_;
    int last_width_ = 0;
    int last_height_ = 0;
};

} // namespace buddd::editor
```

**`viewport_panel.cpp`:**

**Constructor:**
```cpp
ViewportPanel::ViewportPanel(buddd::engine::RenderDevice& device,
                             buddd::engine::World& editor_world)
    : device_(&device)
    , editor_world_(&editor_world)
    , fbo_([&]() -> std::unique_ptr<buddd::engine::FrameBuffer> {
          auto result = device.create_frame_buffer(1, 1);
          if (!result) {
              BUDDD_LOG_ERROR("ViewportPanel: failed to create initial FBO: {}",
                              buddd::engine::to_string(result.error()));
              return nullptr;
          }
          return std::move(*result);
      }())
    , render_system_(device, editor_world)
{
    BUDDD_LOG_DEBUG("ViewportPanel: created (initial FBO 1×1)");
}
```

**`id()`** returns `"viewport"`.
**`title()`** returns `"Viewport"`.

**`draw_ui()` flow:**

```cpp
auto ViewportPanel::draw_ui(EditorContext const& ctx) -> void {
    // 1. Get available content area size
    auto size = ImGui::GetContentRegionAvail();
    int w = static_cast<int>(size.x);
    int h = static_cast<int>(size.y);

    // 2. Guard against zero/negative dimensions (collapsed/minimized panel)
    if (w <= 0 || h <= 0) {
        BUDDD_LOG_TRACE("ViewportPanel: skip render (panel too small: {}×{})", w, h);
        return;
    }

    // 3. Resize FBO if needed
    if (w != last_width_ || h != last_height_) {
        if (fbo_) {
            auto res = fbo_->resize(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
            if (!res) {
                BUDDD_LOG_ERROR("ViewportPanel: FBO resize failed: {}",
                                buddd::engine::to_string(res.error()));
                // Keep using previous FBO at old size — continue rendering
            } else {
                BUDDD_LOG_DEBUG("ViewportPanel: FBO resized {}×{}", w, h);
                last_width_ = w;
                last_height_ = h;
            }
        } else {
            // FBO was null (creation failed) — attempt to recreate
            auto new_fbo = device_->create_frame_buffer(
                static_cast<uint32_t>(w), static_cast<uint32_t>(h));
            if (new_fbo) {
                fbo_ = std::move(*new_fbo);
                last_width_ = w;
                last_height_ = h;
                BUDDD_LOG_DEBUG("ViewportPanel: FBO recreated {}×{} after previous failure", w, h);
            } else {
                BUDDD_LOG_ERROR("ViewportPanel: failed to recreate FBO: {}",
                                buddd::engine::to_string(new_fbo.error()));
                ImGui::Text("Viewport error: failed to create framebuffer");
                return;
            }
        }
    }

    if (!fbo_) {
        ImGui::Text("Viewport error: framebuffer unavailable");
        return;
    }

    // 4. Compute aspect ratio and guard against invalid values
    float aspect = static_cast<float>(last_width_) / static_cast<float>(last_height_);
    if (aspect <= 0.0f) {
        BUDDD_LOG_TRACE("ViewportPanel: skip render (invalid aspect: {})", aspect);
        return;
    }

    // 5. Compute view-projection matrix
    auto vp = camera_.view_projection(aspect);

    // 6. Verify world pointer still valid (may change on Editor::new_scene())
    auto& active_world = ctx.editor.world();
    if (&active_world != editor_world_) {
        // The Editor replaced its World — recreate RenderSystem bound to the new world
        editor_world_ = &active_world;
        render_system_ = buddd::engine::RenderSystem(*device_, *editor_world_);
        BUDDD_LOG_DEBUG("ViewportPanel: re-bound RenderSystem to new editor World");
    }

    // 7. Render into FBO
    BUDDD_LOG_TRACE("ViewportPanel: rendering scene with camera (aspect {})", aspect);
    render_system_.render_scene_with_camera(*fbo_, vp, camera_.position);

    // 8. Display via ImGui::Image
    uint32_t tex_id = fbo_->color_texture().gl_handle();
    ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(tex_id)),
                 ImVec2(static_cast<float>(last_width_), static_cast<float>(last_height_)));
}
```

**Notes on error handling:**
- If `create_frame_buffer()` fails in the constructor, `fbo_` is `nullptr`. Every frame, the panel checks for a null FBO and displays an error text overlay instead of crashing.
- If `resize()` fails (e.g., very large panel), the panel logs the error, keeps the old FBO, and renders at the old size.
- If `gl_handle()` returns 0 (headless mode), `ImGui::Image` renders nothing gracefully — no crash.
- The `reinterpret_cast<ImTextureID>(static_cast<intptr_t>(tex_id))` follows the standard ImGui pattern for converting a GLuint texture ID to ImTextureID.

### 6. `editor.cpp` — panel registration

In `Editor::setup()`, after the existing panel registrations (line 190), add:

```cpp
#include "panels/viewport_panel.h"

// Inside setup(), after the existing panel registrations:
add_panel(std::make_unique<ViewportPanel>(ctx.device, world()));
```

The ViewportPanel is registered after AssetsPanel (the last existing panel). Panel order does not affect functional behavior — each panel is an independent ImGui window.

### 7. `editor.cpp` — dock layout rework

Replace the existing first_layout dock builder block (lines 383–418) with:

```cpp
static bool first_layout = true;
if (first_layout) {
    first_layout = false;

    // Check if a saved layout already exists
    ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspace_id);
    if (node && node->ChildNodes[0] == nullptr && node->ChildNodes[1] == nullptr) {
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

        // North-star layout:
        //
        // ┌────────┬─────────────────┬───────────┐
        // │ Scene  │   Viewport      │ Properties│
        // │ ← 25%  │   ← center →    │ ← 25%    │
        // ├────────┴─────────────────┴───────────┤
        // │ Console│Project│Assets (bottom tabs)  │
        // └──────────────────────────────────────┘

        ImGuiID dock_right;
        ImGuiID dock_main = dockspace_id;

        // 1. Split right 25% for Properties
        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Right, 0.25f, &dock_right, &dock_main);

        // 2. Split left 25% from the remaining center for Scene
        ImGuiID dock_left;
        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Left, 0.25f, &dock_left, &dock_main);

        // 3. Split bottom 25% from the center area
        ImGuiID dock_bottom;
        ImGui::DockBuilderSplitNode(dock_main, ImGuiDir_Down, 0.25f, &dock_bottom, &dock_main);

        // 4. Split bottom area: left half for Console+Project, right half for Assets
        ImGuiID dock_bottom_left;
        ImGuiID dock_bottom_right;
        ImGui::DockBuilderSplitNode(dock_bottom, ImGuiDir_Left, 0.5f, &dock_bottom_left, &dock_bottom_right);

        // Dock windows
        ImGui::DockBuilderDockWindow("Scene", dock_left);
        ImGui::DockBuilderDockWindow("Viewport", dock_main);
        ImGui::DockBuilderDockWindow("Properties", dock_right);
        ImGui::DockBuilderDockWindow("Console", dock_bottom_left);
        ImGui::DockBuilderDockWindow("Project", dock_bottom_left);
        ImGui::DockBuilderDockWindow("Assets", dock_bottom_right);

        ImGui::DockBuilderFinish(dockspace_id);

        BUDDD_LOG_DEBUG("Editor: applied default viewport layout (Scene|Viewport|Properties)");
    }
}
```

The `first_layout` guard and `DockBuilderGetNode` check remain intact to preserve saved layouts from `buddd_editor.ini`.

### 8. No `#include` of forbidden headers

The following files must be verified to NOT contain any of:
- `#include <SDL3/...>`
- `#include <GL/...>`
- `#include <glm/...>` (except through engine wrappers like `<math/mat4.h>`)
- `#include <glad/...>`

Files to verify: `src/editor/panels/viewport_panel.h`, `src/editor/panels/viewport_panel.cpp`.

### 9. World reference handling (new_scene / open_scene)

`Editor::new_scene()` and `Editor::open_scene()` replace `world_` with a new `std::make_unique<World>()`. This invalidates the `World&` stored in the ViewportPanel's `RenderSystem`. To handle this, the `draw_ui()` implementation must compare `&ctx.editor.world()` with the stored `editor_world_` pointer each frame, and recreate the `RenderSystem` if they differ (see step 5 above). This ensures that after `new_scene()` or `open_scene()`, the ViewportPanel transparently rebinds to the new World on the next frame, with no crash or stale data.

## Required tests

### Unit tests (in `tests/editor/viewport_panel_tests.cpp`)

Use Catch2 `TEST_CASE` with appropriate tags `[editor][viewport]`. Test file uses `GLOB_RECURSE` pattern `*_tests.cpp` — auto-discovered by the test CMake.

| # | Test | Tag | Traces to AC |
|---|---|---|---|
| 1 | `ViewportCamera::view_projection()` computes correct matrix for non-zero aspect | `[editor][viewport]` | AC-015, AC-016 |
| 2 | `ViewportCamera::view_projection()` returns identity for zero aspect | `[editor][viewport]` | AC-005, EC aspect guard |
| 3 | `ViewportPanel` constructor creates FBO (1×1) and RenderSystem | `[editor][viewport]` | AC-001, AC-003 |
| 4 | `ViewportPanel::id()` returns `"viewport"`, `title()` returns `"Viewport"` | `[editor][viewport]` | AC-002 |
| 5 | `ViewportPanel::draw_ui()` skips rendering when panel has zero dimensions | `[editor][viewport]` | AC-004 |
| 6 | `RenderSystem::render_scene_with_camera()` exists with correct signature | `[editor][viewport]` | AC-008 |
| 7 | `render_scene_with_camera()` binds FBO, calls clear, renders, unbinds FBO | `[editor][viewport]` | AC-009, AC-018 |
| 8 | Dock layout snapshot test: verify default layout structure matches 3-column spec | `[editor][viewport]` | AC-012 |
| 9 | `ViewportCamera` default values: position=(3,3,3), target=(0,0,0), up=(0,1,0), 60° FOV | `[editor][viewport]` | AC-015, AC-016 |
| 10 | First_layout guard preserves existing saved layouts | `[editor][viewport]` | AC-013 |
| 11 | `RenderDevice::clear()` does not crash (smoke test) | `[engine][render]` | AC-018 |
| 12 | World pointer change in `draw_ui()` triggers RenderSystem recreation | `[editor][viewport]` | Edge case: scene switch |

### Integration / E2E verification

- Build with `cmake --build --preset debug`, run `buddd_tests`, verify all `[editor][viewport]` tests pass.
- Run `buddd edit` manually: verify 3-column layout, dark gray viewport when scene is empty.
- Create an entity via right-click Scene panel → Create Empty → verify viewport shows dark gray (no mesh), then load a scene and verify rendering.
- Resize dock dividers, verify viewport content fills area.
- Verify zero new warnings from `src/editor/` and `src/engine/render/` during build.
- Verify `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/panels/viewport_panel.*` returns zero matches.

## Edge cases

| Case | Expected behavior |
|---|---|
| **Panel collapsed/minimized (zero content area)** | `draw_ui()` checks `w <= 0 \|\| h <= 0`, logs trace, returns. No resize, no render. |
| **Panel extremely small (1×1)** | FBO resize to 1×1 succeeds. Aspect ratio = 1.0. Render produces degenerate output but no crash. |
| **First launch (no saved layout)** | Default north-star layout applied. Logged. |
| **Subsequent launch (saved layout exists)** | `first_layout` guard + `DockBuilderGetNode` check prevents overwriting. |
| **Very large panel (> 4096)** | FBO resize may fail. `resize()` returns error → keep previous FBO, log error. |
| **Editor World has no lights** | Light count = 0. MeshRenderers using lighting shaders render unlit (shader defaults). |
| **FrameBuffer creation fails** | `fbo_` is nullptr. Panel logs error, displays "Viewport error" text in the ImGui window. |
| **Editor scene switch (new_scene)** | `editor_world_` pointer tracked in `draw_ui()`. If world changed, RenderSystem is recreated bound to the new world. |
| **Multiple frames with no dimension change** | `last_width_`/`last_height_` guard prevents unnecessary FBO resize calls. |
| **Headless mode** | `create_frame_buffer(1, 1)` succeeds (headless FBO). `gl_handle()` returns 0. `ImGui::Image` with tex_id=0 renders nothing gracefully. |
| **FBO resize fails mid-session** | Logged as error. Panel keeps previous FBO at old size, continues rendering. |
| **ImGui::Begin returns false (panel hidden)** | The existing `draw_ui()` loop in `editor.cpp` wraps each panel's `draw_ui()` inside `ImGui::Begin()`/`End()`. If `Begin()` returns false because the panel is tabbed behind another, ImGui skips the content. No additional guard needed. |

## Security impact

None. The ViewportPanel reads the editor's World in memory only. No file I/O, no network access, no external resource loading during rendering. The FBO color texture is displayed via ImGui but never written to disk or exposed outside the process.

## Data and migration impact

None. No schema changes, data migrations, or seed data.

## API compatibility impact

- `RenderDevice` gains a new pure virtual method `clear()`. All existing subclasses (`RenderDeviceOpenGL`, `RenderDeviceHeadless`) must override it.
- `RenderSystem` gains `render_scene_with_camera()` and a private `render_impl()`. The existing `render()` and `render_scene()` overloads are unchanged — backward compatible.
- `EditorPanel::draw_ui()` is unchanged — ViewportPanel implements the existing interface.
- No public API removal or signature change.

## Documentation impact

| Document | Change needed |
|---|---|
| `docs/wiki/architecture/module-map.md` | Add `viewport_panel.h`/`.cpp` to concrete dockable panels table. Update `RenderSystem` entry to include `render_scene_with_camera()`. |
| `docs/wiki/editor/editor-panels.md` | Add ViewportPanel section: default position (center), content (3D scene), editor camera description. Update layout diagram to show 3-column north-star layout. |
| `docs/wiki/architecture/data-flow.md` | Update offscreen rendering / FBO section to describe Viewport Panel usage with `render_scene_with_camera()`. |
| `docs/wiki/architecture/dependency-map.md` | Update if needed to reflect new files. |

## ADR impact

No new ADR needed. The implementation follows existing ADRs (019, 026, 027, 003). The `clear()` method addition to `RenderDevice` is a natural extension of the existing abstraction and does not warrant a new ADR.

## Done criteria

- [ ] `ViewportPanel` class exists in `src/editor/panels/viewport_panel.h` with `id()`, `title()`, `draw_ui()` overrides.
- [ ] `ViewportPanel::id()` returns `"viewport"`, `title()` returns `"Viewport"`.
- [ ] `ViewportPanel` constructor creates `FrameBuffer` (1×1 initial) and `RenderSystem` bound to editor's World.
- [ ] `ViewportPanel::draw_ui()` guards against zero/negative dimensions (no resize/render).
- [ ] `ViewportPanel::draw_ui()` resizes FBO when panel dimensions change, computes camera VP, calls `render_scene_with_camera()`, displays via `ImGui::Image()`.
- [ ] `ViewportPanel::draw_ui()` handles FBO creation failure gracefully (displays error text).
- [ ] `ViewportPanel::draw_ui()` detects World pointer changes and recreates RenderSystem as needed.
- [ ] `ViewportCamera` struct defined with correct defaults: position (3,3,3), target (0,0,0), up (0,1,0), 60° FOV, 0.1 near, 100 far.
- [ ] `ViewportCamera::view_projection()` returns identity for aspect ≤ 0.
- [ ] `RenderSystem::render_scene_with_camera()` exists with signature `(FrameBuffer& target, math::Mat4 const& vp, math::Vec3 const& camera_pos) -> void`.
- [ ] `render_scene_with_camera()` binds FBO, calls `device_->clear()`, iterates MeshRenderers using provided VP and camera_pos, unbinds FBO.
- [ ] `RenderSystem::render_impl()` extracted as private helper, used by both `render_scene()` and `render_scene_with_camera()`.
- [ ] `RenderDevice::clear()` added as pure virtual, implemented in OpenGL (glClearColor + glClear) and Headless (no-op) backends.
- [ ] `Editor::setup()` registers `ViewportPanel` after existing panels.
- [ ] Default dock layout reworked to north-star: Scene left 25%, Viewport center, Properties right 25%, bottom tabs.
- [ ] First_layout guard and `DockBuilderGetNode` check preserve saved layouts.
- [ ] Build with `cmake --build --preset debug` produces zero new warnings from `src/editor/` and `src/engine/render/`.
- [ ] `grep -rnE '#include.*(SDL3|GL/|glm/)' src/editor/panels/viewport_panel.*` returns zero matches.
- [ ] All existing unit tests still pass (`buddd_tests`).
- [ ] New test file `tests/editor/viewport_panel_tests.cpp` with tests for all ACs listed above.

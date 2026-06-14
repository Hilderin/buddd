# SPEC-F-07 — Viewport Panel — Scene Rendering

## Problem

The editor currently has five dockable panels (Scene, Properties, Console, Project, Assets), but the central area — which should display the 3D scene — is occupied by the Scene Panel (entity tree). Users have no visual feedback when they create, move, or modify entities. The layout does not match the north-star vision (Scene tree left, 3D Viewport center, Inspector right). There is no editor camera independent of scene cameras, no way to see the editor's World rendered into an ImGui panel, and no infrastructure for rendering the editor scene into a framebuffer for ImGui display.

## Goals

| ID | Goal |
|---|---|
| G-01 | **ViewportPanel**: New dockable ImGui panel that renders the editor's 3D scene using a persistent editor camera. Positioned in the center dock slot. |
| G-02 | **Layout rework**: Restructure the default dock layout to match north-star (Scene 25% left, Viewport center, Properties/Inspector 25% right, bottom row unchanged). |
| G-03 | **Editor camera**: Persistent camera owned by the viewport panel (not a CameraComponent in the World), positioned at (3, 3, 3) looking at origin, Y-up. 60° FOV, 16:9 aspect, 0.1 near, 100 far. |
| G-04 | **FBO-backed rendering**: Per-panel FrameBuffer that auto-resizes to match panel content area size. Rendered into `ImGui::Image()` each frame. |
| G-05 | **RenderSystem::render_scene_with_camera()**: New explicit API on the existing RenderSystem accepting a FrameBuffer, view-projection matrix, and camera position — no mutable state leak, no override that can mutate the RenderSystem's internal world reference. |
| G-06 | **Two renders per frame pattern**: Main loop's `render_scene()` renders `ctx.world` (empty → early return), then the ViewportPanel renders `editor.world()` into its own FBO during `draw_ui()`. |
| G-07 | **Non-regression**: All existing tests pass. Zero new warnings from `src/editor/` and `tests/`. |

## Non-goals

| # | Exclusion |
|---|---|
| NG-01 | **No editor camera controls** — camera look/fly/pan/dolly (right-click + WASD, scroll, middle-click) are deferred to a follow-up feature. The camera is static at (3, 3, 3) looking at origin. |
| NG-02 | **No gizmo** — translate/rotate/scale gizmo rendering is deferred. |
| NG-03 | **No grid overlay** — ground-plane grid rendering is deferred. |
| NG-04 | **No selection highlight** — wireframe outline or bounding box on selected entities is deferred. |
| NG-05 | **No play-mode integration** — viewport behavior during Play mode (camera mode switch, read-only) is deferred. |
| NG-06 | **No multi-viewport** — only one viewport panel is supported. Split-screen or additional viewports are deferred. |
| NG-07 | **No prefab tab viewport** — prefab tab viewport rendering is deferred until tab system is implemented. |
| NG-08 | **No camera orbit on entity select** — pressing F to focus on entity is deferred. |
| NG-09 | **No ImGui input routing** — viewport click-to-select, drag-to-select box, or input routing to the editor camera are deferred. |
| NG-10 | **No View menu toggle** — the View menu does not yet have a Viewport visibility toggle. |
| NG-11 | **No changes to `src/engine/scene/`** — the editor camera is an editor-level concept, not added to the engine's scene or entity system. |
| NG-12 | **No modifications to the main loop's `RenderSystem`** — the viewport creates its own `RenderSystem` bound to `editor.world()`. The existing `RenderSystem` (bound to `ctx.world`) is untouched. |

## Actors

| Actor | Description |
|---|---|
| **Editor user** | Opens the editor, sees the 3-panel layout (Scene tree left, Viewport center, Properties right). Views the 3D scene rendered in the center panel. Modifies the scene (creates entities, edits transforms) and sees changes reflected in the viewport in real-time. |
| **ViewportPanel developer** | Implements the viewport panel. Creates and manages a FrameBuffer, a ViewportCamera, and a dedicated RenderSystem. Calls `render_scene_with_camera()` each frame. Handles FBO resize on panel resize. |
| **RenderSystem consumer** | Editor code that calls the new `render_scene_with_camera(FrameBuffer&, Mat4 const& vp, Vec3 const& camera_pos)` method to render any world into any FBO with an explicit camera (bypassing scene CameraComponent lookup). |

## User-visible behavior

### Layout change

The default dock layout changes from the current layout:

```
Before (current):
┌──────────────────────────────────┐
│              Scene               │ ← 100% width
├────────────────┬─────────────────┤
│ Console+Project│     Assets      │
└────────────────┴─────────────────┘
Properties is docked in right 25% but the layout has no Viewport.
```

To the north-star layout:

```
After (F-07):
┌────────┬─────────────────┬───────────┐
│ Scene  │   Viewport      │ Properties│
│ (tree) │   (3D scene)    │           │
│ ← 25%  │   ← center →    │ ← 25%    │
├────────┴─────────────────┴───────────┤
│ Console│Project│Assets (bottom tabs)  │
└──────────────────────────────────────┘
```

**Layout details:**
1. Split the dockspace into three vertical columns: left 25% (Scene), center (Viewport), right 25% (Properties/Inspector).
2. Below all three columns, split bottom 25% height for the bottom tabs (Console, Project, Assets).
3. The bottom area is tabbed: Console + Project in a shared tab group, Assets in its own.
4. If a saved `buddd_editor.ini` layout exists, it is preserved (the default layout only applies on first launch).

### ViewportPanel rendering pipeline (per frame)

For each frame, during `ViewportPanel::draw_ui()`:

1. Begin ImGui window (`"Viewport"`).
2. Get available content area size via `ImGui::GetContentRegionAvail()`.
3. If size changed (or first frame), call `fbo_->resize(width, height)`. Guard against zero dimensions (collapsed panel).
4. Compute view-projection matrix from editor camera:
   - `Mat4 projection = Mat4::perspective(60° * π/180, aspect, 0.1f, 100.f)`
   - `Mat4 view = Mat4::look_at(eye_{3,3,3}, center_{0,0,0}, up_{0,1,0})`
   - `Mat4 vp = projection * view`
5. Call `viewport_render_system_->render_scene_with_camera(*fbo_, vp, camera_pos_)`.
6. Get the color texture's GL handle: `uint32_t tex_id = fbo_->color_texture().gl_handle()`.
7. Display in ImGui: `ImGui::Image((ImTextureID)(intptr_t)tex_id, size)`.
8. End ImGui window.

### Editor camera state

The editor camera is a simple struct stored privately in the ViewportPanel:

```cpp
struct ViewportCamera {
    math::Vec3 position{3.0f, 3.0f, 3.0f};
    math::Vec3 target{0.0f, 0.0f, 0.0f};
    math::Vec3 up{0.0f, 1.0f, 0.0f};
    float fov_y = 60.0f * (std::numbers::pi_v<float> / 180.0f);  // 60° in radians
    float near_plane = 0.1f;
    float far_plane = 100.0f;
};
```

- The camera is **not** an entity in any World. It is purely a set of projection/view parameters.
- The camera is initialized with the above defaults.
- `view_projection()` computes `Mat4::perspective(fov_y, aspect, near, far) * Mat4::look_at(position, target, up)`.
- `aspect` is `width / (float)height` of the FBO (i.e., panel content area). Guard against height == 0 → skip.

### RenderSystem new API

A new public method added to the existing `RenderSystem` class:

```cpp
/// Render the scene using an explicit camera (view-projection matrix and position).
/// The camera parameters are provided explicitly — no CameraComponent lookup is performed.
/// Binds the target FBO before rendering and unbinds it after.
/// @param target      The FrameBuffer to render into.
/// @param vp          Combined view-projection matrix (projection * view).
/// @param camera_pos  Camera world-space position (for lighting calculations).
/// Behaviour is undefined if called from within a render_scene() call.
/// Behaviour is undefined if target is nullptr.
auto render_scene_with_camera(FrameBuffer& target, math::Mat4 const& vp,
                              math::Vec3 const& camera_pos) -> void;
```

**Behaviour:**
1. Binds `target` (FBO).
2. Clears the FBO (color + depth).
3. Iterates MeshRenderers in `world_` (the RenderSystem's bound world).
4. For each MeshRenderer: renders using `vp` for the MVP matrix and `camera_pos` for lighting uniforms (`u_camera_pos`).
5. Collects lights from the world as the existing `render_scene()` does.
6. Unbinds `target`.

This method does **not** look up an active camera from the World. It uses the provided VP matrix and camera position directly. This is the key difference from `render_scene()`.

### Panel resizing

- Each frame, the panel checks if content area size differs from the FBO size.
- If different (or FBO not yet created), `fbo_->resize(width, height)` is called.
- Zero dimensions (collapsed/minimized panel) are guarded — FBO is not resized, and rendering is skipped.
- The FBO is created once with initial size (1, 1) in the constructor and resized as needed.
- `resize()` on `FrameBuffer` destroys old attachments and creates new ones at the specified size (existing API).

### Empty world rendering

If `editor.world()` has no MeshRenderers (empty scene), the render clears the FBO to the clear color (dark gray, matching the engine's default clear color). ImGui displays the cleared texture as a dark gray rectangle.

### Two renders per frame

The main loop in `app.cpp` calls `render_system->render_scene()` (step 5 in the loop). Since `ctx.world` (the engine's demo world) is empty or has no active camera, this call either:
- Renders nothing (no active camera → early return with trace log).
- Or renders the engine's demo scene if one is loaded (unlikely in editor mode).

The ViewportPanel's rendering happens later in the same frame during `draw_ui()` (step 6 in the loop, after `render_scene()`), using its own `RenderSystem` bound to `editor.world()`.

This means two renders happen per frame:
1. `ctx.render_system.render_scene()` — renders `ctx.world` (empty → early return).
2. `viewport_render_system_->render_scene_with_camera(fbo, vp, pos)` — renders `editor.world()` into the viewport FBO.

Both renders occur within the same `begin_frame()` / `end_frame()` pair, which is valid because FBO bind/unbind switches the render target.

## Key entities

### ViewportCamera (private struct in ViewportPanel)

```cpp
namespace buddd::editor {

struct ViewportCamera {
    math::Vec3 position{3.0f, 3.0f, 3.0f};
    math::Vec3 target{0.0f, 0.0f, 0.0f};
    math::Vec3 up{0.0f, 1.0f, 0.0f};
    float fov_y = glm::radians(60.0f);
    float near_plane = 0.1f;
    float far_plane = 100.0f;

    [[nodiscard]] auto view_projection(float aspect) const noexcept -> math::Mat4 {
        if (aspect <= 0.0f) return math::Mat4::identity();  // guard against zero
        auto proj = math::Mat4::perspective(fov_y, aspect, near_plane, far_plane);
        auto view = math::Mat4::look_at(position, target, up);
        return proj * view;
    }
};

} // namespace buddd::editor
```

### ViewportPanel class

```cpp
namespace buddd::editor {

/// Dockable panel that renders the editor's 3D scene in an ImGui window.
class ViewportPanel final : public EditorPanel {
public:
    explicit ViewportPanel(buddd::engine::RenderDevice& device,
                           buddd::engine::World& editor_world);

    [[nodiscard]] auto id() const -> std::string_view override;
    [[nodiscard]] auto title() const -> std::string_view override;
    auto draw_ui(EditorContext const& ctx) -> void override;

private:
    std::unique_ptr<buddd::engine::FrameBuffer> fbo_;
    buddd::engine::RenderSystem render_system_;  // bound to editor.world()
    ViewportCamera camera_;
    int last_width_ = 0;
    int last_height_ = 0;
};

} // namespace buddd::editor
```

### RenderSystem addition

```cpp
// Added to src/engine/render/render_system.h:

/// Render the scene using an explicit camera (view-projection matrix and position).
/// Binds the target FBO before rendering and unbinds it after.
/// @param target      The FrameBuffer to render into.
/// @param vp          Combined view-projection matrix (projection * view).
/// @param camera_pos  Camera world-space position (for lighting uniforms).
auto render_scene_with_camera(FrameBuffer& target, math::Mat4 const& vp,
                              math::Vec3 const& camera_pos) -> void;
```

## Interface Changes

**New files:**
- `src/editor/panels/viewport_panel.h` — `ViewportPanel` class declaration (private `ViewportCamera` struct, `EditorPanel` override, FBO and RenderSystem ownership).
- `src/editor/panels/viewport_panel.cpp` — `ViewportPanel` implementation: constructor creates FBO (1×1 initial) + RenderSystem, `draw_ui()` handles resize, camera computation, rendering, and ImGui display.

**Modified files:**
- `src/engine/render/render_system.h` — Add `render_scene_with_camera(FrameBuffer&, Mat4 const& vp, Vec3 const& camera_pos)` declaration.
- `src/engine/render/render_system.cpp` — Implement `render_scene_with_camera()`: bind FBO, clear, iterate MeshRenderers using explicit VP, unbind FBO.
- `src/editor/editor.cpp` — Register `ViewportPanel`, update dock layout (left 25% Scene, center Viewport, right 25% Properties, bottom unchanged).
- `src/editor/CMakeLists.txt` — No explicit change needed (already uses `GLOB_RECURSE` which picks up new `.cpp` files automatically).

**New test files:**
- `tests/editor/viewport_panel_tests.cpp` — Headless tests:
  - `render_scene_with_camera()` smoke test (basic rendering into FBO).
  - `ViewportCamera::view_projection()` basic computation (non-zero aspect, identity for zero aspect).
  - `ViewportPanel` layout change verification (snapshot-based test).
  - FBO creation and resize (reuses existing FBO test infrastructure).

## User stories

### Story 1 — Layout rework: Scene left, Viewport center, Properties right (Priority: P1)

As an editor user opening the editor for the first time, I want to see the 3-panel layout (Scene tree left, Viewport center, Properties/Inspector right) so that I can immediately understand the editor's spatial layout.

**Given** the editor has no saved layout (`buddd_editor.ini` does not exist or has no dockspace settings)
**When** I launch the editor with `buddd edit`
**Then** the default dock layout shows three vertical columns:
- Left 25%: Scene panel
- Center: Viewport panel (rendering the 3D scene)
- Right 25%: Properties panel
**And** the bottom 25% area shows Console/Project/Assets tabs
**And** no ImGui errors or dock setup failures occur

**Given** the editor has a saved custom layout (from a previous session)
**When** I launch the editor
**Then** the saved layout is preserved (not overridden by the default)
**And** the Viewport panel is visible in its previously docked position

### Story 2 — Viewport renders the editor scene (Priority: P1)

As an editor user, I want to see the editor's 3D scene rendered in the center viewport panel so that I can visually inspect my scene.

**Given** the editor is running with a scene that has entities with MeshRenderer components
**When** I look at the Viewport panel
**Then** I see the 3D scene rendered from the editor camera's perspective (position (3,3,3), looking at origin)
**And** the rendering updates every frame

**Given** the editor scene is empty (no entities)
**When** I look at the Viewport panel
**Then** I see a dark gray rectangle (clear color) — no entities rendered
**And** no rendering errors occur

### Story 3 — Viewport auto-resizes with panel (Priority: P1)

As an editor user resizing the viewport dock divider, I want the 3D render to fill the available panel space without distortion or error.

**Given** the Viewport panel is visible at 800×600 pixels
**When** I drag the left divider to make the Viewport wider (e.g., 1000×600)
**Then** the FBO is resized to match the new width
**And** the rendered image fills the new panel area with correct aspect ratio
**And** no visual artifacts or tearing occur

**Given** the Viewport panel is resized to very small dimensions (e.g., 10×10)
**When** the panel content area is smaller than 1×1
**Then** rendering is skipped for that frame (no resize or render call)
**And** no crash, no OpenGL error, no buffer overflow

### Story 4 — Transform edits reflect in viewport in real-time (Priority: P2)

As an editor user editing entity transforms in the Properties panel, I want the Viewport to update immediately so that I can see the visual result of my edits.

**Given** a scene with a visible entity (MeshRenderer with a model)
**When** I edit the entity's Position X from 0 to 5 in the Properties Panel
**Then** the Viewport shows the entity at its new position on the next frame
**And** the render is correct (no stale FBO content, no flicker)

**Given** I create a new entity with a MeshRenderer
**When** the entity appears in the Scene Panel and is rendered
**Then** the Viewport shows the new entity in the next frame
**And** the Viewport updates without requiring a manual refresh

### Story 5 — Default editor camera rendered correctly (Priority: P2)

As an editor user, I want the default camera position to give me a usable overview of the scene from a 3/4 angle.

**Given** a scene with an entity at the origin (0,0,0)
**When** the Viewport renders
**Then** the camera is at position (3, 3, 3) looking at (0, 0, 0)
**And** the entity is visible and centered in the viewport
**And** the Y-up orientation is correct (Y axis points upward)

## Acceptance criteria

| ID | Description | Verification |
|---|---|---|
| AC-001 | `ViewportPanel` class exists in `src/editor/panels/viewport_panel.h` with `id()`, `title()`, `draw_ui()` overrides, and a constructor taking `RenderDevice&` and `World&`. | Unit test: compile check, instantiation test. |
| AC-002 | `ViewportPanel::id()` returns `"viewport"`, `title()` returns `"Viewport"`. | Unit test: verify returned string_views. |
| AC-003 | `ViewportPanel` creates a `FrameBuffer` with initial size (1, 1) in constructor. FBO is resized to match panel content available size each frame. | Code review + test: verify FBO is created, resize is called when dimensions change. |
| AC-004 | `ViewportPanel::draw_ui()` guards against zero or negative content area dimensions (does not resize/render when width or height ≤ 0). | Test: simulate collapsed panel (0 width), verify no FBO resize or render call. |
| AC-005 | `ViewportPanel::draw_ui()` computes aspect ratio from FBO dimensions. If aspect ≤ 0, rendering is skipped. | Test: verify aspect guard. |
| AC-006 | `ViewportPanel::draw_ui()` calls `render_scene_with_camera(fbo, vp, camera_pos)` when dimensions are valid. | Integration test: set up panel with mocked world, verify render method is called. |
| AC-007 | `ViewportPanel::draw_ui()` displays the FBO's color texture via `ImGui::Image()`. | Snapshot test: verify `ImGui::Image` is called with correct texture ID. |
| AC-008 | `render_scene_with_camera(FrameBuffer&, Mat4 const&, Vec3 const&)` exists on `RenderSystem` with the specified signature. | Unit test: compile check. |
| AC-009 | `render_scene_with_camera()` binds the target FBO, renders the scene using the provided VP matrix (not looking up a CameraComponent), and unbinds the FBO. | Unit test: verify bind/unbind calls on a mock FBO. |
| AC-010 | `render_scene_with_camera()` passes `camera_pos` to material uniforms (`u_camera_pos`), matching the existing `render_scene()` behaviour. | Code review + integration test: render known scene, verify uniform is set with the provided position. |
| AC-011 | `render_scene_with_camera()` uses `world_` (the RenderSystem's bound world) — not `ctx.world`. | Code review: verify no reference to external world. |
| AC-012 | The default dock layout splits the dockspace into left 25% (Scene), center (Viewport), right 25% (Properties), with bottom 25% tabbed area (Console/Project/Assets). | Snapshot test (headless): verify dock builder configuration matches expected structure. |
| AC-013 | The layout change only applies on first launch (no saved layout exists). Saved layouts from `buddd_editor.ini` are preserved. | Code review: verify the `first_layout` guard and `DockBuilderGetNode` check remain intact. |
| AC-014 | `ViewportPanel` is registered in `Editor::setup()` via `add_panel()` and appears in the panels list. | Integration test: verify `Editor` panels include `ViewportPanel`. |
| AC-015 | The editor camera starts at position (3, 3, 3) looking at (0, 0, 0) with Y-up. | Unit test: verify `ViewportCamera` default values. |
| AC-016 | The editor camera uses 60° FOV, 0.1 near plane, 100 far plane. | Unit test: verify `ViewportCamera` projection defaults. |
| AC-017 | The Viewport's `RenderSystem` is bound to `editor.world()` (not `ctx.world`). | Code review: verify the RenderSystem is created with `device` and `editor.world()`. |
| AC-018 | `render_scene_with_camera()` clears the FBO before rendering (color + depth). | Integration test: render empty world, verify FBO content is clear color. |
| AC-019 | The Viewport panel renders correctly when the editor has an empty world (no entities). | Manual: launch editor, see dark gray rectangle in center (no crash). |
| AC-020 | Creating an entity with a MeshRenderer in the editor updates the Viewport on the next frame. | Manual: launch editor, create entity with model, see it appear in viewport. |
| AC-021 | Changing an entity's transform updates the Viewport immediately. | Manual: edit position in Properties Panel, see entity move in viewport. |
| AC-022 | All existing tests still pass. | Run `buddd_tests`. |
| AC-023 | Zero new warnings from `src/editor/` and `tests/`. | Build with `cmake --build --preset debug` and verify zero warnings. |
| AC-024 | No SDL3, OpenGL, or GLM headers are included in `src/editor/panels/viewport_panel.h` or `.cpp`. | Code review: verify architecture boundary (ADR-019) is respected. The panel uses only engine abstractions. |

## E2E Verification

| Method | Description |
|---|---|
| **Headless unit test (CI)** | Build with `BUDDD_HAS_DISPLAY=OFF`. Run `buddd_tests`. Verify `[editor][viewport]` tagged tests pass — FBO creation, `render_scene_with_camera()` lifecycle, `ViewportCamera::view_projection()`, dock layout snapshot. |
| **Manual smoke test (display)** | Run `buddd edit`. Verify 3-column layout (Scene left, Viewport center, Properties right). Verify Viewport shows dark gray rectangle when scene is empty. Create a cube entity (or load a scene with models) and verify it renders in the Viewport. Resize dividers and verify Viewport content area fills the space. |
| **Clean build verification (CI)** | Run `cmake --build --preset debug` and verify zero warnings from `src/editor/` and `src/engine/render/`. |
| **Architecture boundary check** | Verify that no `#include <SDL3/...>`, `#include <glm/...>`, or `#include <glad/...>` appears in `src/editor/panels/viewport_panel.*` or `src/editor/editor.cpp` additions. |

## Success criteria

| ID | Metric | Verification |
|---|---|---|
| SC-001 | A user opening the editor sees the Scene tree left, 3D Viewport center, Properties right layout. | Manual: launch editor, observe layout. |
| SC-002 | A user can see the editor's 3D scene rendered in the Viewport panel at all times. | Manual: load/create a scene, observe rendered view in center panel. |
| SC-003 | The Viewport rendering updates every frame and reflects entity changes (create, move, delete) without manual refresh. | Manual: create entity → appears; move entity → moves in viewport. |
| SC-004 | Resizing the Viewport panel immediately adjusts the rendered image to fill the new area without artifacts. | Manual: drag dividers, observe correct resize. |
| SC-005 | The editor camera provides a usable default view of the scene from (3, 3, 3) looking at origin. | Manual: load a scene with objects at origin, verify they are visible and centred. |
| SC-006 | The editor World renders independently from the engine's demo World (`ctx.world`). Changes to `ctx.world` do not affect the Viewport. | Automated: set up test with two worlds, verify Viewport renders only editor.world(). |

## Edge cases

| Case | Expected behaviour |
|---|---|
| **Panel collapsed/minimized (zero content area)** | `draw_ui()` is still called by ImGui but `ImGui::GetContentRegionAvail()` returns (0,0) or negative. The panel skips FBO resize and rendering. No crash. |
| **Panel extremely small (1×1 pixels)** | FBO resize to 1×1 succeeds. Aspect ratio is 1.0. Rendering may produce degenerate output but no crash. The FBO's color texture exists and can be displayed. |
| **First launch (no saved layout)** | Default layout is applied with Scene left 25%, Viewport center, Properties right 25%, bottom row. |
| **Subsequent launch (saved layout exists)** | Saved layout from `buddd_editor.ini` is loaded. The `first_layout` guard prevents overwriting. Viewport panel appears where previously docked. |
| **Very large panel (> 4096 px)** | FBO resize may fail if the GPU cannot allocate a texture of that size. `create_frame_buffer()` returns `Error::ResourceCreationFailed`. The panel should handle this gracefully (keep previous FBO, log warning). |
| **Editor World has no lights** | Lighting calculations proceed with zero lights. MeshRenderers using lighting shaders may render as unlit (pure ambient, controlled by shader defaults). No crash. |
| **Editor World has no active camera (expected — editor camera is separate)** | `render_scene_with_camera()` does not look up a camera — it uses the provided VP directly. No issue. |
| **FrameBuffer creation failure** | If `create_frame_buffer()` fails (e.g., out of memory), the panel should handle the error gracefully: log error, show a placeholder text in the viewport, and not crash. |
| **Editor scene switch (new_scene)** | `new_scene()` replaces `editor.world()` content. The ViewportPanel's `RenderSystem` retains a pointer to `World&`, which remains valid (the World object itself is the same, only its content changes). The next render will render the new scene. |
| **ImGui::Image() with zero-size texture** | Not possible because FBO resize is guarded. Even at 1×1, the texture exists. |
| **Multiple frames with no dimension change** | FBO resize is skipped if `last_width_ == width && last_height_ == height`. No unnecessary GPU work. |
| **Panel hidden by ImGui docking (tabbed behind another panel)** | ImGui does not call `draw_ui()` for hidden tabs. No rendering occurs. No resource waste. |

## Error cases

| Case | Expected behaviour |
|---|---|
| **FrameBuffer creation fails** | `Result` error is logged (`BUDDD_LOG_ERROR`). The panel displays an error text overlay ("Viewport error: failed to create framebuffer") and does not render. No crash. |
| **FrameBuffer resize fails** | Logged as error. The panel keeps the previous FBO at the old size and continues rendering. |
| **RenderDevice invalid (null/dangling)** | Not possible — the panel receives `RenderDevice&` from engine context which is always valid during editor lifetime. If it becomes invalid, the engine is in an undefined state. |
| **Editor World is destroyed before the panel** | Not possible — the Editor owns both the World and the panels. The World outlives all panels. |
| **GL error during render_scene_with_camera()** | OpenGL errors are not handled at this level (consistent with existing `render_scene()` behaviour). Future work: add `GL_KHR_debug` callback. |
| **Out of memory during FBO creation** | `create_frame_buffer()` returns `ResourceCreationFailed`. Handled as per the FBO failure case above. |
| **ImGui frame not active (Begin returns false)** | `ImGui::Begin("Viewport")` returns false if the panel is collapsed or hidden. Guard ensures no rendering work is done for invisible panels. |

## Permissions and security

- No changes to permissions or security posture.
- The Viewport panel reads the editor's World in-memory only. No file I/O is performed during rendering.
- The FBO color texture is displayed via ImGui but is never written to disk or exposed outside the process.
- No authentication or authorization boundaries are crossed.
- The Viewport does not load any external resources during rendering (models, textures, shaders are already loaded by the World's asset system).

## Observability

| Signal | Source |
|---|---|
| **ViewportPanel created** | Debug-level log: `BUDDD_LOG_DEBUG("ViewportPanel: created (initial FBO 1×1)")` in constructor. |
| **FBO resize** | Debug-level log: `BUDDD_LOG_DEBUG("ViewportPanel: FBO resized {}×{}", w, h)` — only when dimensions actually change. |
| **FBO creation failure** | Error-level log: `BUDDD_LOG_ERROR("ViewportPanel: failed to create framebuffer: {}")` with error message. |
| **FBO resize failure** | Error-level log: `BUDDD_LOG_ERROR("ViewportPanel: FBO resize failed: {}")` with error message. |
| **Render skipped (zero dimensions)** | Trace-level log: `BUDDD_LOG_TRACE("ViewportPanel: skip render (panel too small: {}×{})", w, h)` — useful for debugging if users report black viewports. |
| **Render scene with camera** | Trace-level log: `BUDDD_LOG_TRACE("ViewportPanel: rendering scene with camera (aspect {})", aspect)` — per-frame debug. |
| **Layout first-launch applied** | Debug-level log: `BUDDD_LOG_DEBUG("Editor: applied default viewport layout (Scene|Viewport|Properties)")` — logged from `draw_ui()` first_layout block. |

## Documentation impact

The following existing wiki pages must be updated when this spec is implemented:

| Document | Reason for update |
|---|---|
| `docs/wiki/editor/editor-panels.md` | Update the Viewport Panel section (default position, content, editor camera). Update layout diagram to show 3-column layout. Update the v1 foundation summary to mention ViewportPanel registration. |
| `docs/wiki/architecture/module-map.md` | Add `viewport_panel.h`/`.cpp` to the concrete dockable panels table. Update `RenderSystem` entry to include `render_scene_with_camera()`. |
| `docs/wiki/architecture/data-flow.md` | Update the offscreen rendering/FBO section to describe the Viewport Panel usage pattern with `render_scene_with_camera()`. |
| `docs/wiki/architecture/ADR-019.md` or relevant ADR page | No ADR changes needed — the architecture boundary is respected. |

## Out of scope

- Editor camera controls (fly/look/pan/dolly/orbit/zoom).
- Gizmo rendering (translate/rotate/scale) — deferred.
- Grid overlay rendering — deferred.
- Entity selection highlight in viewport — deferred.
- Click-to-select entities in viewport — deferred.
- Drag-to-select box in viewport — deferred.
- F-key focus on selected entity — deferred.
- Multi-viewport support — deferred.
- Prefab tab viewport — deferred.
- Play mode viewport behavior (camera mode switch, read-only gizmo) — deferred.
- View > Viewport visibility toggle — deferred.
- Camera animation (smooth transitions) — deferred.
- Adding `ViewportCamera` as an engine-level component or entity — the editor camera is an editor concept.
- Any changes to the main loop's `RenderSystem` or `ctx.world` rendering — the viewport is self-contained.

## Assumptions

| ID | Assumption |
|---|---|
| A-01 | `ImGui::GetContentRegionAvail()` returns the available content area inside an ImGui window (after title bar, borders, padding). This is the correct size for the FBO. |
| A-02 | `ImGui::Image()` accepts a `(ImTextureID)(intptr_t)texture.gl_handle()` and renders the texture filling the given size. This is the standard pattern for displaying OpenGL textures in ImGui. Verified by existing usage in the engine. |
| A-03 | `RenderDevice::create_frame_buffer(w, h)` with (1, 1) is valid and succeeds on all target GPUs. |
| A-04 | `FrameBuffer::resize()` is safe to call every frame (only performs GPU work when dimensions actually change — internal guard). We add an additional guard at the panel level to avoid calling resize unnecessarily. |
| A-05 | The editor's `World` outlives the `ViewportPanel` (both owned by `Editor`, destroyed in reverse registration order). |
| A-06 | `Mat4::perspective()` takes FOV in radians. `ViewportCamera::fov_y` is stored in radians (60° converted at initialization). |
| A-07 | The engine's existing clear color (dark gray, `{0.2f, 0.2f, 0.2f, 1.0f}` or similar) is used when rendering into the FBO. The viewport does not override the clear color. |
| A-08 | `render_scene_with_camera()` does not need to handle stencil buffer operations. The FBO has no stencil attachment (D24 depth only). Existing depth-only rendering is unchanged. |
| A-09 | The `RenderSystem` constructor creates a full rendering pipeline bound to the given world. Creating a second `RenderSystem` for the viewport is safe and does not conflict with the main loop's `RenderSystem`. |
| A-10 | The main loop's `ctx.render_system.render_scene()` called on an empty world (no active camera) is an early-return no-op with a trace log. This is the existing behaviour. |
| A-11 | `Buddd_editor.ini` is loaded by ImGui before the first frame's `draw_ui()`, so `DockBuilderGetNode(dockspace_id)` returns the saved layout if one exists. The `first_layout` guard checks this to decide whether to apply the default layout. |
| A-12 | The Viewport panel's `RenderSystem` renders `editor.world()` using the same rendering logic as the main `render_scene()` (same light collection, same MeshRenderer iteration). The only difference is the VP matrix source. |
| A-13 | Panel order in `panels_` vector determines draw order. The ViewportPanel should render after ScenePanel but order does not matter for correctness (each panel is a separate ImGui window). |
| A-14 | `Texture::gl_handle()` returns 0 for headless/null textures. In headless mode, `ImGui::Image` with texture ID 0 renders nothing gracefully. |

## Open questions

| ID | Question | Resolution |
|---|---|---|
| Q-01 | **Should the ViewportPanel's RenderSystem be a member or a pointer?** A member avoids dynamic allocation. The RenderSystem is small (device* + world*) and does not need polymorphic lifetime. | **No clarification needed.** Use direct member (not unique_ptr). |
| Q-02 | **Should the initial FBO size be (1, 1) or should creation be deferred until first draw_ui()?** Creating at (1, 1) ensures the FBO always exists and simplifies null checks. | **No clarification needed.** Create at (1, 1) in constructor. |
| Q-03 | **What ImGui ID/Title should the Viewport panel use?** The panel title "Viewport" matches the north-star layout. `id()` returns `"viewport"` following the snake_case convention of other panels. | **No clarification needed.** Title: "Viewport", id: "viewport". |
| Q-04 | **Should the ViewportPanel be registered before or after other panels?** Panel registration order affects iteration order in `draw_ui()` but not functional behaviour (each panel is an independent ImGui window). Consistent with existing panel registration. | **No clarification needed.** Register after ScenePanel, before PropertiesPanel. |
| Q-05 | **Does `render_scene_with_camera()` need to handle the case where `world_` is empty (no MeshRenderers)?** Yes — the method should silently render nothing (just clear). This matches the existing `render_scene()` behaviour when no active camera exists, but at a different level (no meshes vs no camera). | **No clarification needed.** The method always clears the FBO and iterates MeshRenderers. If none exist, only the clear happens — same as `render_scene()`. |

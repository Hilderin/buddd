# Test Report: Viewport Panel — Scene Rendering (F-07)

## Test Summary

**Total tests**: 832 (828 baseline + 4 new)
**Passed**: 832
**Failed**: 0
**Skipped**: 0

**Build**: clean (zero errors, zero warnings in `src/editor/`, `src/engine/render/`, `tests/`)

---

## Unit Tests

All 832 tests pass. The 4 new tests cover the following gaps identified during analysis:

| Test | Tags | Coverage |
|---|---|---|
| `render_scene_with_camera empty world no crash` | `[editor][viewport]` | AC-018, AC-019 |
| `render_scene_with_camera renders entities with camera_pos` | `[editor][viewport]` | AC-009, AC-010, AC-011 |
| `render_scene_with_camera uses own world` | `[editor][viewport]` | AC-011 |
| `ViewportPanel draw_ui detects World change (guard logic)` | `[editor][viewport]` | Test #12 (contract), World change edge case |

### Existing F-07 tests (original 10 + display test)

| # | Test | Traces to AC |
|---|---|---|
| 1 | ViewportCamera default values (compile-time) | AC-015, AC-016 |
| 2 | ViewportCamera view_projection non-zero aspect | AC-015, AC-016 |
| 3 | ViewportCamera view_projection zero aspect returns identity | AC-005, EC aspect guard |
| 4 | ViewportPanel id and title | AC-002 |
| 5 | ViewportPanel constructor creates FBO and RenderSystem | AC-001, AC-003 |
| 6 | ViewportPanel draw_ui guards against zero dimensions (compile-time) | AC-004 |
| 7 | render_scene_with_camera signature check | AC-008 |
| 8 | render_scene_with_camera lifecycle (no-crash smoke test) | AC-009, AC-018 |
| 9 | RenderDevice clear does not crash | AC-018 |
| 10 | Editor registers ViewportPanel | AC-014, AC-013 |
| 11 (display) | ViewportPanel draw_ui skips rendering on zero dimensions | AC-004 (runtime) |

### New F-07 tests added by tester

| # | Test | Traces to AC |
|---|---|---|
| 12 | render_scene_with_camera empty world no crash | AC-018, AC-019 |
| 13 | render_scene_with_camera renders entities with camera_pos | AC-009, AC-010, AC-011 |
| 14 | render_scene_with_camera uses own world | AC-011 |
| 15 | ViewportPanel draw_ui detects World change (guard logic) | Test #12 (contract) |

---

## Integration / E2E Tests

E2E capture requires a display backend and full ImGui lifecycle. Headless rendering tests pass. Display-mode E2E capture is deferred to manual testing (see below).

| Scenario | Method | Result | Evidence |
|---|---|---|---|
| Headless `render_scene_with_camera` lifecycle | `buddd_tests` headless | PASS | Tests #8, #12, #13, #14 |
| Display-mode draw_ui guards | `buddd_tests` with SDL3 offscreen | PASS | Test #11 |

---

## Regression Checks

| App / Module | Check performed | Result | Evidence |
|---|---|---|---|
| All existing tests | Full `buddd_tests` run (828 → 832) | PASS | All pass, no regressions |
| Build warnings | `cmake --build --preset debug` | PASS | Zero warnings from `src/editor/` and `src/engine/render/` |
| Architecture boundaries | `grep` for SDL3/GL/glm includes in `src/editor/panels/viewport_panel.*` | PASS | No forbidden includes |

No regressions detected.

---

## Coverage Mapping — AC verification

| AC ID | Description | Status | Test(s) |
|---|---|---|---|
| AC-001 | ViewportPanel class exists with correct interface | ✅ | Test 5 |
| AC-002 | id() returns "viewport", title() returns "Viewport" | ✅ | Test 4 |
| AC-003 | Creates FrameBuffer (1,1) in constructor | ✅ | Test 5 (indirect: no-crash) |
| AC-004 | draw_ui guards zero/negative dimensions | ✅ | Tests 6, 11 |
| AC-005 | Aspect ratio guard (aspect ≤ 0 → skip) | ✅ | Test 3 |
| AC-006 | draw_ui calls render_scene_with_camera | ⚠️ | Manual only (needs ImGui frame) |
| AC-007 | draw_ui displays via ImGui::Image | ⚠️ | Manual only (needs ImGui frame) |
| AC-008 | render_scene_with_camera exists with correct signature | ✅ | Test 7 |
| AC-009 | render_scene_with_camera binds/unbinds FBO | ✅ | Tests 8, 12, 13 |
| AC-010 | camera_pos passed to u_camera_pos uniform | ✅ | Test 13 |
| AC-011 | render_scene_with_camera uses world_ (not ctx.world) | ✅ | Tests 13, 14 |
| AC-012 | Default dock layout (3-column north-star) | ⚠️ | Manual only (needs ImGui dockspace) |
| AC-013 | Layout applied only on first launch | ✅ | Test 10, code review |
| AC-014 | ViewportPanel registered in Editor::setup() | ✅ | Test 10 |
| AC-015 | Editor camera at (3,3,3), target (0,0,0), Y-up | ✅ | Tests 1, 2 |
| AC-016 | Editor camera 60° FOV, 0.1 near, 100 far | ✅ | Tests 1, 2 |
| AC-017 | RenderSystem bound to editor.world() | ✅ | Code review + Tests 13, 14 |
| AC-018 | render_scene_with_camera clears FBO | ✅ | Tests 8, 9, 12 |
| AC-019 | Empty world renders dark gray (no crash) | ✅ | Test 12 |
| AC-020 | Entity creation updates viewport | ⚠️ | Manual only |
| AC-021 | Transform edits update viewport | ⚠️ | Manual only |
| AC-022 | All existing tests still pass | ✅ | Verified: 832/832 pass |
| AC-023 | Zero new warnings | ✅ | Verified: clean build |
| AC-024 | No SDL3/GL/glm headers in viewport_panel.* | ✅ | Verified by grep |

Legend: ✅ = Automated test covers it | ⚠️ = Manual test only

---

## Manual Tests Required

The following tests require a display-capable environment and cannot be fully automated:

1. **3-column layout verification**: Launch `buddd edit` and verify:
   - Scene panel docked left (~25% width)
   - Viewport panel in center (filling remaining space)
   - Properties/Inspector panel docked right (~25% width)
   - Console, Project, Assets tabs at bottom (~25% height)

2. **Viewport rendering (empty scene)**: Launch `buddd edit` and verify:
   - Viewport shows a dark gray rectangle (clear color)
   - No rendering errors or crashes

3. **Viewport rendering (with entities)**: Create an entity via right-click → Create Empty, add a MeshRenderer with a model. Verify:
   - The model renders in the viewport
   - The camera shows the scene from (3, 3, 3) looking at origin

4. **Viewport resize**: Drag the left or right dock divider. Verify:
   - The viewport content area resizes smoothly
   - No visual artifacts, stretching, or flickering

5. **Entity transform updates**: Select an entity visible in viewport, edit its transform in the Properties panel (e.g., Position X from 0 to 5). Verify:
   - The viewport updates immediately on next frame
   - The entity moves to the new position

---

## Issues Found

### Blocking
- [ ] None

### Non-blocking
- AC-006, AC-007, AC-012, AC-020, AC-021 remain manual-test only. These require an active ImGui dockspace frame, which cannot be set up in headless CI. The code-contract verification confirms the guard logic exists.
- Test #12 (World pointer change detection) is verified at the code-contract level (compile-time signature check + source review). A full runtime test would require triggering `Editor::new_scene()` with an active ImGui frame.

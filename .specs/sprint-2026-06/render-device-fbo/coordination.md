# Workflow Coordination: render-device-fbo

## Orchestrator

**Feature**: `render-device-fbo`
**Status**: in-progress
**Current step**: completed
**Initial instructions**: Ajouter la capacité de rendu offscreen (FBO/render-to-texture) au RenderDevice de l'engine, prérequis pour le Viewport de l'éditeur (F-07). Le RenderDevice doit pouvoir créer des framebuffer objects et rendre une scène vers une texture, qui pourra être affichée dans un panel ImGui.
**Notes**:
- Feature décidée avec l'humain le 2026-06-14 : on commence par l'infra moteur (FBO) avant le ViewportPanel.
- Issue #1: `RenderDevice` n'a aucune abstraction FBO aujourd'hui. `begin_frame()` dessine toujours dans le framebuffer par défaut. `render_scene()` n'accepte pas de cible de rendu.
- Issue #2: `create_texture()` ne permet pas de créer une texture vide/storage-only (nécessite une `Image` avec données CPU). Il faut une nouvelle API pour créer des textures render-target.
- Issue #3: `read_pixels()` hardcode `GL_BACK`. Pour le readback depuis un FBO custom, il faudra une surcharge ou un paramètre.
- Issue #4: Pas de support depth/stencil en texture ou renderbuffer — seul un depth buffer window-surface existe.
- Aucun spec existant pour le FBO. Le wiki et les ADRs ne décrivent pas cette capacité.
- **Loop back**: implementation-contract-critic rejected contract with 2 blocking issues — re-invoking implementation-contract-author.

**Decision Log** (grill-me, 2026-06-14) :

| # | Question | Décision |
|---|---|---|
| D-01 | Scope de la feature | **A+B+C+D+E** : FrameBuffer abstraction + create_render_texture + depth/stencil renderbuffer + RenderSystem modifié + read_pixels(FBO). |
| D-02 | Depth attachment | **Renderbuffer DEPTH24** (pas depth texture — sera ajouté si shadow mapping nécessaire un jour). |
| D-03 | API d'intégration render_scene | **Option B + helper** : FrameBuffer::bind()/unbind(), render_scene() reste inchangé. L'appelant gère le bind. |
| D-04 | resize(w, h) | **Inclus dans la feature.** `FrameBuffer::resize()` recrée les attachements. |
| D-05 | Tests headless | **FBO no-op valide** en headless (bind/unbind/resize ne crash pas). `read_pixels()` retourne erreur. |
| D-06 | Tests d'intégration | **Test automatisé avec contexte GL offscreen** : créer FBO → render → readback → vérifier pixels. Tourne en local avec display. |
| D-07 | CI headless | Le FrameBuffer ne doit pas crasher en headless. `create_frame_buffer` retourne un objet valide mais no-op. |
| D-08 | Erreurs et edge cases | w=0/h=0 → erreur. FBO incomplete → erreur. Destruction safe après resize. |
| D-09 | Documentation à mettre à jour | `docs/wiki/architecture/overview.md`, `docs/wiki/architecture/data-flow.md`, `docs/adr/ADR-003` si nécessaire. |
| D-10 | Performance | ~14MB pour un FBO 1080p. Création = 3 appels GL. Bind/Unbind = quasi nul. Pas de pooling pour MVP. |

## spec-author

**Status**: completed
**Summary**:
Updated spec per human validation feedback: replaced optional `FrameBuffer*` parameter on `render_scene()` with a separate `render_scene(FrameBuffer&)` overload, matching `read_pixels()` overload pattern. Updated all references in goals, actors, user-visible behavior, user stories, acceptance criteria, and assumptions.
**Artifacts**:
- `.specs/sprint-2026-06/render-device-fbo/spec.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## Human Spec Validation

**Status**: approved
**Summary**: Spec approved with change: render_scene(FrameBuffer&) overload instead of optional pointer.
**Date**: 2026-06-14
**Feedback**: "Je suggère que render_scene tout comme read_pixels soient des overload et non des paramètres optionnels, c'est plus clean" — change applied.

## implementation-contract-author

**Status**: completed
**Summary**:
Fixed 2 blocking issues: (1) added viewport save/restore to FrameBufferOpenGL::bind()/unbind() — previous_viewport_[4] stored and restored; (2) added AC-003 verification steps to the OpenGL integration test — bind FBO, verify binding changed via glGetIntegerv, unbind, verify default framebuffer restored. Also added 2 new headless tests (RenderTexture_Headless_Create, FrameBuffer_Headless_BindResizeUnbind) addressing non-blocking warnings.
**Artifacts**:
- `.specs/sprint-2026-06/render-device-fbo/implementation-contract.md`
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## implementation-contract-critic

**Status**: completed
**Summary**:
Re-review verdict: ACCEPTED. Both blocking issues resolved — AC-003 is now tested (verifies unbind restores default framebuffer via glGetIntegerv), and viewport save/restore is implemented (previous_viewport_[4] saved in bind(), restored in unbind()). Non-blocking warnings about missing tests also addressed (RenderTexture_Headless_Create, FrameBuffer_Headless_BindResizeUnbind added).
**Artifacts**:
- `.specs/sprint-2026-06/render-device-fbo/implementation-contract-critic.md`
**Questions for human**:
none
**Warnings**:
- FBO not explicitly cleared before rendering in render_scene(FrameBuffer&)
- width() return type mismatch (int vs uint32_t)
- Unnecessary self-forward-declaration in FrameBuffer header
- AC-003 test uses GL_DRAW_FRAMEBUFFER_BINDING while implementation uses GL_FRAMEBUFFER_BINDING (they are synonymous in OpenGL 3.0+, but inconsistent)
**Blocking issues**:
none

## Human Validation

**Status**: approved
**Approver**: Guillaume (git config user.name)
**Date**: 2026-06-14
**Warnings**:
none
**Notes**: Implementation contract approved by human. Go to implementation.

## code-implementer

**Status**: completed
**Summary**:
Implemented FrameBuffer abstract class hierarchy (base + OpenGL + Headless backends) with bind/unbind/resize/color_texture. Added create_render_texture, create_frame_buffer, and read_pixels(FrameBuffer&) to RenderDevice (both backends). Added render_scene(FrameBuffer&) overload to RenderSystem. Wrote 9 headless unit tests and 1 OpenGL integration test with offscreen driver. All 814 tests pass, zero new warnings.
**Artifacts**:
- `src/engine/render/frame_buffer.h` (new)
- `src/engine/render/frame_buffer_opengl.h` (new)
- `src/engine/render/frame_buffer_opengl.cpp` (new)
- `src/engine/render/frame_buffer_headless.h` (new)
- `src/engine/render/frame_buffer_headless.cpp` (new)
- `tests/engine/render_device_fbo_tests.cpp` (new)
- `tests/engine/render_device_fbo_opengl_tests.cpp` (new)
- `src/engine/render/render_device.h` (modified)
- `src/engine/render/render_device_opengl.h` (modified)
- `src/engine/render/render_device_opengl.cpp` (modified)
- `src/engine/render/render_device_headless.h` (modified)
- `src/engine/render/render_device_headless.cpp` (modified)
- `src/engine/render/render_system.h` (modified)
- `src/engine/render/render_system.cpp` (modified)
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

## tester

**Status**: completed
**Summary**:
All 817 tests pass (814 existing + 3 new FBO tests added). Full AC coverage: AC-001 through AC-010 are all verified with passing tests. Added 3 missing tests: OpenGL zero-size creation (AC-007), OpenGL resize (AC-004), and RenderSystem::render_scene(FrameBuffer&) overload (AC-008). No regressions, zero new compiler warnings. All OpenGL tests run with SDL offscreen driver — no physical display needed.
**Artifacts**:
- `.specs/sprint-2026-06/render-device-fbo/test-report.md`
**Questions for human**:
none
**Warnings**:
- render_scene(FrameBuffer&) does not clear the FBO before rendering (consistent with regular render_scene() behavior).
- Very large dimensions and multiple FBO sequential creation are not explicitly tested (rely on OpenGL driver behavior).
**Blocking issues**:
none
**Manual tests required**:
none

## Manual Test Validation

**Status**: pending | passed | failed | skipped
**Instructions**:
<step-by-step test instructions from tester, or "none">
**Human feedback**:
<none, or the human's response after testing>
**Date**: <date and time>
**Notes**: <any additional notes>

## wiki-agent

**Status**: completed
**Summary**:
Updated docs/wiki/architecture/overview.md to include new FrameBuffer source files in the directory tree and add a description of the FrameBuffer class hierarchy and API (create_frame_buffer, create_render_texture, read_pixels(FrameBuffer&), render_scene(FrameBuffer&)). Added an "Offscreen rendering / FrameBuffer usage" section to docs/wiki/architecture/data-flow.md documenting the FBO lifecycle, bind/unbind pattern, viewport save/restore, and the intended viewport usage pattern. Checked ADR-003 — no framebuffer-related content to update.
**Artifacts**:
- `docs/wiki/architecture/overview.md` (modified)
- `docs/wiki/architecture/data-flow.md` (modified)
**Changes made**:
- `overview.md`: Added frame_buffer.h, frame_buffer_opengl.h/.cpp, frame_buffer_headless.h/.cpp to the render directory tree; updated render_device.h and render_system.h descriptions; added a Key behaviors bullet for the FrameBuffer/Render-to-Texture feature.
- `data-flow.md`: Added "Offscreen rendering / FrameBuffer usage" section after the frame loop, documenting FBO lifecycle, render_scene(FrameBuffer&) flow, resize, viewport save/restore, headless safety, and the intended editor viewport pattern.
- `ADR-003`: No changes needed — the ADR does not mention framebuffer limitations.
**Questions for human**:
none
**Warnings**:
none
**Blocking issues**:
none

---

**Constraints:**

- Use exact heading names as listed above (case-sensitive).
- Use exact field names as listed above (bold markdown `**Field**`).
- Sub-agent sections must appear in the exact order listed above (spec-author → Human Spec Validation → implementation-contract-author → implementation-contract-critic → Human Validation → code-implementer → tester → Manual Test Validation → wiki-agent).
- The `## Human Validation` section must appear between `## implementation-contract-critic` and `## code-implementer`.
- The `## wiki-agent` section must include `**Changes made**` instead of `**Decisions needed**`.
- **`{{SPRINT}}` must be replaced** with the actual sprint folder (e.g. `sprint-2026-06`) when the orchestrator creates coordination.md from this template.
- **Exception**: during loop-backs, the orchestrator may temporarily reset a sub-agent's `**Status**` to "in-progress" to re-invoke them. This overrides the general principle that sub-agents self-manage their own status and is the only case where the orchestrator writes to a sub-agent's status field.

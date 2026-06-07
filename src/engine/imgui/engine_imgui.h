#pragma once

#include "error.h"

#include <cstdint>

// Forward declarations only — do NOT include SDL3 headers in public API
struct SDL_Window;
union SDL_Event;

namespace buddd::engine::engine_imgui {

/// Initialise ImGui context and both backends (SDL3 + OpenGL3).
/// Must be called after SDL_GL_MakeCurrent and before any ImGui calls.
/// Must be called only when a display is available.
/// Returns error if init fails.
[[nodiscard]] auto init(SDL_Window* window, void* gl_context) -> Result<void>;

/// Shut down ImGui backends and destroy context.
/// Safe to call even if init() was not called (no-op).
auto shutdown() -> void;

/// Begin a new ImGui frame. Must be called once per frame before
/// app.on_frame_begin(). No-op if not initialised.
auto new_frame() -> void;

/// Render ImGui draw data. Must be called once per frame after
/// app.on_render() and before end_frame(). No-op if not initialised.
auto render() -> void;

/// Forward an SDL event to ImGui_ImplSDL3_ProcessEvent().
/// Returns true if ImGui consumed the event.
/// No-op if not initialised — returns false.
[[nodiscard]] auto on_sdl_event(const SDL_Event& event) -> bool;

/// Returns true if ImGui was successfully initialised.
[[nodiscard]] auto is_initialized() -> bool;

} // namespace buddd::engine::engine_imgui

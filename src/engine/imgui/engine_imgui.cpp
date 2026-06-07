#include "imgui/engine_imgui.h"

#include <imgui.h>
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_opengl3.h"

#include "log/log.h"

BUDDD_LOG_TAG("ImGui");

namespace buddd::engine::engine_imgui {

// ============================================================================
// Internal state (file-scope static variables)
// ============================================================================
static ImGuiContext* s_context = nullptr;
static bool s_initialized = false;

// ============================================================================
// init
// ============================================================================

auto init(SDL_Window* window, void* gl_context) -> Result<void> {
    if (s_initialized) {
        return make_error(Error::Category::InitFailed, "ImGui already initialised");
    }

    if (window == nullptr) {
        return make_error(Error::Category::InvalidArgument, "SDL_Window cannot be null");
    }

    IMGUI_CHECKVERSION();

    s_context = ImGui::CreateContext();
    if (s_context == nullptr) {
        return make_error(Error::Category::InitFailed, "ImGui::CreateContext failed");
    }

    ImGui::SetCurrentContext(s_context);

    // Disable ini file persistence by default. Apps opt in via setup().
    ImGui::GetIO().IniFilename = nullptr;

    if (!ImGui_ImplSDL3_InitForOpenGL(window, gl_context)) {
        ImGui::DestroyContext(s_context);
        s_context = nullptr;
        return make_error(Error::Category::InitFailed, "ImGui_ImplSDL3_InitForOpenGL failed");
    }

    if (!ImGui_ImplOpenGL3_Init("#version 410 core")) {
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext(s_context);
        s_context = nullptr;
        return make_error(Error::Category::InitFailed, "ImGui_ImplOpenGL3_Init failed");
    }

    s_initialized = true;

    BUDDD_LOG_INFO("ImGui: initialised (backend SDL3+OpenGL3)");

    return {};
}

// ============================================================================
// shutdown
// ============================================================================

auto shutdown() -> void {
    if (!s_initialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(s_context);

    s_context = nullptr;
    s_initialized = false;

    BUDDD_LOG_INFO("ImGui: shutdown");
}

// ============================================================================
// new_frame
// ============================================================================

auto new_frame() -> void {
    if (!s_initialized) {
        BUDDD_LOG_TRACE("ImGui: skipped (not initialised)");
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

// ============================================================================
// render
// ============================================================================

auto render() -> void {
    if (!s_initialized) {
        BUDDD_LOG_TRACE("ImGui: skipped (not initialised)");
        return;
    }

    ImGui::Render();
    if (ImGui::GetDrawData() != nullptr) {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}

// ============================================================================
// on_sdl_event
// ============================================================================

auto on_sdl_event(const SDL_Event& event) -> bool {
    if (!s_initialized) {
        return false;
    }

    return ImGui_ImplSDL3_ProcessEvent(&event);
}

// ============================================================================
// is_initialized
// ============================================================================

auto is_initialized() -> bool {
    return s_initialized;
}

} // namespace buddd::engine::engine_imgui

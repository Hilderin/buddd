#include "inspector_editors.h"

#include "editor.h"
#include "log/log.h"

#include "math/color.h"
#include "math/quat.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <typeindex>

#include "engine_service.h"
#include "input/input_system.h"
#include "platform/platform.h"
#include "window/window.h"

namespace buddd::editor {
namespace {

/// Per-handle state for indefinite drag-to-scrub using relative mouse mode.
struct DragState {
    float initial_value;      ///< Value when the drag started.
    float drag_accumulator;   ///< Accumulated raw mouse delta since drag start.
    float start_x;            ///< ImGui window X coordinate at drag start.
    float start_y;            ///< ImGui window Y coordinate at drag start.
};

/// Draw a composite axis input widget.
///
/// ┌──────────┬──────────┐
/// │ [ 0.00 ] │ [■ LABEL] │
/// └──────────┴──────────┘
///
/// Left side: an ImGui::InputFloat for single-click text entry, format "%.2f".
///
/// Right side: a colored rectangle (~20px wide) drawn via ImDrawList with white text label.
/// An ImGui::InvisibleButton of the same size is overlaid for hit testing.
/// Click+drag left/right on the handle scrubs the float value.
///
/// @param id     Short identifier and display text for the drag handle (e.g., "X", "Y", "Z").
///               Used for both PushID scoping and as the label text on the colored rectangle.
/// @param value     Pointer to the float value being edited.
/// @param color     Axis color as ImVec4 (e.g., red for X, green for Y, blue for Z).
/// @param drag_speed Sensitivity for drag-to-scrub (0.1 for position/scale, 0.5 for rotation).
/// @param ctx       EditorContext (reserved for future use).
/// @param tooltip   Optional tooltip text shown on hover over the drag handle (e.g., "Pitch").
/// @return true if the value changed this frame.
auto draw_axis_widget(const char* id, float* value, ImVec4 color,
                      float drag_speed, const EditorContext& ctx,
                      const char* tooltip = nullptr) -> bool {
    ImGui::PushID(id);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    float widget_height = ImGui::GetFrameHeight();

    // ── InputFloat (LEFT side) ──
    ImGui::SetNextItemWidth(60.0f);
    bool input_changed = ImGui::InputFloat("##input", value, 0.0f, 0.0f, "%.5g");

    // ── Colored drag handle (RIGHT side, flush against input) ──
    ImGui::SameLine(0.0f, 0.0f);

    ImVec2 cursor_pos = ImGui::GetCursorScreenPos();

    // Colored rectangle
    draw_list->AddRectFilled(cursor_pos,
                              ImVec2(cursor_pos.x + 20.0f, cursor_pos.y + widget_height),
                              ImGui::ColorConvertFloat4ToU32(color));

    // Centered text
    ImVec2 text_size = ImGui::CalcTextSize(id);
    draw_list->AddText(
        ImVec2(cursor_pos.x + (20.0f - text_size.x) * 0.5f,
               cursor_pos.y + (widget_height - text_size.y) * 0.5f),
        IM_COL32(255, 255, 255, 255), id);

    // InvisibleButton for hit testing
    ImGui::InvisibleButton("##handle", ImVec2(20.0f, widget_height));

    // Tooltip
    if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip("%s", tooltip);
    }

    // Indefinite drag-to-scrub using relative mouse mode
    static std::unordered_map<const void*, DragState> drag_states;
    bool drag_changed = false;

    if (ImGui::IsItemActive()) {
        if (ImGui::IsItemActivated()) {
            // Save initial state and enable relative mouse mode
            DragState ds;
            ds.initial_value = *value;
            ds.drag_accumulator = 0.0f;
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ds.start_x = mouse_pos.x;
            ds.start_y = mouse_pos.y;
            drag_states[static_cast<const void*>(value)] = ds;

            ctx.engine.window.set_mouse_capture(true);

            BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                "drag start: handle={} initial_value={}", id, ds.initial_value);
        }

        auto& ds = drag_states[static_cast<const void*>(value)];
        auto& input = ctx.engine.services.platform().input_system();
        ds.drag_accumulator += input.mouse_delta().first;
        float new_val = ds.initial_value + ds.drag_accumulator * drag_speed * 0.01f;

        if (new_val != *value) {
            *value = new_val;
            drag_changed = true;
        }
    }

    if (ImGui::IsItemDeactivated()) {
        auto it = drag_states.find(static_cast<const void*>(value));
        if (it != drag_states.end()) {
            const auto& ds = it->second;

            ctx.engine.window.set_mouse_capture(false);

            auto& input = ctx.engine.services.platform().input_system();
            input.set_mouse_position(static_cast<int>(ds.start_x),
                                     static_cast<int>(ds.start_y));

            BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                "drag end: handle={} final_value={}", id, *value);
            BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                "warp mouse to ({}, {})", ds.start_x, ds.start_y);

            drag_states.erase(it);
        }
    }

    ImGui::PopID();

    return drag_changed || input_changed;
}

} // anonymous namespace

// ── static registry map ──
auto InspectorTypeEditorRegistry::map()
    -> std::unordered_map<std::type_index, std::unique_ptr<InspectorTypeEditor>>&
{
    static std::unordered_map<std::type_index, std::unique_ptr<InspectorTypeEditor>> instance;
    return instance;
}

auto InspectorTypeEditorRegistry::get(std::type_index type) -> InspectorTypeEditor* {
    auto& m = map();
    auto it = m.find(type);
    if (it != m.end()) return it->second.get();
    return nullptr;
}

// ── Fallback read-only display ──

auto draw_fallback_readonly(const std::string& label, std::type_index type,
                            const EditorContext& ctx) -> void {
    (void)ctx;
    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
        "No editor registered for type '%s' — rendering read-only fallback",
        type.name());
    // Guard against missing ImGui context (headless mode)
    if (ImGui::GetCurrentContext()) {
        ImGui::TextDisabled("(no editor for type %s)", type.name());
    }
}

// ── register_builtin_inspector_editors ──

auto register_builtin_inspector_editors() -> void {
    using namespace buddd::engine;

    // float — composite widget: InputFloat + gray drag handle
    InspectorTypeEditorRegistry::register_editor<float>(
        [](const std::string& label, float& value, const EditorFlags& flags,
           const EditorContext& ctx) -> bool {
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;

            ImGui::PushID(label.c_str());

            // ── InputFloat (left side) ──
            ImGui::SetNextItemWidth(60.0f);
            bool changed = ImGui::InputFloat("##val", &value, 0.0f, 0.0f, "%.5g");

            // ── Gray drag handle (right side) ──
            ImGui::SameLine(0.0f, 0.0f);

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
            float widget_height = ImGui::GetFrameHeight();

            ImVec4 gray_color(0.5f, 0.5f, 0.5f, 1.0f);
            draw_list->AddRectFilled(
                cursor_pos,
                ImVec2(cursor_pos.x + 20.0f, cursor_pos.y + widget_height),
                ImGui::ColorConvertFloat4ToU32(gray_color));

            ImGui::InvisibleButton("##handle", ImVec2(20.0f, widget_height));

            // Indefinite drag-to-scrub using relative mouse mode
            static std::unordered_map<const void*, DragState> drag_states;
            if (ImGui::IsItemActive()) {
                if (ImGui::IsItemActivated()) {
                    DragState ds;
                    ds.initial_value = value;
                    ds.drag_accumulator = 0.0f;
                    ImVec2 mouse_pos = ImGui::GetMousePos();
                    ds.start_x = mouse_pos.x;
                    ds.start_y = mouse_pos.y;
                    drag_states[static_cast<const void*>(&value)] = ds;

                    ctx.engine.window.set_mouse_capture(true);

                    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                        "drag start: handle={} initial_value={}", label.c_str(), ds.initial_value);
                }

                auto& ds = drag_states[static_cast<const void*>(&value)];
                auto& input = ctx.engine.services.platform().input_system();
                ds.drag_accumulator += input.mouse_delta().first;
                float new_val = ds.initial_value + ds.drag_accumulator * speed * 0.01f;

                if (new_val != value) {
                    value = new_val;
                    changed = true;
                }
            }
            if (ImGui::IsItemDeactivated()) {
                auto it = drag_states.find(static_cast<const void*>(&value));
                if (it != drag_states.end()) {
                    const auto& ds = it->second;

                    ctx.engine.window.set_mouse_capture(false);

                    auto& input = ctx.engine.services.platform().input_system();
                    input.set_mouse_position(static_cast<int>(ds.start_x),
                                             static_cast<int>(ds.start_y));

                    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                        "drag end: handle={} final_value={}", label.c_str(), value);
                    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
                        "warp mouse to ({}, {})", ds.start_x, ds.start_y);

                    drag_states.erase(it);
                }
            }

            ImGui::PopID();

            if (changed) {
                value = std::clamp(value, flags.min_value, flags.max_value);
                ctx.editor.mark_dirty();
            }
            return changed;
        }
    );

    // int
    InspectorTypeEditorRegistry::register_editor<int>(
        [](const std::string& label, int& value, const EditorFlags& flags,
           const EditorContext& ctx) -> bool {
            (void)label;
            bool changed = ImGui::DragInt("##val", &value, 1.0f,
                                           static_cast<int>(flags.min_value),
                                           static_cast<int>(flags.max_value));
            if (changed) {
                ctx.editor.mark_dirty();
            }
            return changed;
        }
    );

    // bool
    InspectorTypeEditorRegistry::register_editor<bool>(
        [](const std::string& label, bool& value, const EditorFlags&,
           const EditorContext& ctx) -> bool {
            (void)label;
            bool changed = ImGui::Checkbox("##val", &value);
            if (changed) {
                ctx.editor.mark_dirty();
            }
            return changed;
        }
    );

    // std::string
    InspectorTypeEditorRegistry::register_editor<std::string>(
        [](const std::string& label, std::string& value, const EditorFlags&,
           const EditorContext& ctx) -> bool {
            (void)label;
            // Use a local buffer for ImGui::InputText
            constexpr size_t BUF_SIZE = 1024;
            char buf[BUF_SIZE];
            std::strncpy(buf, value.c_str(), BUF_SIZE - 1);
            buf[BUF_SIZE - 1] = '\0';
            if (ImGui::InputText("##val", buf, BUF_SIZE)) {
                value = buf;
                ctx.editor.mark_dirty();
                return true;
            }
            return false;
        }
    );

    // math::Vec2
    InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Vec2>(
        [](const std::string& id, buddd::engine::math::Vec2& value,
           const EditorFlags& flags,
           const EditorContext& ctx) -> bool {
            float vals[2] = {value.x, value.y};
            ImGui::PushID(id.c_str());
            bool changed = false;
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;

            changed |= draw_axis_widget("X", &vals[0], ImVec4(0.7f, 0.1f, 0.1f, 1.0f), speed, ctx);
            ImGui::SameLine();
            changed |= draw_axis_widget("Y", &vals[1], ImVec4(0.0f, 0.55f, 0.0f, 1.0f), speed, ctx);

            ImGui::PopID();
            if (changed) {
                vals[0] = std::clamp(vals[0], flags.min_value, flags.max_value);
                vals[1] = std::clamp(vals[1], flags.min_value, flags.max_value);
                value.x = vals[0];
                value.y = vals[1];
                ctx.editor.mark_dirty();
            }
            return changed;
        }
    );

    // math::Vec3
    InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Vec3>(
        [](const std::string& id, buddd::engine::math::Vec3& value,
           const EditorFlags& flags,
           const EditorContext& ctx) -> bool {
            float vals[3] = {value.x, value.y, value.z};
            ImGui::PushID(id.c_str());
            bool changed = false;
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;

            changed |= draw_axis_widget("X", &vals[0], ImVec4(0.7f, 0.1f, 0.1f, 1.0f), speed, ctx);
            ImGui::SameLine();
            changed |= draw_axis_widget("Y", &vals[1], ImVec4(0.0f, 0.55f, 0.0f, 1.0f), speed, ctx);
            ImGui::SameLine();
            changed |= draw_axis_widget("Z", &vals[2], ImVec4(0.27f, 0.27f, 1.0f, 1.0f), speed, ctx);

            ImGui::PopID();
            if (changed) {
                vals[0] = std::clamp(vals[0], flags.min_value, flags.max_value);
                vals[1] = std::clamp(vals[1], flags.min_value, flags.max_value);
                vals[2] = std::clamp(vals[2], flags.min_value, flags.max_value);
                value.x = vals[0];
                value.y = vals[1];
                value.z = vals[2];
                ctx.editor.mark_dirty();
            }
            return changed;
        }
    );

    // math::Vec4
    InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Vec4>(
        [](const std::string& id, buddd::engine::math::Vec4& value,
           const EditorFlags& flags,
           const EditorContext& ctx) -> bool {
            float vals[4] = {value.x, value.y, value.z, value.w};
            ImGui::PushID(id.c_str());
            bool changed = false;
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;

            changed |= draw_axis_widget("X", &vals[0], ImVec4(0.7f, 0.1f, 0.1f, 1.0f), speed, ctx);
            ImGui::SameLine();
            changed |= draw_axis_widget("Y", &vals[1], ImVec4(0.0f, 0.55f, 0.0f, 1.0f), speed, ctx);
            ImGui::SameLine();
            changed |= draw_axis_widget("Z", &vals[2], ImVec4(0.27f, 0.27f, 1.0f, 1.0f), speed, ctx);
            ImGui::SameLine();
            changed |= draw_axis_widget("W", &vals[3], ImVec4(0.7f, 0.7f, 0.7f, 1.0f), speed, ctx);

            ImGui::PopID();
            if (changed) {
                vals[0] = std::clamp(vals[0], flags.min_value, flags.max_value);
                vals[1] = std::clamp(vals[1], flags.min_value, flags.max_value);
                vals[2] = std::clamp(vals[2], flags.min_value, flags.max_value);
                vals[3] = std::clamp(vals[3], flags.min_value, flags.max_value);
                value.x = vals[0];
                value.y = vals[1];
                value.z = vals[2];
                value.w = vals[3];
                ctx.editor.mark_dirty();
            }
            return changed;
        }
    );

    // math::Quat — displayed as Euler angles in degrees
    InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Quat>(
        [](const std::string& id, buddd::engine::math::Quat& value,
           const EditorFlags&,
           const EditorContext& ctx) -> bool {
            // ── Helper: degrees ↔ radians ──
            static constexpr double RAD_TO_DEG = 180.0 / 3.14159265358979323846;
            static constexpr double DEG_TO_RAD = 3.14159265358979323846 / 180.0;

            // Convert quat to Euler radians, then to degrees
            auto euler_rad = value.to_euler();
            float pitch_deg = static_cast<float>(euler_rad.x * RAD_TO_DEG);
            float yaw_deg   = static_cast<float>(euler_rad.y * RAD_TO_DEG);
            float roll_deg  = static_cast<float>(euler_rad.z * RAD_TO_DEG);

            // Wrap to [-180, 180]
            auto wrap = [](float deg) -> float {
                deg = std::fmod(deg + 180.0f, 360.0f);
                if (deg < 0.0f) deg += 360.0f;
                return deg - 180.0f;
            };
            pitch_deg = wrap(pitch_deg);
            yaw_deg   = wrap(yaw_deg);
            roll_deg  = wrap(roll_deg);

            ImGui::PushID(id.c_str());
            bool changed = false;
            constexpr float speed = 0.5f;

            changed |= draw_axis_widget("X", &pitch_deg,
                                         ImVec4(0.7f, 0.1f, 0.1f, 1.0f), speed, ctx,
                                         "Pitch (rotation around X axis)");
            ImGui::SameLine();
            changed |= draw_axis_widget("Y", &yaw_deg,
                                         ImVec4(0.0f, 0.55f, 0.0f, 1.0f), speed, ctx,
                                         "Yaw (rotation around Y axis)");
            ImGui::SameLine();
            changed |= draw_axis_widget("Z", &roll_deg,
                                         ImVec4(0.27f, 0.27f, 1.0f, 1.0f), speed, ctx,
                                         "Roll (rotation around Z axis)");
            ImGui::PopID();

            if (changed) {
                // Wrap each angle to [-180, 180]
                pitch_deg = wrap(pitch_deg);
                yaw_deg   = wrap(yaw_deg);
                roll_deg  = wrap(roll_deg);

                // Convert back to radians and construct quaternion
                float pitch_rad = static_cast<float>(pitch_deg * DEG_TO_RAD);
                float yaw_rad   = static_cast<float>(yaw_deg * DEG_TO_RAD);
                float roll_rad  = static_cast<float>(roll_deg * DEG_TO_RAD);
                value = buddd::engine::math::Quat::from_euler(pitch_rad, yaw_rad, roll_rad);
                ctx.editor.mark_dirty();
            }
            return changed;
        }
    );

    // math::Color
    InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Color>(
        [](const std::string& id, buddd::engine::math::Color& value,
           const EditorFlags& flags,
           const EditorContext& ctx) -> bool {
            float vals[4] = {value.r, value.g, value.b, value.a};
            ImGui::PushID(id.c_str());
            bool changed = false;

            if (flags.has_tag("rgb")) {
                // 3-channel color picker (no alpha)
                changed = ImGui::ColorEdit3("##color", vals,
                    ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                if (changed) {
                    value.r = vals[0];
                    value.g = vals[1];
                    value.b = vals[2];
                    // alpha unchanged
                }
            } else {
                // 4-channel color picker (with alpha)
                changed = ImGui::ColorEdit4("##color", vals,
                    ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                if (changed) {
                    value.r = vals[0];
                    value.g = vals[1];
                    value.b = vals[2];
                    value.a = vals[3];
                }
            }

            ImGui::PopID();
            if (changed) {
                ctx.editor.mark_dirty();
            }
            return changed;
        }
    );

    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
        "Registered 9 built-in inspector editors (float, int, bool, string, Vec2, Vec3, Vec4, Quat, Color)");
}

} // namespace buddd::editor

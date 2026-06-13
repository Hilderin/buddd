#include "inspector_editors.h"

#include "editor.h"
#include "log/log.h"

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

namespace buddd::editor {

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
    ImGui::TextDisabled("(no editor for type %s)", type.name());
}

// ── register_builtin_inspector_editors ──

auto register_builtin_inspector_editors() -> void {
    using namespace buddd::engine;

    // float
    InspectorTypeEditorRegistry::register_editor<float>(
        [](const std::string& label, float& value, const EditorFlags& flags,
           const EditorContext& ctx) -> bool {
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;
            bool changed = ImGui::DragFloat(label.c_str(), &value, speed,
                                            flags.min_value, flags.max_value);
            if (changed) {
                ctx.editor.mark_dirty();
            }
            return changed;
        }
    );

    // int
    InspectorTypeEditorRegistry::register_editor<int>(
        [](const std::string& label, int& value, const EditorFlags& flags,
           const EditorContext& ctx) -> bool {
            bool changed = ImGui::DragInt(label.c_str(), &value, 1.0f,
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
            bool changed = ImGui::Checkbox(label.c_str(), &value);
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
            // Use a local buffer for ImGui::InputText
            constexpr size_t BUF_SIZE = 1024;
            char buf[BUF_SIZE];
            std::strncpy(buf, value.c_str(), BUF_SIZE - 1);
            buf[BUF_SIZE - 1] = '\0';
            if (ImGui::InputText(label.c_str(), buf, BUF_SIZE)) {
                value = buf;
                ctx.editor.mark_dirty();
                return true;
            }
            return false;
        }
    );

    // math::Vec2
    InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Vec2>(
        [](const std::string& label, buddd::engine::math::Vec2& value,
           const EditorFlags& flags,
           const EditorContext& ctx) -> bool {
            float vals[2] = {value.x, value.y};
            ImGui::TextUnformatted(label.c_str());
            ImGui::SameLine();
            ImGui::PushID(label.c_str());
            ImGui::PushItemWidth(80.0f);
            bool changed = false;
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;
            changed |= ImGui::DragFloat("##x", &vals[0], speed, flags.min_value, flags.max_value, "X: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##y", &vals[1], speed, flags.min_value, flags.max_value, "Y: %.2f");
            ImGui::PopItemWidth();
            ImGui::PopID();
            if (changed) {
                value.x = vals[0];
                value.y = vals[1];
                ctx.editor.mark_dirty();
            }
            return changed;
        }
    );

    // math::Vec3
    InspectorTypeEditorRegistry::register_editor<buddd::engine::math::Vec3>(
        [](const std::string& label, buddd::engine::math::Vec3& value,
           const EditorFlags& flags,
           const EditorContext& ctx) -> bool {
            float vals[3] = {value.x, value.y, value.z};
            ImGui::TextUnformatted(label.c_str());
            ImGui::SameLine();
            ImGui::PushID(label.c_str());
            ImGui::PushItemWidth(80.0f);
            bool changed = false;
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;
            changed |= ImGui::DragFloat("##x", &vals[0], speed, flags.min_value, flags.max_value, "X: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##y", &vals[1], speed, flags.min_value, flags.max_value, "Y: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##z", &vals[2], speed, flags.min_value, flags.max_value, "Z: %.2f");
            ImGui::PopItemWidth();
            ImGui::PopID();
            if (changed) {
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
        [](const std::string& label, buddd::engine::math::Vec4& value,
           const EditorFlags& flags,
           const EditorContext& ctx) -> bool {
            float vals[4] = {value.x, value.y, value.z, value.w};
            ImGui::TextUnformatted(label.c_str());
            ImGui::SameLine();
            ImGui::PushID(label.c_str());
            ImGui::PushItemWidth(60.0f);
            bool changed = false;
            float speed = (flags.step_value > 0.0f) ? flags.step_value : 0.1f;
            changed |= ImGui::DragFloat("##x", &vals[0], speed, flags.min_value, flags.max_value, "X: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##y", &vals[1], speed, flags.min_value, flags.max_value, "Y: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##z", &vals[2], speed, flags.min_value, flags.max_value, "Z: %.2f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##w", &vals[3], speed, flags.min_value, flags.max_value, "W: %.2f");
            ImGui::PopItemWidth();
            ImGui::PopID();
            if (changed) {
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
        [](const std::string& label, buddd::engine::math::Quat& value,
           const EditorFlags&,
           const EditorContext& ctx) -> bool {
            // ── Helper: degrees ↔ radians without GLM ──
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

            ImGui::TextUnformatted(label.c_str());
            ImGui::SameLine();
            ImGui::PushID(label.c_str());
            ImGui::PushItemWidth(80.0f);
            bool changed = false;
            const float speed = 0.5f;
            const float no_min = -std::numeric_limits<float>::max();
            const float no_max = std::numeric_limits<float>::max();

            changed |= ImGui::DragFloat("##pitch", &pitch_deg, speed, no_min, no_max, "Pitch: %.1f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##yaw",   &yaw_deg,   speed, no_min, no_max, "Yaw: %.1f");
            ImGui::SameLine();
            changed |= ImGui::DragFloat("##roll",  &roll_deg,  speed, no_min, no_max, "Roll: %.1f");
            ImGui::PopItemWidth();
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

    BUDDD_LOG_TAGGED_DEBUG("Editor:Inspector",
        "Registered 8 built-in inspector editors (float, int, bool, string, Vec2, Vec3, Vec4, Quat)");
}

} // namespace buddd::editor

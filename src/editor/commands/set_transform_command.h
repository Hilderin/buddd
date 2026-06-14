#pragma once

#include "command.h"
#include "editor.h"
#include "editor_context.h"
#include "editor_selection.h"

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"
#include "scene/transform.h"

#include <string_view>

namespace buddd::editor {

/// Which transform property triggered this command (for per-property undo granularity).
enum class TransformProperty { Position, Rotation, Scale };

/// Command that stores all three transform properties (Position, Rotation, Scale)
/// as native math types (Vec3/Quat). Every SetTransformCommand captures all 3
/// properties regardless of which specific property was changed.
/// No YAML — stores Vec3/Quat directly.
/// Uses TransformProperty to track which property triggered the edit,
/// enabling per-property undo granularity within the transform section.
class SetTransformCommand final : public Command {
public:
    SetTransformCommand(
        buddd::engine::EntityId entity_id,
        TransformProperty property,
        buddd::engine::math::Vec3 old_position,
        buddd::engine::math::Quat old_rotation,
        buddd::engine::math::Vec3 old_scale,
        buddd::engine::math::Vec3 new_position,
        buddd::engine::math::Quat new_rotation,
        buddd::engine::math::Vec3 new_scale)
        : entity_id_(entity_id)
        , property_(property)
        , old_position_(old_position)
        , old_rotation_(old_rotation)
        , old_scale_(old_scale)
        , new_position_(new_position)
        , new_rotation_(new_rotation)
        , new_scale_(new_scale)
    {}

    auto execute(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);

        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetTransformCommand: entity {} not found", entity_id_.index);
            return;
        }

        auto& t = entity.transform();
        t.position = new_position_;
        t.rotation = new_rotation_;
        t.scale = new_scale_;
        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "SetTransform: entity={}", entity_id_.index);
    }

    auto undo(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);

        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetTransformCommand UNDO: entity {} not found", entity_id_.index);
            return;
        }

        auto& t = entity.transform();
        t.position = old_position_;
        t.rotation = old_rotation_;
        t.scale = old_scale_;
        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "SetTransform UNDO: entity={}", entity_id_.index);
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Set Transform";
    }

    [[nodiscard]] auto try_update_new_value(YAML::Node const& new_value,
                                              EditorContext const& ctx,
                                              std::string_view property_name) -> bool override {
        // Reject non-empty YAML — this is NOT a transform merge request
        // (draw_component_sections passes valid YAML; draw_transform_section passes empty/undefined)
        if (new_value.IsDefined() && !new_value.IsNull()) {
            return false;
        }

        // Per-property merge guard: only merge if the same property is being edited
        // (e.g., editing Position then Rotation should create separate undo steps)
        auto prop_to_string = [](TransformProperty p) -> std::string_view {
            switch (p) {
                case TransformProperty::Position: return "Position";
                case TransformProperty::Rotation: return "Rotation";
                case TransformProperty::Scale:    return "Scale";
            }
            return "";
        };
        if (property_name != prop_to_string(property_)) {
            return false;
        }

        // Safety: prevent cross-entity merge — command's entity_id_ must match
        // the currently selected primary entity.
        auto primary = ctx.editor.selection().primary();
        if (!primary.has_value() || *primary != entity_id_) {
            return false;
        }

        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);

        if (entity.id() == buddd::engine::EntityId::none()) {
            return false;
        }

        auto& t = entity.transform();
        new_position_ = t.position;
        new_rotation_ = t.rotation;
        new_scale_ = t.scale;

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "Merged SetTransformCommand for entity={}, prop={}",
            entity_id_.index, property_name);
        return true;
    }

private:
    buddd::engine::EntityId entity_id_;
    TransformProperty property_;

    buddd::engine::math::Vec3 old_position_;
    buddd::engine::math::Quat old_rotation_;
    buddd::engine::math::Vec3 old_scale_;

    buddd::engine::math::Vec3 new_position_;
    buddd::engine::math::Quat new_rotation_;
    buddd::engine::math::Vec3 new_scale_;
};

} // namespace buddd::editor

#pragma once

#include "command.h"
#include "editor.h"
#include "editor_context.h"
#include "editor_selection.h"
#include "engine_service.h"
#include "scene/component_registry/component_info.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/serialization_context.h"
#include "scene/world.h"

#include "log/log.h"

#include <yaml-cpp/yaml.h>

#include <optional>
#include <string>
#include <string_view>
#include <typeindex>

namespace buddd::editor {

/// Command that modifies a single component property on an entity.
/// Uses YAML-based value transport: the old and new values are stored as YAML nodes,
/// and applied via ComponentInfoBase::property_deserialize().
class SetComponentPropertyCommand final : public Command {
public:
    SetComponentPropertyCommand(
        buddd::engine::EntityId entity_id,
        std::string component_type_name,
        std::string property_name,
        YAML::Node old_value,
        YAML::Node new_value)
        : entity_id_(entity_id)
        , component_type_name_(std::move(component_type_name))
        , property_name_(std::move(property_name))
        , old_value_(std::move(old_value))
        , new_value_(std::move(new_value))
    {}

    auto execute(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);

        // Check if entity is valid
        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand: entity {} not found",
                entity_id_.index);
            return;
        }

        // Resolve component_type_name_ to ComponentInfoBase
        auto& registry = ctx.engine.services.registry();
        const auto* info = registry.describe(component_type_name_);
        if (!info) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand: component type '{}' not registered",
                component_type_name_);
            return;
        }

        // Build type_index from info using SceneSaver pattern:
        // create() a temporary instance and extract typeid(*tmp).
        auto tmp = const_cast<buddd::engine::ComponentInfoBase*>(info)->create();
        auto target_type = std::type_index(typeid(*tmp));

        // Find the component on the entity by matching type_index
        std::optional<size_t> component_index;
        for (size_t i = 0; i < entity.component_count(); ++i) {
            auto& comp = entity.component_at(i);
            if (std::type_index(typeid(comp)) == target_type) {
                component_index = i;
                break;
            }
        }

        if (!component_index.has_value()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand: component '{}' not found on entity {}",
                component_type_name_, entity_id_.index);
            return;
        }

        // Find the property index by name
        std::optional<size_t> prop_index;
        for (size_t j = 0; j < info->property_count(); ++j) {
            if (info->property_name(j) == property_name_) {
                prop_index = j;
                break;
            }
        }

        if (!prop_index.has_value()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand: property '{}' not found on component '{}'",
                property_name_, component_type_name_);
            return;
        }

        // Read current value — for redundancy check
        auto ser_ctx = buddd::engine::SerializationContext{ctx.engine.services.assets()};
        auto current_yaml = info->property_serialize(entity.component_at(*component_index), *prop_index, ser_ctx);

        // If current value already matches new_value, no-op
        if (current_yaml == new_value_) {
            return;
        }

        // Apply new value
        auto result = info->property_deserialize(entity.component_at(*component_index), *prop_index, new_value_, ser_ctx);
        if (!result) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand: property_deserialize failed for '{}' on '{}': {}",
                property_name_, component_type_name_, result.error().message);
            // Still mark dirty — the write was attempted
        }

        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "SetComponentProperty: entity={} comp={} prop={}", entity_id_.index, component_type_name_, property_name_);
    }

    auto undo(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);

        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand UNDO: entity {} not found",
                entity_id_.index);
            return;
        }

        // Resolve component_type_name_ to ComponentInfoBase
        auto& registry = ctx.engine.services.registry();
        const auto* info = registry.describe(component_type_name_);
        if (!info) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand UNDO: component type '{}' not registered",
                component_type_name_);
            return;
        }

        // Build type_index from info using SceneSaver pattern
        auto tmp = const_cast<buddd::engine::ComponentInfoBase*>(info)->create();
        auto target_type = std::type_index(typeid(*tmp));

        // Find the component on the entity by matching type_index
        std::optional<size_t> component_index;
        for (size_t i = 0; i < entity.component_count(); ++i) {
            auto& comp = entity.component_at(i);
            if (std::type_index(typeid(comp)) == target_type) {
                component_index = i;
                break;
            }
        }

        if (!component_index.has_value()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand UNDO: component '{}' not found on entity {}",
                component_type_name_, entity_id_.index);
            return;
        }

        // Find the property index by name
        std::optional<size_t> prop_index;
        for (size_t j = 0; j < info->property_count(); ++j) {
            if (info->property_name(j) == property_name_) {
                prop_index = j;
                break;
            }
        }

        if (!prop_index.has_value()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand UNDO: property '{}' not found on component '{}'",
                property_name_, component_type_name_);
            return;
        }

        // Write old value
        auto ser_ctx = buddd::engine::SerializationContext{ctx.engine.services.assets()};
        auto result = info->property_deserialize(entity.component_at(*component_index), *prop_index, old_value_, ser_ctx);
        if (!result) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "SetComponentPropertyCommand UNDO: property_deserialize failed for '{}' on '{}': {}",
                property_name_, component_type_name_, result.error().message);
        }

        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "SetComponentProperty UNDO: entity={} comp={} prop={}", entity_id_.index, component_type_name_, property_name_);
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Set Component Property";
    }

private:
    buddd::engine::EntityId entity_id_;
    std::string component_type_name_;
    std::string property_name_;
    YAML::Node old_value_;
    YAML::Node new_value_;
};

} // namespace buddd::editor

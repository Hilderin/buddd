#pragma once

#include "command.h"
#include "editor.h"
#include "editor_context.h"
#include "editor_selection.h"

#include "log/log.h"
#include "scene/entity.h"
#include "scene/world.h"
#include "scene/component_registry/component_registry.h"
#include "scene/component_registry/component_info.h"
#include "scene/component_registry/serialization_context.h"

#include <yaml-cpp/yaml.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>

namespace buddd::editor {

/// Command that removes a component from an entity at a specific index.
/// Serializes the full component state before removal, and restores it
/// on undo via insert_component_raw_at().
class RemoveComponentCommand final : public Command {
public:
    RemoveComponentCommand(buddd::engine::EntityId entity_id, std::string component_type_name, size_t component_index)
        : entity_id_(entity_id)
        , component_type_name_(std::move(component_type_name))
        , component_index_(component_index)
    {}

    auto execute(EditorContext const& ctx) -> void override {
        pre_execution_selection_ = ctx.editor.selection().snapshot();

        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);
        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "RemoveComponentCommand: entity {} not found", entity_id_.index);
            return;
        }

        auto& registry = ctx.engine.services.registry();
        const auto* info = registry.describe(component_type_name_);
        if (!info) {
            BUDDD_LOG_TAGGED_ERROR("Editor:Command",
                "RemoveComponentCommand: unregistered type '{}'", component_type_name_);
            return;
        }

        // Safety check: verify the component at the stored index matches the expected type
        if (component_index_ >= entity.component_count()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "RemoveComponentCommand: index {} out of bounds for entity {} ({} components)",
                component_index_, entity_id_.index, entity.component_count());
            return;
        }

        // Build expected type_index from info (SceneSaver pattern)
        auto tmp = const_cast<buddd::engine::ComponentInfoBase*>(info)->create();
        auto expected_type = std::type_index(typeid(*tmp));

        auto& comp = entity.component_at(component_index_);
        auto actual_type = std::type_index(typeid(comp));
        if (expected_type != actual_type) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "RemoveComponentCommand: type mismatch at index {} for entity {}: "
                "expected '{}', found '{}'",
                component_index_, entity_id_.index, component_type_name_,
                actual_type.name());
            return;
        }

        // Serialize full component state
        auto ser_ctx = buddd::engine::SerializationContext{ctx.engine.services.assets()};
        auto state = info->serialize(comp, ser_ctx);
        serialized_state_ = YAML::Clone(state);

        // Remove the component
        world.remove_component_at(entity_id_, component_index_);

        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "RemoveComponent: entity={} type={} index={}",
            entity_id_.index, component_type_name_, component_index_);
    }

    auto undo(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);
        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "RemoveComponentCommand UNDO: entity {} not found", entity_id_.index);
            return;
        }

        auto& registry = ctx.engine.services.registry();
        const auto* info = registry.describe(component_type_name_);
        if (!info) {
            BUDDD_LOG_TAGGED_ERROR("Editor:Command",
                "RemoveComponentCommand UNDO: unregistered type '{}'", component_type_name_);
            return;
        }

        // Create a fresh component
        auto comp_result = registry.create(component_type_name_);
        if (!comp_result) {
            BUDDD_LOG_TAGGED_ERROR("Editor:Command",
                "RemoveComponentCommand UNDO: failed to create component of type '{}': {}",
                component_type_name_, comp_result.error().message);
            return;
        }

        // Deserialize stored state into the new component
        auto ser_ctx = buddd::engine::SerializationContext{ctx.engine.services.assets()};
        auto deser_result = info->deserialize(**comp_result, serialized_state_, ser_ctx);
        if (!deser_result) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "RemoveComponentCommand UNDO: deserialize failed for '{}' on entity {}: {}",
                component_type_name_, entity_id_.index, deser_result.error().message);
            // Continue — component will be attached with default properties
        }

        // Insert at the original position
        world.insert_component_raw_at(entity_id_, component_index_, std::move(*comp_result));

        ctx.editor.selection().restore(pre_execution_selection_);
        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "RemoveComponent UNDO: entity={} type={} index={}",
            entity_id_.index, component_type_name_, component_index_);
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Remove Component";
    }

    [[nodiscard]] auto try_update_new_value(YAML::Node const&, EditorContext const&, std::string_view) -> bool override {
        return false;
    }

private:
    buddd::engine::EntityId entity_id_;
    std::string component_type_name_;
    size_t component_index_;
    YAML::Node serialized_state_;
    Selection pre_execution_selection_;
};

} // namespace buddd::editor

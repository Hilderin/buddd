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

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <typeindex>

namespace buddd::editor {

/// Command that adds a component of a given type name to an entity.
/// The component is created via ComponentRegistry::create() and attached
/// at the back of the entity's component vector via World::add_component_raw().
/// On undo, removes the component at the stored index.
class AddComponentCommand final : public Command {
public:
    AddComponentCommand(buddd::engine::EntityId entity_id, std::string component_type_name)
        : entity_id_(entity_id)
        , component_type_name_(std::move(component_type_name))
    {}

    auto execute(EditorContext const& ctx) -> void override {
        pre_execution_selection_ = ctx.editor.selection().snapshot();

        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);
        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "AddComponentCommand: entity {} not found", entity_id_.index);
            return;
        }

        auto& registry = ctx.engine.services.registry();
        const auto* info = registry.describe(component_type_name_);
        if (!info) {
            BUDDD_LOG_TAGGED_ERROR("Editor:Command",
                "AddComponentCommand: unregistered type '{}'", component_type_name_);
            return;
        }

        auto comp_result = registry.create(component_type_name_);
        if (!comp_result) {
            BUDDD_LOG_TAGGED_ERROR("Editor:Command",
                "AddComponentCommand: failed to create component of type '{}': {}",
                component_type_name_, comp_result.error().message);
            return;
        }

        // Record component count BEFORE adding — the new component will be at this index
        size_t count_before = entity.component_count();

        world.add_component_raw(entity_id_, std::move(*comp_result));

        // Store the index of the newly added component (always at the back)
        component_index_ = count_before;

        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "AddComponent: entity={} type={} index={}",
            entity_id_.index, component_type_name_, *component_index_);
    }

    auto undo(EditorContext const& ctx) -> void override {
        auto& world = ctx.editor.world();
        auto entity = world.entity(entity_id_);
        if (entity.id() == buddd::engine::EntityId::none()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "AddComponentCommand UNDO: entity {} not found", entity_id_.index);
            return;
        }

        if (!component_index_.has_value()) {
            BUDDD_LOG_TAGGED_WARN("Editor:Command",
                "AddComponentCommand UNDO: no component index stored for entity {}",
                entity_id_.index);
            return;
        }

        world.remove_component_at(entity_id_, *component_index_);

        ctx.editor.selection().restore(pre_execution_selection_);
        ctx.editor.mark_dirty();

        BUDDD_LOG_TAGGED_DEBUG("Editor:Command",
            "AddComponent UNDO: entity={} type={}", entity_id_.index, component_type_name_);
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Add Component";
    }

    [[nodiscard]] auto try_update_new_value(YAML::Node const&, EditorContext const&, std::string_view) -> bool override {
        return false;
    }

private:
    buddd::engine::EntityId entity_id_;
    std::string component_type_name_;
    std::optional<size_t> component_index_;
    Selection pre_execution_selection_;
};

} // namespace buddd::editor

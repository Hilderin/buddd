#pragma once

#include <string_view>

// Forward declaration for try_update_new_value parameter
namespace YAML {
class Node;
}

namespace buddd::editor {

struct EditorContext;

/// Abstract base for all editor commands.
/// Lifecycle: construct -> execute() -> (undo() | execute())...
class Command {
public:
    virtual ~Command() = default;

    /// Execute (or re-execute) the command.
    virtual auto execute(EditorContext const& ctx) -> void = 0;

    /// Undo the command. Only called after execute().
    virtual auto undo(EditorContext const& ctx) -> void = 0;

    /// Human-readable name for menu display (e.g., "Quit").
    [[nodiscard]] virtual auto name() const -> std::string_view = 0;

    /// Attempt to update this command's new value from incoming state.
    /// @param new_value     The latest value (used by SetComponentPropertyCommand).
    /// @param ctx           Editor context (used by SetTransformCommand to read current entity transform).
    /// @param property_name Property name for cross-property merge guard (empty for non-property commands).
    /// @return true if the command accepted the update (caller should NOT push a new command).
    [[nodiscard]] virtual auto try_update_new_value(YAML::Node const& new_value,
                                                     EditorContext const& ctx,
                                                     std::string_view property_name = {}) -> bool;
};

inline auto Command::try_update_new_value(YAML::Node const&, EditorContext const&,
                                           std::string_view) -> bool {
    return false;
}

} // namespace buddd::editor

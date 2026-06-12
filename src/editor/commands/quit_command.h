#pragma once

#include "command.h"

#include "editor_context.h"

#include <string_view>

namespace buddd::editor {

/// Command that requests the engine to exit.
/// execute() calls ctx.engine.request_exit(). undo() is a no-op.
class QuitCommand final : public Command {
public:
    QuitCommand() = default;

    auto execute(EditorContext const& ctx) -> void override {
        ctx.engine.request_exit();
    }

    auto undo(EditorContext const& /*ctx*/) -> void override {
        // No-op: cannot un-request exit
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Quit";
    }
};

} // namespace buddd::editor

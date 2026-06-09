#pragma once

#include "command.h"

#include "engine_context.h"

#include <string_view>

namespace buddd::editor {

/// Command that requests the engine to exit.
/// execute() calls ctx->request_exit(). undo() is a no-op.
class QuitCommand final : public Command {
public:
    explicit QuitCommand(buddd::engine::EngineContext const& ctx)
        : ctx_(&ctx) {}

    auto execute() -> void override {
        ctx_->request_exit();
    }

    auto undo() -> void override {
        // No-op: cannot un-request exit
    }

    [[nodiscard]] auto name() const -> std::string_view override {
        return "Quit";
    }

private:
    buddd::engine::EngineContext const* ctx_;
};

} // namespace buddd::editor

#include "debug/assert.h"
#include "debug/debug_break.h"
#include "log/log.h"

#include <format>
#include <string>

namespace buddd::engine {

auto format_assertion_failure_message(
    std::string_view expr,
    std::string_view file,
    int line,
    std::string_view function,
    std::optional<std::string> message
) -> std::string
{
    std::string result;
    result += std::format("Assertion failed: {}\n", expr);
    if (message.has_value() && !message->empty()) {
        result += std::format("Message: {}\n", *message);
    }
    result += std::format("Location: {}:{}\n", file, line);
    result += std::format("Function: {}", function);
    return result;
}

void handle_assertion_failure(
    std::string_view expr,
    std::string_view file,
    int line,
    std::string_view function,
    std::optional<std::string> message
)
{
    auto formatted = format_assertion_failure_message(expr, file, line, function, std::move(message));

    auto& logger = ::buddd::log::Logger::instance();
    logger.log(
        ::buddd::log::LogLevel::Fatal,
        "Assert",
        file, line, function,
        "{}", formatted
    );

    ::buddd::engine::debug_break();
    std::abort();
}

} // namespace buddd::engine

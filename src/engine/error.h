#pragma once

#include <expected>
#include <string>
#include <utility>

namespace buddd::engine {

struct Error {
    enum class Category {
        InitFailed,
        WindowCreationFailed,
        RenderDeviceCreationFailed,
        Unsupported,
        Unknown
    };

    Category category{Category::Unknown};
    int code{0};
    std::string message;

    Error() = default;
    Error(Category cat, int c, std::string msg)
        : category{cat}, code{c}, message{std::move(msg)} {}
};

inline auto to_string(const Error& error) -> std::string {
    std::string category_str;
    switch (error.category) {
        case Error::Category::InitFailed:                category_str = "InitFailed"; break;
        case Error::Category::WindowCreationFailed:      category_str = "WindowCreationFailed"; break;
        case Error::Category::RenderDeviceCreationFailed: category_str = "RenderDeviceCreationFailed"; break;
        case Error::Category::Unsupported:               category_str = "Unsupported"; break;
        case Error::Category::Unknown:                   category_str = "Unknown"; break;
    }
    return category_str + ": " + error.message + " (code " + std::to_string(error.code) + ")";
}

/// Creates a `std::unexpected<Error>` for use as a return value from Result<T> functions.
/// @param category The error category.
/// @param message  Human-readable error description.
/// @param code     Backend-specific numeric error code (default 0).
inline auto make_error(Error::Category category, std::string message, int code = 0) -> std::unexpected<Error> {
    return std::unexpected<Error>(Error{category, code, std::move(message)});
}

template<typename T>
using Result = std::expected<T, Error>;

} // namespace buddd::engine

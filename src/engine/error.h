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
        ShaderCompilationFailed,
        LinkingFailed,
        ResourceCreationFailed,
        InvalidArgument,
        UniformNotFound,
        ReadbackFailed,    // Framebuffer readback (glReadPixels) failure
        TextureCreationFailed,
        IoFailed,          // File I/O error (read/write image file)
        InputInitFailed,   // Input system initialisation failure
        Unsupported,
        InvalidFormat,  // corrupt/invalid file format (e.g., malformed glTF)
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
        case Error::Category::ShaderCompilationFailed:    category_str = "ShaderCompilationFailed"; break;
        case Error::Category::LinkingFailed:              category_str = "LinkingFailed"; break;
        case Error::Category::ResourceCreationFailed:     category_str = "ResourceCreationFailed"; break;
        case Error::Category::InvalidArgument:            category_str = "InvalidArgument"; break;
        case Error::Category::UniformNotFound:            category_str = "UniformNotFound"; break;
        case Error::Category::ReadbackFailed:             category_str = "ReadbackFailed"; break;
        case Error::Category::TextureCreationFailed:       category_str = "TextureCreationFailed"; break;
        case Error::Category::IoFailed:                   category_str = "IoFailed"; break;
        case Error::Category::InputInitFailed:            category_str = "InputInitFailed"; break;
        case Error::Category::Unsupported:                category_str = "Unsupported"; break;
        case Error::Category::InvalidFormat:              category_str = "InvalidFormat"; break;
        case Error::Category::Unknown:                    category_str = "Unknown"; break;
    }
    return category_str + ": " + error.message + " (code " + std::to_string(error.code) + ")";
}

template<typename T>
using Result = std::expected<T, Error>;

/// Creates a `std::unexpected<Error>` for use as a return value from Result<T> functions.
/// @param category The error category.
/// @param message  Human-readable error description.
/// @param code     Backend-specific numeric error code (default 0).
inline auto make_error(Error::Category category, std::string message, int code = 0) -> std::unexpected<Error> {
    return std::unexpected<Error>(Error{category, code, std::move(message)});
}

/// Propagates an existing Error by wrapping it in std::unexpected<Error>.
/// Enables: `return make_error(vs.error())` instead of `return std::unexpected(vs.error())`.
inline auto make_error(const Error& error) -> std::unexpected<Error> {
    return std::unexpected<Error>(error);
}

/// Propagates a failed Result<T> by extracting its error and wrapping it.
/// Enables: `return make_error(vs)` instead of `return std::unexpected(vs.error())`.
template<typename T>
inline auto make_error(const std::expected<T, Error>& result) -> std::unexpected<Error> {
    return std::unexpected<Error>(result.error());
}

} // namespace buddd::engine

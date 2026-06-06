#pragma once

namespace buddd::engine {

inline void debug_break() {
#ifndef NDEBUG
    #if defined(_MSC_VER)
        __debugbreak();
    #elif defined(__clang__) || defined(__GNUC__)
        __builtin_trap();
    #else
        // Fallback — platform not supported
    #endif
#endif
}

} // namespace buddd::engine

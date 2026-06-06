#include "console_sink.h"

#include <cstdio>

namespace buddd::log {

static auto level_name(LogLevel level) -> const char* {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

void ConsoleSink::write(const LogMessage& message) {
    std::fprintf(stderr, "[%s] [%.*s] %s\n",
                 level_name(message.level),
                 static_cast<int>(message.tag.length()), message.tag.data(),
                 message.message.c_str());
}

} // namespace buddd::log

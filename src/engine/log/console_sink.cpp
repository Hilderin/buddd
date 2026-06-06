#include "console_sink.h"

#include <chrono>
#include <cstdio>
#include <ctime>

namespace buddd::log {

static auto level_name(LogLevel level) -> const char* {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
    }
    return "UNKNOWN";
}

void ConsoleSink::write(const LogMessage& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count() % 1000;

    struct tm local_tm;
    localtime_r(&time_t_now, &local_tm);

    char timestamp[64];
    std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &local_tm);

    std::fprintf(stderr, "[%s.%03d] [%s] [%.*s] %s\n",
                 timestamp, static_cast<int>(ms),
                 level_name(message.level),
                 static_cast<int>(message.tag.length()), message.tag.data(),
                 message.message.c_str());
}

} // namespace buddd::log

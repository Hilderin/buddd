#pragma once

#include "log/log.h"

#include <memory>

namespace buddd::test {

/// RAII helper that creates a MemorySink, configures the Logger with it,
/// and resets the Logger on destruction.
/// All log messages are captured at Trace level (most verbose) for test assertions.
struct ScopedMemoryLogger {
    std::shared_ptr<buddd::log::MemorySink> sink;

    ScopedMemoryLogger() {
        sink = std::make_shared<buddd::log::MemorySink>();
        buddd::log::LogConfig config;
        config.sinks.push_back(sink);
        // Set global level to Trace to capture all messages in tests
        config.global_min_level = buddd::log::LogLevel::Trace;
        buddd::log::Logger::init(std::move(config));
    }

    ~ScopedMemoryLogger() {
        buddd::log::Logger::reset();
    }
};

} // namespace buddd::test

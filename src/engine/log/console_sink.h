#pragma once

#include "log.h"

namespace buddd::log {

/// Console sink that writes log messages to stderr.
/// Format: [HH:MM:SS.fff] [LEVEL] [Tag] message\n
/// Wall-clock timestamp with millisecond precision, no color.
class ConsoleSink : public Sink {
public:
    void write(const LogMessage& message) override;
};

} // namespace buddd::log

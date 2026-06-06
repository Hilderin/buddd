#pragma once

#include "log.h"

namespace buddd::log {

/// Console sink that writes log messages to stderr.
/// Format: [LEVEL] [Tag] message\n
/// No timestamp, no color.
class ConsoleSink : public Sink {
public:
    void write(const LogMessage& message) override;
};

} // namespace buddd::log

#pragma once

#include "log.h"

#include <vector>

namespace buddd::log {

/// Memory sink that accumulates log messages in a vector for test assertions,
/// diagnostics, or any use case requiring in-process log capture.
/// Not thread-safe.
class MemorySink : public Sink {
public:
    void write(const LogMessage& message) override { messages_.push_back(message); }
    [[nodiscard]] auto messages() const -> const std::vector<LogMessage>& { return messages_; }
    void clear() { messages_.clear(); }
private:
    std::vector<LogMessage> messages_;
};

} // namespace buddd::log

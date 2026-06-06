#pragma once

#include "log.h"

#include <vector>

#ifdef BUDDD_TESTING

namespace buddd::log {

/// Memory sink that accumulates log messages in a vector for test assertions.
/// Only compiled when BUDDD_TESTING is defined.
/// Not thread-safe (test-only, single-threaded test environment).
class MemorySink : public Sink {
public:
    void write(const LogMessage& message) override { messages_.push_back(message); }
    [[nodiscard]] auto messages() const -> const std::vector<LogMessage>& { return messages_; }
    void clear() { messages_.clear(); }
private:
    std::vector<LogMessage> messages_;
};

} // namespace buddd::log

#endif // BUDDD_TESTING

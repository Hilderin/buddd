#pragma once

#include "log.h"

#include <memory>
#include <string_view>

namespace buddd::log {

/// File sink that writes log messages to a file in append mode.
/// Format: YYYY-MM-DDTHH:MM:SS [LEVEL] [Tag] message\n
///
/// Constructed via the static create() factory which returns nullptr on failure.
class FileSink : public Sink {
public:
    /// Destructor declared here, defined in .cpp where Impl is complete.
    ~FileSink() override;

    /// Attempts to open file_path in append mode.
    /// On success returns a unique_ptr to the FileSink.
    /// On failure writes a raw warning to stderr via write(2) and returns nullptr.
    [[nodiscard]] static auto create(std::string_view file_path) -> std::unique_ptr<FileSink>;
    void write(const LogMessage& message) override;
private:
    explicit FileSink(std::string_view file_path); // private — constructed by create()
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace buddd::log

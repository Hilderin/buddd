#include "file_sink.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <mutex>
#include <string>
#include <unistd.h>

namespace buddd::log {

// ---------------------------------------------------------------------------
// FileSink::Impl — holds the ofstream for writing
// ---------------------------------------------------------------------------
struct FileSink::Impl {
    std::ofstream stream;
    std::mutex file_mutex; // protects the stream (though Logger already serializes, belt-and-suspenders)
    explicit Impl(std::string_view path)
        : stream(std::string{path}.c_str(), std::ios::app)
    {
    }
};

// ---------------------------------------------------------------------------
// Factory: create()
// ---------------------------------------------------------------------------
auto FileSink::create(std::string_view file_path) -> std::unique_ptr<FileSink> {
    // Attempt to open the file to verify it's writable
    auto impl = std::make_unique<Impl>(file_path);
    if (!impl->stream.is_open()) {
        // Write raw warning to stderr via write(2) — logger may not be initialised
        std::string warning = "[WARN] [Log] Failed to open log file: ";
        warning += file_path;
        warning += "\n";
        // Use POSIX write for low-level output
        [[maybe_unused]] auto result = ::write(STDERR_FILENO, warning.data(), warning.size());
        return nullptr;
    }
    return std::unique_ptr<FileSink>(new FileSink(file_path));
}

// ---------------------------------------------------------------------------
// Destructor — defined here where Impl is complete
// ---------------------------------------------------------------------------
FileSink::~FileSink() = default;

// ---------------------------------------------------------------------------
// Private constructor
// ---------------------------------------------------------------------------
FileSink::FileSink(std::string_view file_path)
    : impl_(std::make_unique<Impl>(file_path))
{
}

// ---------------------------------------------------------------------------
// Helper: level name string
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// write() — format and write a log line to the file
// ---------------------------------------------------------------------------
void FileSink::write(const LogMessage& message) {
    if (!impl_ || !impl_->stream.is_open()) return;

    // Generate ISO 8601 timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    struct tm local_tm;
    localtime_r(&time_t_now, &local_tm);

    char timestamp[64];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &local_tm);

    // Write the formatted line
    std::lock_guard<std::mutex> lock(impl_->file_mutex);
    impl_->stream << timestamp << " ["
                  << level_name(message.level) << "] ["
                  << message.tag << "] "
                  << message.message << std::endl;
}

} // namespace buddd::log

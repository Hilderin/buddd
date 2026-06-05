#pragma once

#include "error.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace buddd::engine {

enum class FileEventType {
    Created,
    Modified,
    Deleted
};

struct FileEvent {
    std::string path;
    FileEventType type;
};

class FileWatcher {
public:
    [[nodiscard]] static auto create(std::string_view watch_path)
        -> Result<std::unique_ptr<FileWatcher>>;

    virtual ~FileWatcher();

    virtual auto poll_events() -> std::vector<FileEvent> = 0;
    virtual auto start() -> void = 0;
    virtual auto stop() -> void = 0;

    FileWatcher(const FileWatcher&) = delete;
    auto operator=(const FileWatcher&) -> FileWatcher& = delete;
    FileWatcher(FileWatcher&&) = delete;
    auto operator=(FileWatcher&&) -> FileWatcher& = delete;

protected:
    FileWatcher() = default;
};

class NullFileWatcher final : public FileWatcher {
public:
    auto poll_events() -> std::vector<FileEvent> override { return {}; }
    auto start() -> void override {}
    auto stop() -> void override {}
};

} // namespace buddd::engine

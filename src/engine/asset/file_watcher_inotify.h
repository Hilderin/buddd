#pragma once

#include "asset/file_watcher.h"

#ifdef __linux__

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

namespace buddd::engine {

class InotifyFileWatcher final : public FileWatcher {
public:
    explicit InotifyFileWatcher(std::string_view watch_path);
    ~InotifyFileWatcher() override;

    auto poll_events() -> std::vector<FileEvent> override;
    auto start() -> void override;
    auto stop() -> void override;

    InotifyFileWatcher(const InotifyFileWatcher&) = delete;
    auto operator=(const InotifyFileWatcher&) -> InotifyFileWatcher& = delete;
    InotifyFileWatcher(InotifyFileWatcher&&) = delete;
    auto operator=(InotifyFileWatcher&&) -> InotifyFileWatcher& = delete;

private:
    auto watcher_thread_func() -> void;
    auto add_watch_recursive(const std::string& dir_path, const std::string& relative_path) -> void;

    int inotify_fd_{-1};
    std::unordered_map<int, std::string> watch_dirs_;
    int self_pipe_[2] = {-1, -1};
    std::thread watcher_thread_;
    std::mutex queue_mutex_;
    std::queue<FileEvent> event_queue_;
    std::atomic<bool> running_{false};
    std::string watch_path_;
};

} // namespace buddd::engine

#endif // __linux__

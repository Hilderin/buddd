#include "asset/file_watcher_inotify.h"

#ifdef __linux__

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <vector>

#include <climits>
#include <fcntl.h>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

namespace buddd::engine {

InotifyFileWatcher::InotifyFileWatcher(std::string_view watch_path)
    : watch_path_(watch_path)
{
    inotify_fd_ = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (inotify_fd_ == -1) {
        std::cerr << "[FileWatcher] inotify_init1 failed: "
                  << std::strerror(errno) << "\n";
        // Mark as invalid; start() will detect this.
        return;
    }

    // Recursively add watches for all subdirectories
    add_watch_recursive(std::string(watch_path), "");

    // Create self-pipe for wake-on-shutdown
    if (pipe2(self_pipe_, O_CLOEXEC | O_NONBLOCK) == -1) {
        self_pipe_[0] = -1;
        self_pipe_[1] = -1;
        std::cerr << "[FileWatcher] pipe2 failed: " << std::strerror(errno) << "\n";
    }
}

InotifyFileWatcher::~InotifyFileWatcher() {
    stop();
    if (inotify_fd_ != -1) {
        close(inotify_fd_);
        inotify_fd_ = -1;
    }
    if (self_pipe_[0] != -1) {
        close(self_pipe_[0]);
        close(self_pipe_[1]);
        self_pipe_[0] = -1;
        self_pipe_[1] = -1;
    }
    // watch_dirs_ watches are automatically removed when inotify_fd_ is closed
}

auto InotifyFileWatcher::add_watch_recursive(const std::string& dir_path, const std::string& relative_path) -> void {
    int wd = inotify_add_watch(inotify_fd_, dir_path.c_str(),
                               IN_CREATE | IN_MODIFY | IN_DELETE | IN_MOVED_TO);
    if (wd != -1) {
        watch_dirs_[wd] = relative_path;
    }

    // Recursively scan subdirectories
    try {
        for (auto& entry : std::filesystem::directory_iterator(dir_path)) {
            if (entry.is_directory()) {
                auto sub_dir = entry.path().string();
                auto sub_rel = relative_path.empty()
                    ? entry.path().filename().string()
                    : relative_path + "/" + entry.path().filename().string();
                add_watch_recursive(sub_dir, sub_rel);
            }
        }
    } catch (...) {
        // Ignore permission errors etc.
    }
}

auto InotifyFileWatcher::start() -> void {
    if (inotify_fd_ == -1) {
        std::cerr << "[FileWatcher] Cannot start: inotify fd invalid\n";
        return;
    }
    if (running_.exchange(true)) {
        return; // Already running
    }

    watcher_thread_ = std::thread(&InotifyFileWatcher::watcher_thread_func, this);

#ifndef NDEBUG
    std::cerr << "[FileWatcher] Monitoring: " << watch_path_ << "\n";
#endif
}

auto InotifyFileWatcher::stop() -> void {
    if (!running_.exchange(false)) {
        return; // Already stopped
    }

    // Wake up the watcher thread by writing to the self-pipe
    if (self_pipe_[1] != -1) {
        char c = 1;
        write(self_pipe_[1], &c, 1);
    }

    if (watcher_thread_.joinable()) {
        watcher_thread_.join();
    }

#ifndef NDEBUG
    std::cerr << "[FileWatcher] Stopped\n";
#endif
}

auto InotifyFileWatcher::poll_events() -> std::vector<FileEvent> {
    std::vector<FileEvent> events;
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!event_queue_.empty()) {
        events.push_back(std::move(event_queue_.front()));
        event_queue_.pop();
    }
    return events;
}

auto InotifyFileWatcher::watcher_thread_func() -> void {
    // Buffer for inotify events — large enough for multiple events
    std::vector<char> buffer(sizeof(struct inotify_event) + NAME_MAX + 1);
    struct pollfd fds[2];
    fds[0].fd = inotify_fd_;
    fds[0].events = POLLIN;
    fds[1].fd = self_pipe_[0];
    fds[1].events = POLLIN;

    while (running_.load()) {
        int ret = poll(fds, 2, 500); // 500ms timeout for wake-on-shutdown

        if (!running_.load()) break;

        if (ret < 0) {
            if (errno == EINTR) continue;
            std::cerr << "[FileWatcher] poll failed: " << std::strerror(errno) << "\n";
            break;
        }

        if (ret == 0) continue; // Timeout

        // Check self-pipe for shutdown signal
        if (fds[1].revents & POLLIN) {
            char c;
            read(self_pipe_[0], &c, 1);
            continue;
        }

        // Read inotify events
        if (fds[0].revents & POLLIN) {
            ssize_t len = read(inotify_fd_, buffer.data(), buffer.size());
            if (len <= 0) continue;

            size_t offset = 0;
            while (offset < static_cast<size_t>(len)) {
                auto* event = reinterpret_cast<struct inotify_event*>(buffer.data() + offset);

                if (event->len > 0) {
                    // Look up which directory this watch corresponds to
                    auto dir_it = watch_dirs_.find(event->wd);
                    std::string file_path;
                    if (dir_it != watch_dirs_.end() && !dir_it->second.empty()) {
                        file_path = dir_it->second + "/" + event->name;
                    } else {
                        file_path = event->name;
                    }

                    FileEventType type;
                    if (event->mask & IN_CREATE || event->mask & IN_MOVED_TO) {
                        type = FileEventType::Created;
                    } else if (event->mask & IN_MODIFY) {
                        type = FileEventType::Modified;
                    } else if (event->mask & IN_DELETE) {
                        type = FileEventType::Deleted;
                    } else {
                        offset += sizeof(struct inotify_event) + event->len;
                        continue;
                    }

                    FileEvent fe{std::move(file_path), type};
                    {
                        std::lock_guard<std::mutex> lock(queue_mutex_);
                        event_queue_.push(std::move(fe));
                    }
                }

                offset += sizeof(struct inotify_event) + event->len;
            }
        }
    }
}

} // namespace buddd::engine

#endif // __linux__

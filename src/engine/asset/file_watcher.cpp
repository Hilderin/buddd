#include "asset/file_watcher.h"

#ifdef __linux__
#include "asset/file_watcher_inotify.h"
#endif

namespace buddd::engine {

FileWatcher::~FileWatcher() = default;

auto FileWatcher::create(std::string_view watch_path)
    -> Result<std::unique_ptr<FileWatcher>>
{
    if (watch_path.empty()) {
        return make_error(Error::Category::InvalidArgument,
            "FileWatcher watch path must not be empty");
    }

#ifdef __linux__
    auto watcher = std::make_unique<InotifyFileWatcher>(watch_path);
    return std::unique_ptr<FileWatcher>(std::move(watcher));
#else
    (void)watch_path;
    return make_error(Error::Category::Unsupported,
        "FileWatcher is only supported on Linux");
#endif
}

} // namespace buddd::engine

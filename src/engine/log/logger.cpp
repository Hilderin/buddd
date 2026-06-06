#include "log_filter.h"
#include "logger.h"

#include <mutex>
#include <utility>
#include <vector>

namespace buddd::log {

// ---------------------------------------------------------------------------
// Logger::Impl — pimpl struct holding mutex, filter, sinks, and state
// ---------------------------------------------------------------------------
struct Logger::Impl {
    std::mutex mutex;
    LogFilter filter;
    std::vector<std::shared_ptr<Sink>> sinks;
    bool initialized = false;
    bool shutdown = false;
};

// ---------------------------------------------------------------------------
// Singleton instance (C++11 magic static — thread-safe)
// ---------------------------------------------------------------------------
auto Logger::instance() -> Logger& {
    static Logger logger;
    return logger;
}

// ---------------------------------------------------------------------------
// init(LogConfig) — configure the logger from a config struct
// Idempotent: second call is a no-op. Not thread-safe.
// ---------------------------------------------------------------------------
void Logger::init(LogConfig config) {
    auto& self = instance();
    if (self.impl_ && self.impl_->initialized) {
        return; // idempotent — no-op on second call
    }

    auto impl = std::make_unique<Impl>();
    impl->filter.set_global_level(config.global_min_level);
    impl->filter.set_tag_overrides(config.tag_overrides);
    impl->sinks = std::move(config.sinks);
    impl->initialized = true;
    impl->shutdown = false;
    self.impl_ = std::move(impl);
}

// ---------------------------------------------------------------------------
// shutdown() — release resources, prevent further logging. Not thread-safe.
// ---------------------------------------------------------------------------
void Logger::shutdown() {
    auto& self = instance();
    if (!self.impl_ || !self.impl_->initialized) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(self.impl_->mutex);
        self.impl_->sinks.clear();
        self.impl_->shutdown = true;
        self.impl_->initialized = false;
    }
}

// ---------------------------------------------------------------------------
// reset() — completely clear all state. Not thread-safe.
// ---------------------------------------------------------------------------
void Logger::reset() {
    auto& self = instance();
    self.impl_.reset(); // destroys Impl, including mutex, filter, sinks
}

// ---------------------------------------------------------------------------
// is_enabled(level, tag) — lock-free quick check
// ---------------------------------------------------------------------------
auto Logger::is_enabled(LogLevel level, std::string_view tag) const -> bool {
    if (!impl_ || !impl_->initialized || impl_->shutdown) {
        return false;
    }
    return impl_->filter.is_enabled(level, tag);
}

// ---------------------------------------------------------------------------
// write_to_sinks(msg) — mutex-guarded dispatch to all sinks
// ---------------------------------------------------------------------------
void Logger::write_to_sinks(const LogMessage& msg) {
    if (!impl_) return;

    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (impl_->shutdown) return; // silent drop after shutdown

    for (auto& sink : impl_->sinks) {
        sink->write(msg);
    }
}

} // namespace buddd::log

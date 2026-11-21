#include "cfg/Log.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace cfg::log {

namespace {

std::string lower(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

} // namespace

spdlog::level::level_enum to_spdlog_level(Level level) {
    switch (level) {
        case Level::Trace: return spdlog::level::trace;
        case Level::Debug: return spdlog::level::debug;
        case Level::Info: return spdlog::level::info;
        case Level::Warn: return spdlog::level::warn;
        case Level::Error: return spdlog::level::err;
        case Level::Critical: return spdlog::level::critical;
        case Level::Off: return spdlog::level::off;
    }
    return spdlog::level::info;
}

Level level_from_string(std::string_view value, Level fallback) {
    const std::string v = lower(value);
    if (v == "trace") return Level::Trace;
    if (v == "debug") return Level::Debug;
    if (v == "info") return Level::Info;
    if (v == "warn" || v == "warning") return Level::Warn;
    if (v == "error" || v == "err") return Level::Error;
    if (v == "critical" || v == "crit" || v == "fatal") return Level::Critical;
    if (v == "off" || v == "none") return Level::Off;
    return fallback;
}

std::shared_ptr<spdlog::logger> init(const Options& options) {
    try {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.reserve(2);

        if (options.console) {
            if (options.color) {
                auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                sink->set_pattern(options.pattern);
                sinks.push_back(std::move(sink));
            } else {
                auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
                sink->set_pattern(options.pattern);
                sinks.push_back(std::move(sink));
            }
        }

        if (!options.file_path.empty()) {
            auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(options.file_path, options.truncate_file);
            sink->set_pattern(options.pattern);
            sinks.push_back(std::move(sink));
        }

        std::shared_ptr<spdlog::logger> logger;
        if (sinks.empty()) {
            // Keep a valid logger even when no sinks are requested.
            auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            sink->set_pattern(options.pattern);
            logger = std::make_shared<spdlog::logger>(options.logger_name, std::move(sink));
            logger->set_level(spdlog::level::off);
        } else {
            logger = std::make_shared<spdlog::logger>(options.logger_name, sinks.begin(), sinks.end());
            logger->set_level(to_spdlog_level(options.level));
        }

        const auto flush_level = static_cast<Level>(
            std::min<std::size_t>(options.flush_on_level, static_cast<std::size_t>(Level::Off)));
        logger->flush_on(to_spdlog_level(flush_level));
        spdlog::set_default_logger(logger);
        spdlog::set_level(logger->level());
        return logger;
    } catch (const spdlog::spdlog_ex& ex) {
        // Fallback: avoid crashing application during logging setup.
        auto fallback = spdlog::default_logger();
        if (fallback) {
            fallback->error("Failed to initialize logger '{}': {}", options.logger_name, ex.what());
        }
        return fallback;
    }
}

std::shared_ptr<spdlog::logger> get() {
    auto logger = spdlog::default_logger();
    if (logger) {
        return logger;
    }
    return init();
}

void set_logger(std::shared_ptr<spdlog::logger> logger) {
    if (!logger) {
        return;
    }
    spdlog::set_default_logger(std::move(logger));
    spdlog::set_level(spdlog::default_logger()->level());
}

void set_level(Level level) {
    auto logger = get();
    logger->set_level(to_spdlog_level(level));
    spdlog::set_level(logger->level());
}

void shutdown() {
    spdlog::shutdown();
}

} // namespace cfg::log

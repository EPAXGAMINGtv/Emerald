#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

namespace cfg::log {

enum class Level {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off
};

struct Options {
    std::string logger_name = "emerald";
    Level level = Level::Info;
    bool console = true;
    bool color = true;
    std::string file_path;              // empty = no file sink
    bool truncate_file = false;
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v";
    std::size_t flush_on_level = static_cast<std::size_t>(Level::Info);
};

// Initializes the default project logger. Safe to call multiple times.
std::shared_ptr<spdlog::logger> init(const Options& options = {});

// Returns the active default logger, initializing a fallback logger if needed.
std::shared_ptr<spdlog::logger> get();

// Replaces the default logger.
void set_logger(std::shared_ptr<spdlog::logger> logger);

// Changes runtime log level on the default logger.
void set_level(Level level);

// Parses a level name: trace, debug, info, warn/warning, error, critical, off.
Level level_from_string(std::string_view value, Level fallback = Level::Info);

spdlog::level::level_enum to_spdlog_level(Level level);

// Flushes and drops all spdlog loggers.
void shutdown();

} // namespace cfg::log

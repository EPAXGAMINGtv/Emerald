#include "cfg/ConfigManager.hpp"

#include <stdexcept>
#include <utility>

#include <spdlog/spdlog.h>

namespace cfg {

ConfigManager::ConfigManager(std::filesystem::path base_dir)
    : base_dir_(std::move(base_dir)) {}

void ConfigManager::set_base_dir(std::filesystem::path base_dir) {
    base_dir_ = std::move(base_dir);
    for (auto& [name, entry] : entries_) {
        (void)name;
        if (!entry.file_name.is_absolute()) {
            entry.config.set_path(resolve(entry.file_name));
        }
    }
    for (auto& [name, watch] : watches_) {
        const Entry* entry = find_entry(name);
        if (entry) {
            watch.path = resolve(entry->file_name);
            watch.has_file = false;
        }
    }
}

const std::filesystem::path& ConfigManager::base_dir() const noexcept {
    return base_dir_;
}

Config& ConfigManager::add(
    std::string name,
    std::filesystem::path file_name,
    std::function<void(Config&)> setup
) {
    if (Entry* existing = find_entry(name)) {
        spdlog::warn("cfg::ConfigManager: '{}' is already registered", name);
        return existing->config;
    }

    Entry entry;
    entry.file_name = std::move(file_name);
    entry.config.set_path(resolve(entry.file_name));
    if (setup) {
        setup(entry.config);
    }

    const std::string key = std::move(name);
    auto [it, inserted] = entries_.emplace(key, std::move(entry));
    (void)inserted;
    return it->second.config;
}

bool ConfigManager::remove(std::string_view name) {
    const Entry* entry = find_entry(name);
    if (!entry) {
        return false;
    }
    entries_.erase(std::string(name));
    watches_.erase(std::string(name));
    return true;
}

Config* ConfigManager::find(std::string_view name) noexcept {
    Entry* entry = find_entry(name);
    return entry ? &entry->config : nullptr;
}

const Config* ConfigManager::find(std::string_view name) const noexcept {
    const Entry* entry = find_entry(name);
    return entry ? &entry->config : nullptr;
}

Config& ConfigManager::get(std::string_view name) {
    Config* config = find(name);
    if (!config) {
        throw std::out_of_range("cfg::ConfigManager: unknown config '" + std::string(name) + "'");
    }
    return *config;
}

const Config& ConfigManager::get(std::string_view name) const {
    const Config* config = find(name);
    if (!config) {
        throw std::out_of_range("cfg::ConfigManager: unknown config '" + std::string(name) + "'");
    }
    return *config;
}

bool ConfigManager::contains(std::string_view name) const noexcept {
    return find_entry(name) != nullptr;
}

std::vector<std::string> ConfigManager::names() const {
    std::vector<std::string> result;
    result.reserve(entries_.size());
    for (const auto& [name, entry] : entries_) {
        (void)entry;
        result.push_back(name);
    }
    return result;
}

bool ConfigManager::load(std::string_view name) {
    Entry* entry = find_entry(name);
    if (!entry) {
        spdlog::warn("cfg::ConfigManager: cannot load unknown config '{}'", name);
        return false;
    }
    return entry->config.load(resolve(entry->file_name));
}

bool ConfigManager::load_or_create(std::string_view name) {
    Entry* entry = find_entry(name);
    if (!entry) {
        spdlog::warn("cfg::ConfigManager: cannot load unknown config '{}'", name);
        return false;
    }
    return entry->config.load_or_create(resolve(entry->file_name));
}

bool ConfigManager::load_all() {
    bool ok = true;
    for (const auto& [name, entry] : entries_) {
        (void)entry;
        ok = load(name) && ok;
    }
    return ok;
}

bool ConfigManager::load_all_or_create() {
    bool ok = true;
    for (const auto& [name, entry] : entries_) {
        (void)entry;
        ok = load_or_create(name) && ok;
    }
    return ok;
}

bool ConfigManager::save(std::string_view name) const {
    const Entry* entry = find_entry(name);
    if (!entry) {
        spdlog::warn("cfg::ConfigManager: cannot save unknown config '{}'", name);
        return false;
    }
    return entry->config.save();
}

bool ConfigManager::save(std::string_view name, SaveMode mode) const {
    const Entry* entry = find_entry(name);
    if (!entry) {
        spdlog::warn("cfg::ConfigManager: cannot save unknown config '{}'", name);
        return false;
    }
    return entry->config.save(mode);
}

bool ConfigManager::save_all() const {
    bool ok = true;
    for (const auto& [name, entry] : entries_) {
        (void)entry;
        ok = save(name) && ok;
    }
    return ok;
}

bool ConfigManager::save_all(SaveMode mode) const {
    bool ok = true;
    for (const auto& [name, entry] : entries_) {
        (void)entry;
        ok = save(name, mode) && ok;
    }
    return ok;
}

bool ConfigManager::watch(
    std::string_view name,
    std::chrono::seconds interval,
    WatchCallback callback
) {
    const Entry* entry = find_entry(name);
    if (!entry || interval <= std::chrono::seconds::zero()) {
        return false;
    }

    Watch watch;
    watch.path = resolve(entry->file_name);
    watch.interval = interval;
    watch.next_check = std::chrono::steady_clock::now() + interval;
    watch.callback = std::move(callback);

    std::error_code stat_error;
    if (std::filesystem::exists(watch.path, stat_error) && !stat_error) {
        watch.last_write = std::filesystem::last_write_time(watch.path, stat_error);
        watch.has_file = !stat_error;
    }

    watches_.insert_or_assign(std::string(name), std::move(watch));
    return true;
}

bool ConfigManager::unwatch(std::string_view name) {
    return watches_.erase(std::string(name)) != 0;
}

bool ConfigManager::poll() {
    const auto now = std::chrono::steady_clock::now();
    bool any_changed = false;

    for (auto& [name, watch] : watches_) {
        if (now < watch.next_check) {
            continue;
        }
        watch.next_check = now + watch.interval;

        std::error_code stat_error;
        const bool file_exists = std::filesystem::exists(watch.path, stat_error);
        if (stat_error) {
            continue;
        }

        std::filesystem::file_time_type stamp{};
        if (file_exists) {
            stamp = std::filesystem::last_write_time(watch.path, stat_error);
            if (stat_error) {
                continue;
            }
        }

        const bool changed = file_exists != watch.has_file
            || (file_exists && stamp != watch.last_write);
        watch.has_file = file_exists;
        watch.last_write = stamp;
        if (!changed || !file_exists) {
            continue;
        }

        Entry* entry = find_entry(name);
        if (!entry || !entry->config.load(watch.path)) {
            continue;
        }
        any_changed = true;
        if (watch.callback) {
            watch.callback(name, entry->config);
        }
    }

    return any_changed;
}

ConfigManager::Entry* ConfigManager::find_entry(std::string_view name) noexcept {
    const auto it = entries_.find(name);
    return it == entries_.end() ? nullptr : &it->second;
}

const ConfigManager::Entry* ConfigManager::find_entry(
    std::string_view name
) const noexcept {
    const auto it = entries_.find(name);
    return it == entries_.end() ? nullptr : &it->second;
}

std::filesystem::path ConfigManager::resolve(
    const std::filesystem::path& file_name
) const {
    if (file_name.is_absolute() || base_dir_.empty()) {
        return file_name;
    }
    return base_dir_ / file_name;
}

} // namespace cfg

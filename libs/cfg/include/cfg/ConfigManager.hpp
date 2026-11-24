#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cfg/Config.hpp"

namespace cfg {

// Owns a set of named config files. The manager has no knowledge of any
// application domain: names, file names and defaults are provided by callers.
class ConfigManager {
public:
    explicit ConfigManager(std::filesystem::path base_dir = {});

    void set_base_dir(std::filesystem::path base_dir);
    const std::filesystem::path& base_dir() const noexcept;

    // Registers a named config. The setup callback runs immediately, so
    // defaults/validators are in place before load() is called.
    Config& add(
        std::string name,
        std::filesystem::path file_name,
        std::function<void(Config&)> setup = {}
    );
    bool remove(std::string_view name);

    Config* find(std::string_view name) noexcept;
    const Config* find(std::string_view name) const noexcept;
    Config& get(std::string_view name);
    const Config& get(std::string_view name) const;
    bool contains(std::string_view name) const noexcept;

    std::vector<std::string> names() const;

    bool load(std::string_view name);
    bool load_or_create(std::string_view name);
    bool load_all();
    bool load_all_or_create();

    bool save(std::string_view name) const;
    bool save(std::string_view name, SaveMode mode) const;
    bool save_all() const;
    bool save_all(SaveMode mode) const;

private:
    struct Entry {
        std::filesystem::path file_name;
        Config config;
    };

    Entry* find_entry(std::string_view name) noexcept;
    const Entry* find_entry(std::string_view name) const noexcept;
    std::filesystem::path resolve(const std::filesystem::path& file_name) const;

    std::filesystem::path base_dir_;
    std::map<std::string, Entry, std::less<>> entries_;
};

} // namespace cfg

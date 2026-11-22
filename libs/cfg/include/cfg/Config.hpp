#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace cfg {

// Simple YAML-backed config manager.
// - load/save from file
// - dotted key access: "server.port", "logging.level"
// - typed getters with defaults
// - set/remove/contains helpers
class Config {
public:
    explicit Config(std::filesystem::path path = {});

    // Loads YAML from path(). Returns false on IO/parse error.
    bool load();
    // Loads YAML from the given path and updates path().
    bool load(const std::filesystem::path& path);

    // Loads config from path(). If the file does not exist, creates it with the
    // current in-memory defaults. Returns true when config is ready to use.
    bool load_or_create();
    bool load_or_create(const std::filesystem::path& path);

    // Saves current config to path(). Creates parent directories when needed.
    bool save() const;
    // Saves current config to the given path and updates path().
    bool save(const std::filesystem::path& path);

    // Parses YAML text into the current config (does not change path()).
    bool load_from_string(std::string_view yaml_text);

    // Serializes current config as YAML text.
    std::string dump() const;

    // Resets to an empty mapping.
    void clear();

    bool contains(std::string_view key) const;
    bool remove(std::string_view key);

    // Generic typed access. Returns default_value when key is missing or invalid.
    template <typename T>
    T get(std::string_view key, T default_value) const {
        const YAML::Node node = lookup(key);
        if (!node || node.IsNull()) {
            return default_value;
        }
        try {
            return node.as<T>();
        } catch (const YAML::BadConversion&) {
            return default_value;
        }
    }

    std::string get_string(std::string_view key, std::string default_value = {}) const;
    bool get_bool(std::string_view key, bool default_value = false) const;
    int get_int(std::string_view key, int default_value = 0) const;
    double get_double(std::string_view key, double default_value = 0.0) const;
    std::vector<std::string> get_string_list(std::string_view key) const;

    template <typename T>
    std::optional<T> get_optional(std::string_view key) const {
        const YAML::Node node = lookup(key);
        if (!node || node.IsNull()) {
            return std::nullopt;
        }
        try {
            return node.as<T>();
        } catch (const YAML::BadConversion&) {
            return std::nullopt;
        }
    }

    template <typename T>
    void set(std::string_view key, const T& value) {
        YAML::Node node = ensure(key);
        node = value;
    }

    void set_string(std::string_view key, std::string_view value);

    const std::filesystem::path& path() const noexcept;
    void set_path(std::filesystem::path path);

    const YAML::Node& root() const noexcept;
    YAML::Node& root() noexcept;

private:
    static std::vector<std::string> split_key(std::string_view key);

    YAML::Node lookup(std::string_view key) const;
    YAML::Node ensure(std::string_view key);

    //const save() and save(path) share this write-only implementation ->
    bool writeToFile(const std::filesystem::path& path) const;

    std::filesystem::path path_;
    YAML::Node root_ = YAML::Node(YAML::NodeType::Map);
};

} // namespace cfg

#pragma once

#include <filesystem>
#include <functional>
#include <initializer_list>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace cfg {

// Controls what ends up on disk when save()/dump() is called.
enum class SaveMode {
    Effective, // defaults + user values, fully readable
    Overrides  // only keys explicitly set or loaded from file
};

template <typename T>
using Validator = std::function<bool(const T&)>;

struct Issue {
    std::string key;
    std::string message;
};

template <typename T>
Validator<T> range(T low, T high) {
    return [low, high](const T& value) {
        return value >= low && value <= high;
    };
}

Validator<std::string> one_of(std::initializer_list<std::string_view> choices);
Validator<std::string> non_empty();

// Simple YAML-backed config manager.
// - load/save from file
// - dotted key access: "server.port", "logging.level"
// - typed getters with defaults
// - set/remove/contains helpers
class Config {
public:
    explicit Config(std::filesystem::path path = {});

    // Loads YAML from path(). Returns false on IO/parse error and keeps the
    // current in-memory config untouched. Use load_or_create() to create a file.
    bool load();
    // Loads YAML from the given path and updates path().
    bool load(const std::filesystem::path& path);

    // Loads config from path(). If the file does not exist, creates it with the
    // current in-memory defaults. Returns true when config is ready to use.
    bool load_or_create();
    bool load_or_create(const std::filesystem::path& path);

    // Saves current config to path(). Creates parent directories when needed.
    bool save() const;
    // Saves with an explicit mode without changing the configured mode.
    bool save(SaveMode mode) const;
    // Saves current config to the given path and updates path().
    bool save(const std::filesystem::path& path);
    bool save(const std::filesystem::path& path, SaveMode mode);

    void set_save_mode(SaveMode mode) noexcept;
    SaveMode save_mode() const noexcept;

    // Parses YAML text into the current config (does not change path()).
    bool load_from_string(std::string_view yaml_text);

    // Serializes current config as YAML text.
    std::string dump() const;
    std::string dump(SaveMode mode) const;

    // Resets to an empty mapping.
    void clear();

    bool contains(std::string_view key) const;

    // Removes an explicit value so the registered default becomes effective again.
    bool reset(std::string_view key);
    // Hard-deletes a key from both values and registered defaults.
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
        YAML::Node node = ensure_node(root_, key);
        node = value;
        override_keys_.insert(normalize_key(key));
    }

    // Registers an in-memory default. It is used whenever the key is missing
    // from the loaded file and stays out of override-only saves.
    template <typename T>
    void set_default(std::string_view key, const T& value) {
        YAML::Node node;
        node = value;
        set_default_node(key, node);
    }

    // Registers a default plus an optional typed validator in one call.
    template <typename T>
    void define(std::string_view key, T value, Validator<T> validator = {}) {
        set_default(key, value);
        if (!validator) {
            return;
        }

        validators_.push_back({
            normalize_key(key),
            [validator = std::move(validator)](const YAML::Node& node) {
                try {
                    if (validator(node.as<T>())) {
                        return std::string{};
                    }
                } catch (const YAML::Exception&) {
                    return std::string{"value has the wrong type"};
                }
                return std::string{"value failed validation"};
            }
        });
    }

    // Returns all registered keys whose effective value violates validation.
    std::vector<Issue> validate() const;

    void set_string(std::string_view key, std::string_view value);

    const std::filesystem::path& path() const noexcept;
    void set_path(std::filesystem::path path);

    const YAML::Node& root() const noexcept;
    YAML::Node& root() noexcept;

private:
    struct RegisteredValidator {
        std::string key;
        std::function<std::string(const YAML::Node&)> check;
    };

    static std::vector<std::string> split_key(std::string_view key);
    static std::string normalize_key(std::string_view key);
    static YAML::Node ensure_node(YAML::Node& root, std::string_view key);
    static YAML::Node lookup_in(
        const YAML::Node& root, std::string_view key
    );
    static bool remove_at(YAML::Node& root, std::string_view key);
    static void collect_leaf_keys(
        const YAML::Node& node,
        std::string prefix,
        std::vector<std::string>& out
    );
    static void set_at(
        YAML::Node& root, std::string_view key, const YAML::Node& value
    );

    YAML::Node lookup(std::string_view key) const;
    YAML::Node build_output_node(SaveMode mode) const;
    void set_default_node(std::string_view key, const YAML::Node& value);
    void apply_defaults_to(YAML::Node& target) const;

    //const save() and save(path) share this write-only implementation ->
    bool writeToFile(const std::filesystem::path& path, SaveMode mode) const;

    std::filesystem::path path_;
    YAML::Node root_ = YAML::Node(YAML::NodeType::Map);
    std::map<std::string, YAML::Node, std::less<>> defaults_;
    std::set<std::string> override_keys_;
    std::vector<RegisteredValidator> validators_;
    SaveMode save_mode_ = SaveMode::Effective;
};

} // namespace cfg

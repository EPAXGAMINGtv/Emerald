#include "cfg/Config.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <utility>

#include <spdlog/spdlog.h>

namespace cfg {

Config::Config(std::filesystem::path path) : path_(std::move(path)) {}

bool Config::load() {
    return load(path_);
}

bool Config::load(const std::filesystem::path& path) {
    if (path.empty()) {
        spdlog::warn("cfg::Config::load called with empty path");
        return false;
    }

    try {
        if (!std::filesystem::exists(path)) {
            spdlog::warn("Config file does not exist: {}", path.string());
            return false;
        }

        YAML::Node parsed = YAML::LoadFile(path.string());
        if (!parsed || !parsed.IsMap()) {
            // Normalize non-map roots to a map so set/get_by_key stay predictable.
            YAML::Node map(YAML::NodeType::Map);
            if (parsed && !parsed.IsNull()) {
                map["value"] = parsed;
            }
            parsed = map;
        }

        root_ = std::move(parsed);
        override_keys_.clear();
        std::vector<std::string> loaded_keys;
        collect_leaf_keys(root_, std::string{}, loaded_keys);
        override_keys_.insert(loaded_keys.begin(), loaded_keys.end());
        apply_defaults_to(root_);
        path_ = path;

        spdlog::debug("Loaded config from {}", path.string());
        return true;
    } catch (const std::exception& e) {
        // Keep the last known-good in-memory config on IO/parse errors.
        spdlog::error("Failed to load config '{}': {}", path.string(), e.what());
        return false;
    }
}

bool Config::load_or_create() {
    return load_or_create(path_);
}

bool Config::load_or_create(const std::filesystem::path& path) {
    if (path.empty()) {
        spdlog::warn("cfg::Config::load_or_create called with empty path");
        return false;
    }

    if (std::filesystem::exists(path)) {
        return load(path);
    }

    spdlog::info("Config file does not exist, creating default: {}", path.string());
    if (!root_ || !root_.IsMap()) {
        root_ = YAML::Node(YAML::NodeType::Map);
    }
    apply_defaults_to(root_);
    return save(path);
}

bool Config::save() const {
    return writeToFile(path_, save_mode_);
}

bool Config::save(SaveMode mode) const {
    return writeToFile(path_, mode);
}

bool Config::save(const std::filesystem::path& path) {
    return save(path, save_mode_);
}

bool Config::save(const std::filesystem::path& path, SaveMode mode) {
    path_ = path;
    return writeToFile(path_, mode);
}

void Config::set_save_mode(SaveMode mode) noexcept {
    save_mode_ = mode;
}

SaveMode Config::save_mode() const noexcept {
    return save_mode_;
}

bool Config::writeToFile(
    const std::filesystem::path& path, SaveMode mode
) const {
    if (path.empty()) {
        spdlog::warn("cfg::Config::save called with empty path");
        return false;
    }

    try {
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        const std::filesystem::path temp = path.string() + ".tmp";
        auto remove_temp = [&temp]() {
            std::error_code ignored;
            std::filesystem::remove(temp, ignored);
        };

        const YAML::Node output = build_output_node(mode);
        YAML::Emitter emitter;
        emitter << output;
        if (!emitter.good()) {
            spdlog::error(
                "Failed to serialize config '{}': {}",
                path.string(), emitter.GetLastError()
            );
            return false;
        }

        std::ofstream out(temp, std::ios::out | std::ios::trunc);
        if (!out.is_open()) {
            spdlog::error("Failed to open config for writing: {}", temp.string());
            return false;
        }
        out << emitter.c_str() << '\n';
        out.flush();
        if (!out) {
            spdlog::error("Failed to write config: {}", temp.string());
            out.close();
            remove_temp();
            return false;
        }
        out.close();
        if (!out) {
            spdlog::error("Failed to close config: {}", temp.string());
            remove_temp();
            return false;
        }

        std::error_code rename_error;
        std::filesystem::rename(temp, path, rename_error);
        if (rename_error) {
            // Windows cannot rename over an existing file. Move the old file
            // aside first, then restore it if the replacement fails.
            const std::filesystem::path backup = path.string() + ".bak";
            std::filesystem::rename(path, backup, rename_error);
            if (!rename_error) {
                std::filesystem::rename(temp, path, rename_error);
                if (rename_error) {
                    std::filesystem::rename(backup, path, rename_error);
                } else {
                    std::filesystem::remove(backup, rename_error);
                }
            }
        }
        if (rename_error) {
            spdlog::error(
                "Failed to replace config '{}': {}",
                path.string(), rename_error.message()
            );
            remove_temp();
            return false;
        }

        spdlog::debug("Saved config to {}", path.string());
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to save config '{}': {}", path.string(), e.what());
        return false;
    }
}

bool Config::load_from_string(std::string_view yaml_text) {
    try {
        root_ = YAML::Load(std::string(yaml_text));
        if (!root_ || !root_.IsMap()) {
            YAML::Node map(YAML::NodeType::Map);
            if (root_ && !root_.IsNull()) {
                map["value"] = root_;
            }
            root_ = map;
        }
        override_keys_.clear();
        std::vector<std::string> parsed_keys;
        collect_leaf_keys(root_, std::string{}, parsed_keys);
        override_keys_.insert(parsed_keys.begin(), parsed_keys.end());
        apply_defaults_to(root_);
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse config string: {}", e.what());
        root_ = YAML::Node(YAML::NodeType::Map);
        override_keys_.clear();
        return false;
    }
}

std::string Config::dump() const {
    return dump(save_mode_);
}

std::string Config::dump(SaveMode mode) const {
    const YAML::Node output = build_output_node(mode);
    YAML::Emitter emitter;
    emitter << output;
    if (!emitter.good()) {
        return {};
    }
    return emitter.c_str();
}

void Config::clear() {
    root_ = YAML::Node(YAML::NodeType::Map);
    defaults_.clear();
    override_keys_.clear();
    validators_.clear();
}

std::vector<Issue> Config::validate() const {
    std::vector<Issue> issues;
    for (const auto& entry : validators_) {
        const YAML::Node value = lookup(entry.key);
        if (!value || value.IsNull()) {
            continue;
        }
        const std::string message = entry.check(value);
        if (!message.empty()) {
            issues.push_back({entry.key, message});
        }
    }
    return issues;
}

bool Config::contains(std::string_view key) const {
    const YAML::Node node = lookup(key);
    return node && !node.IsNull();
}

bool Config::reset(std::string_view key) {
    const std::string normalized = normalize_key(key);
    if (normalized.empty()) {
        return false;
    }

    remove_at(root_, key);

    const std::string child_prefix = normalized + ".";
    for (auto it = override_keys_.begin(); it != override_keys_.end();) {
        if (*it == normalized || it->starts_with(child_prefix)) {
            it = override_keys_.erase(it);
        } else {
            ++it;
        }
    }

    const auto default_it = defaults_.find(normalized);
    if (default_it == defaults_.end()) {
        return false;
    }
    set_at(root_, key, default_it->second);
    return true;
}

bool Config::remove(std::string_view key) {
    if (!remove_at(root_, key)) {
        return false;
    }

    const std::string normalized = normalize_key(key);
    const std::string child_prefix = normalized + ".";
    for (auto it = override_keys_.begin(); it != override_keys_.end();) {
        if (*it == normalized || it->starts_with(child_prefix)) {
            it = override_keys_.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = defaults_.begin(); it != defaults_.end();) {
        if (it->first == normalized || it->first.starts_with(child_prefix)) {
            it = defaults_.erase(it);
        } else {
            ++it;
        }
    }
    return true;
}

std::string Config::get_string(std::string_view key, std::string default_value) const {
    return get<std::string>(key, std::move(default_value));
}

bool Config::get_bool(std::string_view key, bool default_value) const {
    return get<bool>(key, default_value);
}

int Config::get_int(std::string_view key, int default_value) const {
    return get<int>(key, default_value);
}

double Config::get_double(std::string_view key, double default_value) const {
    return get<double>(key, default_value);
}

std::vector<std::string> Config::get_string_list(
    std::string_view key, std::vector<std::string> default_value
) const {
    const YAML::Node node = lookup(key);
    if (!node || !node.IsSequence()) {
        return default_value;
    }

    std::vector<std::string> out;
    out.reserve(node.size());
    for (const auto& item : node) {
        try {
            out.push_back(item.as<std::string>());
        } catch (const YAML::Exception&) {
            // Skip incompatible entries to keep the API simple and robust.
        }
    }
    return out;
}

void Config::set_string(std::string_view key, std::string_view value) {
    set<std::string>(key, std::string(value));
}

void Config::set_string_list(
    std::string_view key, const std::vector<std::string>& values
) {
    set<std::vector<std::string>>(key, values);
}

const std::filesystem::path& Config::path() const noexcept {
    return path_;
}

void Config::set_path(std::filesystem::path path) {
    path_ = std::move(path);
}

const YAML::Node& Config::root() const noexcept {
    return root_;
}

YAML::Node& Config::root() noexcept {
    return root_;
}

std::vector<std::string> Config::split_key(std::string_view key) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= key.size()) {
        const std::size_t dot = key.find('.', start);
        const std::size_t end = (dot == std::string_view::npos) ? key.size() : dot;
        if (end > start) {
            parts.emplace_back(key.substr(start, end - start));
        }
        if (dot == std::string_view::npos) {
            break;
        }
        start = dot + 1;
    }
    return parts;
}

YAML::Node Config::build_output_node(SaveMode mode) const {
    if (mode == SaveMode::Effective) {
        return YAML::Clone(root_);
    }

    YAML::Node output(YAML::NodeType::Map);
    for (const auto& key : override_keys_) {
        const YAML::Node value = lookup(key);
        if (!value) {
            continue;
        }
        set_at(output, key, value);
    }
    return output;
}

YAML::Node Config::lookup_in(
    const YAML::Node& root, std::string_view key
) {
    const auto parts = split_key(key);
    if (parts.empty()) {
        return {};
    }

    // Copying a YAML::Node is a cheap handle copy; YAML::Clone would deep-copy
    // the whole tree on every read.
    YAML::Node current = root;
    for (const auto& part : parts) {
        if (!current || !current.IsMap()) {
            return {};
        }
        current = current[part];
    }
    return current;
}

YAML::Node Config::lookup(std::string_view key) const {
    return lookup_in(root_, key);
}

std::string Config::normalize_key(std::string_view key) {
    const auto parts = split_key(key);
    std::string normalized;
    for (const auto& part : parts) {
        if (!normalized.empty()) {
            normalized += '.';
        }
        normalized += part;
    }
    return normalized;
}

YAML::Node Config::ensure_node(YAML::Node& root, std::string_view key) {
    const auto parts = split_key(key);
    if (parts.empty()) {
        return root;
    }

    if (!root || !root.IsMap()) {
        root = YAML::Node(YAML::NodeType::Map);
    }

    YAML::Node current = root;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const std::string& part = parts[i];
        const bool leaf = (i + 1 == parts.size());
        if (!current[part] || (!leaf && !current[part].IsMap())) {
            current[part] = YAML::Node(
                leaf ? YAML::NodeType::Null : YAML::NodeType::Map
            );
        }
        if (leaf) {
            return current[part];
        }
        current = current[part];
    }

    return root;
}

bool Config::remove_at(YAML::Node& root, std::string_view key) {
    const auto parts = split_key(key);
    if (parts.empty()) {
        return false;
    }

    YAML::Node current = root;
    for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
        if (!current || !current.IsMap()) {
            return false;
        }
        current = current[parts[i]];
    }

    if (!current || !current.IsMap()) {
        return false;
    }

    const std::string& leaf = parts.back();
    if (!current[leaf]) {
        return false;
    }
    current.remove(leaf);
    return true;
}

void Config::collect_leaf_keys(
    const YAML::Node& node,
    std::string prefix,
    std::vector<std::string>& out
) {
    if (!node) {
        return;
    }
    if (node.IsMap()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            const std::string child = prefix.empty()
                ? it->first.as<std::string>()
                : prefix + "." + it->first.as<std::string>();
            collect_leaf_keys(it->second, child, out);
        }
        return;
    }
    if (!prefix.empty()) {
        out.push_back(prefix);
    }
}

void Config::set_at(
    YAML::Node& root, std::string_view key, const YAML::Node& value
) {
    YAML::Node target = ensure_node(root, key);
    target = YAML::Clone(value);
}

void Config::set_default_node(
    std::string_view key, const YAML::Node& value
) {
    const std::string normalized = normalize_key(key);
    if (normalized.empty()) {
        return;
    }

    defaults_[normalized] = YAML::Clone(value);

    const YAML::Node existing = lookup(key);
    if (!existing || existing.IsNull()) {
        set_at(root_, key, defaults_[normalized]);
    }
}

void Config::apply_defaults_to(YAML::Node& target) const {
    for (const auto& [key, value] : defaults_) {
        const YAML::Node existing = lookup_in(target, key);
        if (!existing || existing.IsNull()) {
            set_at(target, key, value);
        }
    }
}

Validator<std::string> one_of(
    std::initializer_list<std::string_view> choices
) {
    std::vector<std::string> values;
    values.reserve(choices.size());
    for (const std::string_view choice : choices) {
        values.emplace_back(choice);
    }
    return [values = std::move(values)](const std::string& value) {
        return std::find(values.begin(), values.end(), value) != values.end();
    };
}

Validator<std::string> non_empty() {
    return [](const std::string& value) { return !value.empty(); };
}

} // namespace cfg

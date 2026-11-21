#include "cfg/Config.hpp"

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
    path_ = path;
    if (path_.empty()) {
        spdlog::warn("cfg::Config::load called with empty path");
        return false;
    }

    try {
        if (!std::filesystem::exists(path_)) {
            spdlog::info("Config file does not exist, creating default: {}", path_.string());
            root_ = YAML::Node(YAML::NodeType::Map);
            return save(path_);
        }

        root_ = YAML::LoadFile(path_.string());
        if (!root_ || !root_.IsMap()) {
            // Normalize non-map roots to a map so set/get_by_key remain predictable.
            YAML::Node map(YAML::NodeType::Map);
            if (root_ && !root_.IsNull()) {
                map["value"] = root_;
            }
            root_ = map;
        }

        spdlog::debug("Loaded config from {}", path_.string());
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to load config '{}': {}", path_.string(), e.what());
        root_ = YAML::Node(YAML::NodeType::Map);
        return false;
    }
}

bool Config::load_or_create() {
    return load_or_create(path_);
}

bool Config::load_or_create(const std::filesystem::path& path) {
    path_ = path;
    if (path_.empty()) {
        spdlog::warn("cfg::Config::load_or_create called with empty path");
        return false;
    }

    if (std::filesystem::exists(path_)) {
        return load(path_);
    }

    spdlog::info("Config file does not exist, creating default: {}", path_.string());
    if (!root_ || !root_.IsMap()) {
        root_ = YAML::Node(YAML::NodeType::Map);
    }
    return save(path_);
}

bool Config::save() const {
    return const_cast<Config*>(this)->save(path_);
}

bool Config::save(const std::filesystem::path& path) {
    path_ = path;
    if (path_.empty()) {
        spdlog::warn("cfg::Config::save called with empty path");
        return false;
    }

    try {
        if (path_.has_parent_path()) {
            std::filesystem::create_directories(path_.parent_path());
        }

        std::ofstream out(path_, std::ios::out | std::ios::trunc);
        if (!out.is_open()) {
            spdlog::error("Failed to open config for writing: {}", path_.string());
            return false;
        }

        YAML::Emitter emitter;
        emitter << root_;
        if (!emitter.good()) {
            spdlog::error("Failed to serialize config '{}': {}", path_.string(), emitter.GetLastError());
            return false;
        }

        out << emitter.c_str();
        out << '\n';
        spdlog::debug("Saved config to {}", path_.string());
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to save config '{}': {}", path_.string(), e.what());
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
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse config string: {}", e.what());
        root_ = YAML::Node(YAML::NodeType::Map);
        return false;
    }
}

std::string Config::dump() const {
    YAML::Emitter emitter;
    emitter << root_;
    if (!emitter.good()) {
        return {};
    }
    return emitter.c_str();
}

void Config::clear() {
    root_ = YAML::Node(YAML::NodeType::Map);
}

bool Config::contains(std::string_view key) const {
    const YAML::Node node = lookup(key);
    return node && !node.IsNull();
}

bool Config::remove(std::string_view key) {
    const auto parts = split_key(key);
    if (parts.empty()) {
        return false;
    }

    YAML::Node current = root_;
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

std::vector<std::string> Config::get_string_list(std::string_view key) const {
    std::vector<std::string> out;
    const YAML::Node node = lookup(key);
    if (!node || !node.IsSequence()) {
        return out;
    }

    out.reserve(node.size());
    for (const auto& item : node) {
        try {
            out.push_back(item.as<std::string>());
        } catch (const YAML::BadConversion&) {
            // Skip incompatible entries to keep the API simple and robust.
        }
    }
    return out;
}

void Config::set_string(std::string_view key, std::string_view value) {
    set<std::string>(key, std::string(value));
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

YAML::Node Config::lookup(std::string_view key) const {
    const auto parts = split_key(key);
    if (parts.empty()) {
        return {};
    }

    YAML::Node current = YAML::Clone(root_);
    for (const auto& part : parts) {
        if (!current || !current.IsMap()) {
            return {};
        }
        current = current[part];
    }
    return current;
}

YAML::Node Config::ensure(std::string_view key) {
    const auto parts = split_key(key);
    if (parts.empty()) {
        return root_;
    }

    if (!root_ || !root_.IsMap()) {
        root_ = YAML::Node(YAML::NodeType::Map);
    }

    YAML::Node current = root_;
    for (const auto& part : parts) {
        if (!current[part] || !current[part].IsMap()) {
            // If it exists but is scalar/sequence, overwrite with map unless it's the leaf.
            if (&part == &parts.back()) {
                if (!current[part]) {
                    current[part] = YAML::Node();
                }
                return current[part];
            }
            current[part] = YAML::Node(YAML::NodeType::Map);
        }
        if (&part == &parts.back()) {
            return current[part];
        }
        current = current[part];
    }

    return root_;
}

} // namespace cfg

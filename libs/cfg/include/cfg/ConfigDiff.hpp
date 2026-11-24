#pragma once

#include <map>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "cfg/Config.hpp"

namespace cfg {

// Structural comparison of two configurations. Nested maps are flattened into
// dotted keys; sequences are compared as one value per key.
class ConfigDiff {
public:
    struct Change {
        enum class Kind {
            Added,
            Removed,
            Changed
        };

        Kind kind = Kind::Changed;
        std::string key;
        YAML::Node before;
        YAML::Node after;
    };

    static std::vector<Change> compare(const Config& from, const Config& to);
    static std::vector<Change> compare(
        const YAML::Node& from,
        const YAML::Node& to,
        std::string prefix = {}
    );
    static bool equal(const Config& from, const Config& to);
    static std::string to_string(const std::vector<Change>& changes);

private:
    using Flat = std::map<std::string, YAML::Node, std::less<>>;

    static Flat flatten(const YAML::Node& node, const std::string& prefix);
    static std::string node_text(const YAML::Node& node);
    static std::string single_line(std::string text);
};

} // namespace cfg

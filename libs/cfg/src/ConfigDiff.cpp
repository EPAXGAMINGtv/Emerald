#include "cfg/ConfigDiff.hpp"

#include <algorithm>

#include <yaml-cpp/yaml.h>

namespace cfg {

ConfigDiff::Flat ConfigDiff::flatten(
    const YAML::Node& node, const std::string& prefix
) {
    Flat result;
    if (!node || node.IsNull()) {
        return result;
    }

    if (node.IsMap()) {
        for (const auto& entry : node) {
            std::string key;
            try {
                key = entry.first.as<std::string>();
            } catch (const YAML::Exception&) {
                continue;
            }
            if (key.empty()) {
                continue;
            }

            const std::string path = prefix.empty() ? key : prefix + "." + key;
            const Flat nested = flatten(entry.second, path);
            result.insert(nested.begin(), nested.end());
        }
        return result;
    }

    if (!prefix.empty()) {
        result.emplace(prefix, YAML::Clone(node));
    }
    return result;
}

std::string ConfigDiff::node_text(const YAML::Node& node) {
    YAML::Emitter emitter;
    emitter << node;
    return emitter.good() ? emitter.c_str() : std::string{};
}

std::string ConfigDiff::single_line(std::string text) {
    for (char& character : text) {
        if (character == '\n') {
            character = ' ';
        }
    }
    return text;
}

std::vector<ConfigDiff::Change> ConfigDiff::compare(
    const Config& from, const Config& to
) {
    return compare(from.root(), to.root(), std::string{});
}

std::vector<ConfigDiff::Change> ConfigDiff::compare(
    const YAML::Node& from, const YAML::Node& to, std::string prefix
) {
    const Flat before = flatten(from, prefix);
    const Flat after = flatten(to, prefix);

    std::vector<Change> changes;
    for (const auto& [key, value] : before) {
        const auto found = after.find(key);
        if (found == after.end()) {
            changes.push_back({Change::Kind::Removed, key, value, YAML::Node()});
        } else if (node_text(value) != node_text(found->second)) {
            changes.push_back(
                {Change::Kind::Changed, key, value, found->second}
            );
        }
    }

    for (const auto& [key, value] : after) {
        if (before.find(key) == before.end()) {
            changes.push_back({Change::Kind::Added, key, YAML::Node(), value});
        }
    }

    std::sort(
        changes.begin(),
        changes.end(),
        [](const Change& left, const Change& right) {
            return left.key < right.key;
        }
    );
    return changes;
}

bool ConfigDiff::equal(const Config& from, const Config& to) {
    return compare(from, to).empty();
}

std::string ConfigDiff::to_string(const std::vector<Change>& changes) {
    std::string result;

    for (const Change& change : changes) {
        switch (change.kind) {
            case Change::Kind::Added:
                result += "+ ";
                result += change.key;
                result += ": ";
                result += single_line(node_text(change.after));
                break;
            case Change::Kind::Removed:
                result += "- ";
                result += change.key;
                result += ": ";
                result += single_line(node_text(change.before));
                break;
            case Change::Kind::Changed:
                result += "~ ";
                result += change.key;
                result += ": ";
                result += single_line(node_text(change.before));
                result += " -> ";
                result += single_line(node_text(change.after));
                break;
            default:
                break;
        }
        result += '\n';
    }

    return result;
}

} // namespace cfg

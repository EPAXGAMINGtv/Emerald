#include "cfg/ConfigStack.hpp"

#include <algorithm>
#include <stdexcept>

namespace cfg {

namespace {

using Leaf = std::pair<std::string, YAML::Node>;

void collect_leaves(
    const YAML::Node& node,
    const std::string& prefix,
    std::vector<Leaf>& out
) {
    if (!node || node.IsNull()) {
        return;
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
            collect_leaves(entry.second, path, out);
        }
        return;
    }

    if (!prefix.empty()) {
        out.emplace_back(prefix, YAML::Clone(node));
    }
}

[[noreturn]] void throw_missing(std::string_view name) {
    throw std::out_of_range(
        "cfg::ConfigStack: no layer named " + std::string(name)
    );
}

} // namespace

Config& ConfigStack::add(std::string name, std::filesystem::path path) {
    Layer* existing = find_layer(name);
    if (existing != nullptr) {
        if (!path.empty()) {
            existing->config.set_path(std::move(path));
        }
        return existing->config;
    }

    auto layer = std::make_unique<Layer>();
    layer->name = std::move(name);
    layer->config.set_path(std::move(path));
    layers_.push_back(std::move(layer));
    return layers_.back()->config;
}

bool ConfigStack::remove(std::string_view name) {
    const auto found = std::find_if(
        layers_.begin(),
        layers_.end(),
        [name](const std::unique_ptr<Layer>& layer) {
            return layer->name == name;
        }
    );
    if (found == layers_.end()) {
        return false;
    }

    layers_.erase(found);
    return true;
}

Config* ConfigStack::find(std::string_view name) noexcept {
    Layer* layer = find_layer(name);
    return layer == nullptr ? nullptr : &layer->config;
}

const Config* ConfigStack::find(std::string_view name) const noexcept {
    const Layer* layer = find_layer(name);
    return layer == nullptr ? nullptr : &layer->config;
}

Config& ConfigStack::get(std::string_view name) {
    Config* config = find(name);
    if (config == nullptr) {
        throw_missing(name);
    }
    return *config;
}

const Config& ConfigStack::get(std::string_view name) const {
    const Config* config = find(name);
    if (config == nullptr) {
        throw_missing(name);
    }
    return *config;
}

bool ConfigStack::contains(std::string_view name) const noexcept {
    return find(name) != nullptr;
}

std::vector<std::string> ConfigStack::names() const {
    std::vector<std::string> result;
    result.reserve(layers_.size());
    for (const auto& layer : layers_) {
        result.push_back(layer->name);
    }
    return result;
}

std::size_t ConfigStack::size() const noexcept {
    return layers_.size();
}

void ConfigStack::clear() noexcept {
    layers_.clear();
}

bool ConfigStack::load_all() {
    bool ok = true;
    for (const auto& layer : layers_) {
        if (layer->config.path().empty()) {
            continue;
        }
        if (!layer->config.load()) {
            ok = false;
        }
    }
    return ok;
}

bool ConfigStack::save_layer(std::string_view name, SaveMode mode) {
    Config* config = find(name);
    if (config == nullptr) {
        return false;
    }
    return config->save(mode);
}

bool ConfigStack::save_all() {
    bool ok = true;
    for (const auto& layer : layers_) {
        if (layer->config.path().empty()) {
            continue;
        }
        if (!layer->config.save()) {
            ok = false;
        }
    }
    return ok;
}

Config ConfigStack::effective() const {
    Config result;

    for (const auto& layer : layers_) {
        std::vector<Leaf> leaves;
        collect_leaves(layer->config.root(), std::string{}, leaves);
        for (const Leaf& leaf : leaves) {
            result.set(leaf.first, leaf.second);
        }
    }

    return result;
}

ConfigStack::Layer* ConfigStack::find_layer(std::string_view name) noexcept {
    const auto found = std::find_if(
        layers_.begin(),
        layers_.end(),
        [name](const std::unique_ptr<Layer>& layer) {
            return layer->name == name;
        }
    );
    return found == layers_.end() ? nullptr : found->get();
}

const ConfigStack::Layer* ConfigStack::find_layer(
    std::string_view name
) const noexcept {
    const auto found = std::find_if(
        layers_.begin(),
        layers_.end(),
        [name](const std::unique_ptr<Layer>& layer) {
            return layer->name == name;
        }
    );
    return found == layers_.end() ? nullptr : found->get();
}

} // namespace cfg

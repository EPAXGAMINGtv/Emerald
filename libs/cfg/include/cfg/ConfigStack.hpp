#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cfg/Config.hpp"

namespace cfg {

// Ordered configuration layers. Later layers override earlier ones; the
// effective view is a new Config and never modifies the layers themselves.
class ConfigStack {
public:
    ConfigStack() = default;
    ConfigStack(const ConfigStack&) = delete;
    ConfigStack& operator=(const ConfigStack&) = delete;
    ConfigStack(ConfigStack&&) noexcept = default;
    ConfigStack& operator=(ConfigStack&&) noexcept = default;

    // Adds a layer at the end (highest priority). An existing name is returned
    // unchanged so callers can safely re-use setup code.
    Config& add(std::string name, std::filesystem::path path = {});
    bool remove(std::string_view name);

    Config* find(std::string_view name) noexcept;
    const Config* find(std::string_view name) const noexcept;
    Config& get(std::string_view name);
    const Config& get(std::string_view name) const;
    bool contains(std::string_view name) const noexcept;

    std::vector<std::string> names() const;
    std::size_t size() const noexcept;
    void clear() noexcept;

    // load_all()/save_all() skip layers without a path and report real IO errors.
    bool load_all();
    bool save_layer(std::string_view name, SaveMode mode = SaveMode::Effective);
    bool save_all();

    Config effective() const;

private:
    struct Layer {
        std::string name;
        Config config;
    };

    Layer* find_layer(std::string_view name) noexcept;
    const Layer* find_layer(std::string_view name) const noexcept;

    std::vector<std::unique_ptr<Layer>> layers_;
};

} // namespace cfg

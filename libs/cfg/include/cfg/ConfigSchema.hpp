#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "cfg/Config.hpp"

namespace cfg {

// Reusable, declarative description of the keys a Config is expected to hold.
// It can install defaults/validators and validate already loaded values without
// knowing anything about the application that uses them.
class ConfigSchema {
public:
    enum class Type {
        String,
        Integer,
        Real,
        Boolean,
        StringList
    };

    using Validator = std::function<bool(const YAML::Node&)>;

    // Adapts cfg::Validator<T> (e.g. range<int>()) for schema fields.
    template <typename T>
    static Validator typed(cfg::Validator<T> validator) {
        return [validator = std::move(validator)](const YAML::Node& node) {
            try {
                return validator(node.as<T>());
            } catch (const YAML::Exception&) {
                return false;
            }
        };
    }

    struct Field {
        std::string key;
        Type type = Type::String;
        YAML::Node default_value;
        std::string description;
        Validator validator;
    };

    ConfigSchema& string(
        std::string key,
        std::string default_value,
        std::string description = {},
        Validator validator = {}
    );
    ConfigSchema& integer(
        std::string key,
        int default_value,
        std::string description = {},
        Validator validator = {}
    );
    ConfigSchema& real(
        std::string key,
        double default_value,
        std::string description = {},
        Validator validator = {}
    );
    ConfigSchema& boolean(
        std::string key,
        bool default_value,
        std::string description = {},
        Validator validator = {}
    );
    ConfigSchema& string_list(
        std::string key,
        std::vector<std::string> default_value,
        std::string description = {},
        Validator validator = {}
    );

    // Returns false when the key is already registered or the default is empty.
    bool add(Field field);

    bool contains(std::string_view key) const noexcept;
    const Field* find(std::string_view key) const noexcept;
    const std::vector<Field>& fields() const noexcept;
    std::vector<std::string> keys() const;
    std::string documentation() const;

    // Registers every default and validator on the given Config.
    void apply_to(Config& config) const;

    // Reports schema violations in the Config's effective values.
    std::vector<Issue> validate(const Config& config) const;

private:
    static std::string type_name(Type type);
    static std::string describe_default(const YAML::Node& node);
    static std::string check_field(const Field& field, const YAML::Node& node);

    std::vector<Field> fields_;
};

} // namespace cfg

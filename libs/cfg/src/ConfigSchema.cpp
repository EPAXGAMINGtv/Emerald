#include "cfg/ConfigSchema.hpp"

#include <optional>
#include <utility>

namespace cfg {

namespace {

using SchemaField = ConfigSchema::Field;
using SchemaValidator = ConfigSchema::Validator;

YAML::Node make_node(const std::string& value) {
    YAML::Node node;
    node = value;
    return node;
}

YAML::Node make_node(int value) {
    YAML::Node node;
    node = value;
    return node;
}

YAML::Node make_node(double value) {
    YAML::Node node;
    node = value;
    return node;
}

YAML::Node make_node(bool value) {
    YAML::Node node;
    node = value;
    return node;
}

YAML::Node make_node(const std::vector<std::string>& value) {
    YAML::Node node;
    node = value;
    return node;
}

YAML::Node lookup_key(const YAML::Node& root, std::string_view key) {
    std::optional<YAML::Node> current;
    current.emplace(root);
    std::size_t start = 0U;

    while (start <= key.size()) {
        const std::size_t end = key.find('.', start);
        const std::string_view part = end == std::string_view::npos
            ? key.substr(start)
            : key.substr(start, end - start);

        const YAML::Node& view = *current;
        if (!view || !view.IsMap()) {
            return YAML::Node();
        }

        // Node assignment copies values into the same handle. Re-seat a fresh
        // handle on every step instead; copying a Node handle itself is cheap.
        YAML::Node child = view[std::string(part)];
        current.emplace(child);

        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1U;
    }

    return *current;
}

template <typename T>
void install_field(Config& config, const SchemaField& field) {
    T default_value{};
    try {
        default_value = field.default_value.as<T>();
    } catch (const YAML::Exception&) {
        return;
    }

    SchemaValidator validator = field.validator;
    config.define<T>(
        field.key,
        default_value,
        [validator = std::move(validator)](const T& value) {
            if (!validator) {
                return true;
            }
            YAML::Node node;
            node = value;
            return validator(node);
        }
    );
}

} // namespace

ConfigSchema& ConfigSchema::string(
    std::string key,
    std::string default_value,
    std::string description,
    Validator validator
) {
    Field field;
    field.key = std::move(key);
    field.type = Type::String;
    field.default_value = make_node(default_value);
    field.description = std::move(description);
    field.validator = std::move(validator);
    (void)add(std::move(field));
    return *this;
}

ConfigSchema& ConfigSchema::integer(
    std::string key,
    int default_value,
    std::string description,
    Validator validator
) {
    Field field;
    field.key = std::move(key);
    field.type = Type::Integer;
    field.default_value = make_node(default_value);
    field.description = std::move(description);
    field.validator = std::move(validator);
    (void)add(std::move(field));
    return *this;
}

ConfigSchema& ConfigSchema::real(
    std::string key,
    double default_value,
    std::string description,
    Validator validator
) {
    Field field;
    field.key = std::move(key);
    field.type = Type::Real;
    field.default_value = make_node(default_value);
    field.description = std::move(description);
    field.validator = std::move(validator);
    (void)add(std::move(field));
    return *this;
}

ConfigSchema& ConfigSchema::boolean(
    std::string key,
    bool default_value,
    std::string description,
    Validator validator
) {
    Field field;
    field.key = std::move(key);
    field.type = Type::Boolean;
    field.default_value = make_node(default_value);
    field.description = std::move(description);
    field.validator = std::move(validator);
    (void)add(std::move(field));
    return *this;
}

ConfigSchema& ConfigSchema::string_list(
    std::string key,
    std::vector<std::string> default_value,
    std::string description,
    Validator validator
) {
    Field field;
    field.key = std::move(key);
    field.type = Type::StringList;
    field.default_value = make_node(default_value);
    field.description = std::move(description);
    field.validator = std::move(validator);
    (void)add(std::move(field));
    return *this;
}

bool ConfigSchema::add(Field field) {
    if (field.key.empty() || contains(field.key)) {
        return false;
    }
    if (!field.default_value || field.default_value.IsNull()) {
        return false;
    }

    fields_.push_back(std::move(field));
    return true;
}

bool ConfigSchema::contains(std::string_view key) const noexcept {
    return find(key) != nullptr;
}

const ConfigSchema::Field* ConfigSchema::find(std::string_view key) const noexcept {
    for (const Field& field : fields_) {
        if (field.key == key) {
            return &field;
        }
    }
    return nullptr;
}

const std::vector<ConfigSchema::Field>& ConfigSchema::fields() const noexcept {
    return fields_;
}

std::vector<std::string> ConfigSchema::keys() const {
    std::vector<std::string> result;
    result.reserve(fields_.size());
    for (const Field& field : fields_) {
        result.push_back(field.key);
    }
    return result;
}

std::string ConfigSchema::documentation() const {
    std::string result = "# Config schema\n";
    for (const Field& field : fields_) {
        result += "- `";
        result += field.key;
        result += "` (";
        result += type_name(field.type);
        result += ")";
        if (!field.description.empty()) {
            result += " - ";
            result += field.description;
        }
        result += "; default: `";
        result += describe_default(field.default_value);
        result += "`\n";
    }
    return result;
}

void ConfigSchema::apply_to(Config& config) const {
    for (const Field& field : fields_) {
        switch (field.type) {
            case Type::String:
                install_field<std::string>(config, field);
                break;
            case Type::Integer:
                install_field<int>(config, field);
                break;
            case Type::Real:
                install_field<double>(config, field);
                break;
            case Type::Boolean:
                install_field<bool>(config, field);
                break;
            case Type::StringList:
                install_field<std::vector<std::string>>(config, field);
                break;
            default:
                break;
        }
    }
}

std::vector<Issue> ConfigSchema::validate(const Config& config) const {
    std::vector<Issue> issues;
    for (const Field& field : fields_) {
        const YAML::Node node = lookup_key(config.root(), field.key);
        const std::string message = check_field(field, node);
        if (!message.empty()) {
            issues.push_back({field.key, message});
        }
    }
    return issues;
}

std::string ConfigSchema::type_name(Type type) {
    switch (type) {
        case Type::String:
            return "string";
        case Type::Integer:
            return "integer";
        case Type::Real:
            return "real";
        case Type::Boolean:
            return "boolean";
        case Type::StringList:
            return "string-list";
        default:
            break;
    }
    return "unknown";
}

std::string ConfigSchema::describe_default(const YAML::Node& node) {
    YAML::Emitter emitter;
    emitter << node;
    return emitter.good() ? emitter.c_str() : std::string{};
}

std::string ConfigSchema::check_field(
    const Field& field, const YAML::Node& node
) {
    bool valid = false;

    switch (field.type) {
        case Type::String:
            valid = node.IsScalar();
            if (valid) {
                try {
                    (void)node.as<std::string>();
                } catch (const YAML::Exception&) {
                    valid = false;
                }
            }
            break;
        case Type::Integer:
            valid = node.IsScalar();
            if (valid) {
                try {
                    (void)node.as<int>();
                } catch (const YAML::Exception&) {
                    valid = false;
                }
            }
            break;
        case Type::Real:
            valid = node.IsScalar();
            if (valid) {
                try {
                    (void)node.as<double>();
                } catch (const YAML::Exception&) {
                    valid = false;
                }
            }
            break;
        case Type::Boolean:
            valid = node.IsScalar();
            if (valid) {
                try {
                    (void)node.as<bool>();
                } catch (const YAML::Exception&) {
                    valid = false;
                }
            }
            break;
        case Type::StringList:
            valid = node.IsSequence();
            if (valid) {
                for (const YAML::Node& element : node) {
                    if (!element.IsScalar()) {
                        valid = false;
                        break;
                    }
                    try {
                        (void)element.as<std::string>();
                    } catch (const YAML::Exception&) {
                        valid = false;
                        break;
                    }
                }
            }
            break;
        default:
            valid = false;
            break;
    }

    if (!valid) {
        return "expected " + type_name(field.type);
    }
    if (field.validator && !field.validator(node)) {
        return "value failed validation";
    }
    return std::string{};
}

} // namespace cfg

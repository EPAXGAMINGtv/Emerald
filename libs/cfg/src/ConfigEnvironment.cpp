#include "cfg/ConfigEnvironment.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <utility>

namespace cfg {

namespace {

std::string trim(std::string_view text) {
    std::size_t first = 0U;
    while (first < text.size() && std::isspace(static_cast<unsigned char>(text[first]))) {
        ++first;
    }

    std::size_t last = text.size();
    while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1U]))) {
        --last;
    }

    return std::string(text.substr(first, last - first));
}

std::vector<std::string> split(
    const std::string& text, const std::string& separator
) {
    std::vector<std::string> result;
    if (text.empty()) {
        return result;
    }
    if (separator.empty()) {
        result.push_back(text);
        return result;
    }

    std::size_t start = 0U;
    for (;;) {
        const std::size_t found = text.find(separator, start);
        if (found == std::string::npos) {
            result.push_back(trim(text.substr(start)));
            break;
        }
        result.push_back(trim(text.substr(start, found - start)));
        start = found + separator.size();
    }
    return result;
}

} // namespace

ConfigEnvironment& ConfigEnvironment::bind(
    std::string variable,
    std::string key,
    Type type,
    std::string separator
) {
    Binding* existing = find(variable);
    if (existing != nullptr) {
        existing->key = std::move(key);
        existing->type = type;
        existing->separator = std::move(separator);
        return *this;
    }

    Binding binding;
    binding.variable = std::move(variable);
    binding.key = std::move(key);
    binding.type = type;
    binding.separator = std::move(separator);
    bindings_.push_back(std::move(binding));
    return *this;
}

bool ConfigEnvironment::remove(std::string_view variable) {
    const auto found = std::find_if(
        bindings_.begin(),
        bindings_.end(),
        [variable](const Binding& binding) {
            return binding.variable == variable;
        }
    );
    if (found == bindings_.end()) {
        return false;
    }

    bindings_.erase(found);
    return true;
}

bool ConfigEnvironment::contains(std::string_view variable) const noexcept {
    return find(variable) != nullptr;
}

ConfigEnvironment::Binding* ConfigEnvironment::find(
    std::string_view variable
) noexcept {
    for (Binding& binding : bindings_) {
        if (binding.variable == variable) {
            return &binding;
        }
    }
    return nullptr;
}

const ConfigEnvironment::Binding* ConfigEnvironment::find(
    std::string_view variable
) const noexcept {
    for (const Binding& binding : bindings_) {
        if (binding.variable == variable) {
            return &binding;
        }
    }
    return nullptr;
}

const std::vector<ConfigEnvironment::Binding>& ConfigEnvironment::bindings(
) const noexcept {
    return bindings_;
}

std::vector<std::string> ConfigEnvironment::variables() const {
    std::vector<std::string> result;
    result.reserve(bindings_.size());
    for (const Binding& binding : bindings_) {
        result.push_back(binding.variable);
    }
    return result;
}

std::size_t ConfigEnvironment::size() const noexcept {
    return bindings_.size();
}

void ConfigEnvironment::clear() noexcept {
    bindings_.clear();
}

bool ConfigEnvironment::apply(Config& config) const {
    bool ok = true;

    for (const Binding& binding : bindings_) {
        const char* raw = std::getenv(binding.variable.c_str());
        if (raw == nullptr) {
            continue;
        }

        const std::string text = trim(raw);
        switch (binding.type) {
            case Type::String:
                config.set(binding.key, text);
                break;
            case Type::Integer: {
                std::size_t parsed = 0U;
                try {
                    const int value = std::stoi(text, &parsed);
                    if (parsed == text.size()) {
                        config.set(binding.key, value);
                    } else {
                        ok = false;
                    }
                } catch (const std::exception&) {
                    ok = false;
                }
                break;
            }
            case Type::Real: {
                std::size_t parsed = 0U;
                try {
                    const double value = std::stod(text, &parsed);
                    if (parsed == text.size()) {
                        config.set(binding.key, value);
                    } else {
                        ok = false;
                    }
                } catch (const std::exception&) {
                    ok = false;
                }
                break;
            }
            case Type::Boolean: {
                bool value = false;
                if (parse_bool(text, value)) {
                    config.set(binding.key, value);
                } else {
                    ok = false;
                }
                break;
            }
            case Type::StringList:
                config.set(binding.key, split(text, binding.separator));
                break;
        }
    }

    return ok;
}

bool ConfigEnvironment::parse_bool(std::string_view text, bool& value) {
    const std::string normalized = trim(text);

    std::string lower;
    lower.reserve(normalized.size());
    for (const char character : normalized) {
        lower.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(character)))
        );
    }

    if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") {
        value = true;
        return true;
    }
    if (lower == "0" || lower == "false" || lower == "no" || lower == "off") {
        value = false;
        return true;
    }
    return false;
}

} // namespace cfg

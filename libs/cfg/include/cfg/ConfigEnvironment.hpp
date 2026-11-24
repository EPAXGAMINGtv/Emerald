#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "cfg/Config.hpp"

namespace cfg {

// Maps process environment variables onto Config keys. Missing variables are
// ignored; malformed present values are reported through apply()'s return value
// and the affected key is left untouched.
class ConfigEnvironment {
public:
    enum class Type {
        String,
        Integer,
        Real,
        Boolean,
        StringList
    };

    struct Binding {
        std::string variable;
        std::string key;
        Type type = Type::String;
        std::string separator = ",";
    };

    ConfigEnvironment& bind(
        std::string variable,
        std::string key,
        Type type = Type::String,
        std::string separator = ","
    );
    bool remove(std::string_view variable);

    bool contains(std::string_view variable) const noexcept;
    Binding* find(std::string_view variable) noexcept;
    const Binding* find(std::string_view variable) const noexcept;
    const std::vector<Binding>& bindings() const noexcept;
    std::vector<std::string> variables() const;
    std::size_t size() const noexcept;
    void clear() noexcept;

    bool apply(Config& config) const;

    static bool parse_bool(std::string_view text, bool& value);

private:
    std::vector<Binding> bindings_;
};

} // namespace cfg

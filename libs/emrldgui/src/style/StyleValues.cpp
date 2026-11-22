//
// Created by epax on 22.11.26.
//

#include "emrldgui/style/StyleValues.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <string_view>

namespace emgui {

    namespace {

        std::string lower(std::string_view value) {
            std::string result(value);
            std::transform(
                result.begin(),
                result.end(),
                result.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
            );
            return result;
        }

        bool parseHexChannel(std::string_view text, unsigned char& output) {
            if (text.size() != 2) {
                return false;
            }

            try {
                const int value = std::stoi(
                    std::string(text),
                    nullptr,
                    16
                );
                output = static_cast<unsigned char>(value);
                return true;
            } catch (const std::exception&) {
                return false;
            }
        }

        bool parseColorValue(const std::string& value, SDL_Color& output) {
            const std::string normalized = lower(value);

            if (normalized == "white") {
                output = {255, 255, 255, 255};
                return true;
            }
            if (normalized == "black") {
                output = {0, 0, 0, 255};
                return true;
            }
            if (normalized == "red") {
                output = {255, 0, 0, 255};
                return true;
            }
            if (normalized == "green") {
                output = {0, 128, 0, 255};
                return true;
            }
            if (normalized == "blue") {
                output = {0, 0, 255, 255};
                return true;
            }
            if (normalized == "transparent") {
                output = {0, 0, 0, 0};
                return true;
            }

            if (normalized.starts_with("#")) {
                const std::string_view hex(normalized);
                unsigned char r = 0;
                unsigned char g = 0;
                unsigned char b = 0;
                unsigned char a = 255;

                if (hex.size() == 4) {
                    const std::string expanded = {
                        hex[1], hex[1], hex[2], hex[2], hex[3], hex[3]
                    };
                    if (!parseHexChannel(std::string_view(expanded).substr(0, 2), r) ||
                        !parseHexChannel(std::string_view(expanded).substr(2, 2), g) ||
                        !parseHexChannel(std::string_view(expanded).substr(4, 2), b)) {
                        return false;
                    }
                } else if (hex.size() == 7 || hex.size() == 9) {
                    if (!parseHexChannel(hex.substr(1, 2), r) ||
                        !parseHexChannel(hex.substr(3, 2), g) ||
                        !parseHexChannel(hex.substr(5, 2), b)) {
                        return false;
                    }
                    if (hex.size() == 9 && !parseHexChannel(hex.substr(7, 2), a)) {
                        return false;
                    }
                } else {
                    return false;
                }

                output = {r, g, b, a};
                return true;
            } else if (normalized.starts_with("rgba(") && normalized.ends_with(")")) {
                const std::string_view channels(normalized);
                const std::string_view inner = channels.substr(5, channels.size() - 6);
                int r = 0;
                int g = 0;
                int b = 0;
                int a = 255;
                char extra = '\0';

                if (std::sscanf(std::string(inner).c_str(), "%d,%d,%d,%d%c", &r, &g, &b, &a, &extra) < 4) {
                    return false;
                }

                output = {
                    static_cast<unsigned char>(std::clamp(r, 0, 255)),
                    static_cast<unsigned char>(std::clamp(g, 0, 255)),
                    static_cast<unsigned char>(std::clamp(b, 0, 255)),
                    static_cast<unsigned char>(std::clamp(a, 0, 255))
                };
                return true;
            }

            return false;
        }

        std::string unquote(const std::string& value) {
            if (value.size() >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                 (value.front() == '\'' && value.back() == '\''))) {
                return value.substr(1, value.size() - 2);
            }
            return value;
        }

        std::string trim(std::string_view value) {
            std::size_t first = 0;
            while (first < value.size() &&
                   std::isspace(static_cast<unsigned char>(value[first])) != 0) {
                ++first;
            }

            std::size_t last = value.size();
            while (last > first &&
                   std::isspace(static_cast<unsigned char>(value[last - 1])) != 0) {
                --last;
            }

            return std::string(value.substr(first, last - first));
        }

    } // namespace

    std::string StyleValues::getString(
        const std::string& property,
        const std::string& fallback
    ) const {
        const auto it = values_.find(property);
        return it == values_.end() ? fallback : unquote(it->second);
    }

    int StyleValues::getInt(const std::string& property, int fallback) const {
        const auto it = values_.find(property);

        if (it == values_.end()) {
            return fallback;
        }

        std::string value = unquote(it->second);

        const std::string normalized = lower(value);

        if (normalized.ends_with("px") || normalized.ends_with("ms")) {
            value.erase(value.size() - 2);
        }

        try {
            return std::stoi(value);
        } catch (const std::exception&) {
            return fallback;
        }
    }

    bool StyleValues::getBool(const std::string& property, bool fallback) const {
        const auto it = values_.find(property);

        if (it == values_.end()) {
            return fallback;
        }

        const std::string value = lower(unquote(it->second));

        if (value == "true" || value == "yes" || value == "on" || value == "1") {
            return true;
        }
        if (value == "false" || value == "no" || value == "off" || value == "0") {
            return false;
        }

        return fallback;
    }

    float StyleValues::getFloat(const std::string& property, float fallback) const {
        const auto it = values_.find(property);

        if (it == values_.end()) {
            return fallback;
        }

        std::string value = unquote(it->second);
        const std::string normalized = lower(value);

        if (normalized.ends_with("%")) {
            value.erase(value.size() - 1);
        } else if (normalized.ends_with("px") || normalized.ends_with("ms")) {
            value.erase(value.size() - 2);
        }

        try {
            return std::stof(value);
        } catch (const std::exception&) {
            return fallback;
        }
    }

    SDL_Color StyleValues::getColor(
        const std::string& property,
        const SDL_Color& fallback
    ) const {
        const auto it = values_.find(property);

        if (it == values_.end()) {
            return fallback;
        }

        SDL_Color result{};

        if (!parseColorValue(it->second, result)) {
            return fallback;
        }

        return result;
    }

    std::vector<SDL_Color> StyleValues::getColors(
        const std::string& property,
        const std::vector<SDL_Color>& fallback
    ) const {
        const auto it = values_.find(property);

        if (it == values_.end()) {
            return fallback;
        }

        const std::string value = unquote(it->second);
        std::vector<SDL_Color> colors;
        std::size_t position = 0;

        while (position <= value.size()) {
            const std::size_t comma = value.find(',', position);
            const std::string part = trim(
                std::string_view(value).substr(
                    position,
                    comma == std::string::npos ? std::string::npos : comma - position
                )
            );

            SDL_Color color{};

            if (part.empty() || !parseColorValue(part, color)) {
                return fallback;
            }

            colors.push_back(color);

            if (comma == std::string::npos) {
                break;
            }

            position = comma + 1;
        }

        return colors;
    }

    void StyleValues::merge(const StyleValues& other) {
        for (const auto& entry : other.values_) {
            values_[entry.first] = entry.second;
        }
    }

} // emgui

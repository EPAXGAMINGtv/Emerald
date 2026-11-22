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

            if (normalized == "white") output = {255, 255, 255, 255};
            else if (normalized == "black") output = {0, 0, 0, 255};
            else if (normalized == "red") output = {255, 0, 0, 255};
            else if (normalized == "green") output = {0, 128, 0, 255};
            else if (normalized == "blue") output = {0, 0, 255, 255};
            else if (normalized == "transparent") output = {0, 0, 0, 0};
            else if (normalized.starts_with("#")) {
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

        if (!value.empty() && lower(value).ends_with("px")) {
            value.erase(value.size() - 2);
        }

        try {
            return std::stoi(value);
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

} // emgui

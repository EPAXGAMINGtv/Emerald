// Created by epax on 22.11.26.

#pragma once

#include "emrldgui/render/Font.h"

#include <map>
#include <memory>
#include <string>
#include <tuple>

namespace emgui {

    struct FontSpec {
        std::string family = "LiberationSans";
        int size = 24;
        std::string style = "normal";

        bool operator<(const FontSpec& other) const {
            return std::tie(family, size, style) <
                   std::tie(other.family, other.size, other.style);
        }
    };

    // Loads TTF faces from res/font and caches them by family/size/style. Font owns the TTF_Font
    class FontManager {
    public:
        Font* getFont(const FontSpec& spec);
        void clear();

    private:
        std::map<FontSpec, std::unique_ptr<Font>> fonts_;
    };

} // emgui

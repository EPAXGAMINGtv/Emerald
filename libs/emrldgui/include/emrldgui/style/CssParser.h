// Created by epax on 22.11.26.

#pragma once

#include <filesystem>
#include <string_view>

#include "StyleSheet.h"

namespace emgui {

    // Small hand-written parser for the CSS subset used by emgui components
    class CssParser {
    public:
        static StyleSheet parseFile(const std::filesystem::path& path);
        static StyleSheet parseString(std::string_view source);
    };

} // emgui

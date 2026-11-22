//
// Created by epax on 22.11.26.
//

#pragma once

#include "StyleSheet.h"

#include <filesystem>
#include <map>
#include <string>

namespace emgui {

    class UIComponent;

    //loads res/style/<Component>.css and applies resolved values to components
    class StyleManager {
    public:
        void loadComponent(const std::string& componentName);
        const StyleSheet* getSheet(const std::string& componentName) const;
        void applyTo(UIComponent& component);
        void reload();

    private:
        void loadTheme();

        std::map<std::string, std::filesystem::path> paths_;
        std::map<std::string, StyleSheet> sheets_;
        StyleSheet themeSheet_;
        bool themeLoaded_ = false;
    };

} // emgui

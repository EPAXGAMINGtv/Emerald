// Created by epax on 22.11.26.

#include "emrldgui/style/StyleManager.h"

#include "emrldgui/resource/ResourceManager.h"
#include "emrldgui/style/CssParser.h"
#include "emrldgui/style/StyleResolver.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "spdlog/spdlog.h"

#include <vector>

namespace emgui {

    void StyleManager::loadComponent(const std::string& componentName) {
        loadTheme();

        if (componentName.empty()) {
            return;
        }

        const auto path = ResourceManager::stylePath(componentName);

        if (paths_.find(componentName) != paths_.end() &&
            paths_[componentName] == path &&
            sheets_.find(componentName) != sheets_.end()) {
            return;
        }

        paths_[componentName] = path;

        if (!std::filesystem::exists(path)) {
            spdlog::warn("No CSS stylesheet for component '{}' at {}", componentName, path.string());
            sheets_.erase(componentName);
            return;
        }

        sheets_[componentName] = CssParser::parseFile(path);
        spdlog::info(
            "Loaded CSS '{}' with {} rule(s)",
            path.string(),
            sheets_[componentName].getRules().size()
        );
    }

    void StyleManager::loadTheme() {
        if (themeLoaded_) {
            return;
        }

        themeLoaded_ = true;
        const auto path = ResourceManager::stylePath("Theme");

        if (!std::filesystem::exists(path)) {
            spdlog::warn("No theme stylesheet at {}", path.string());
            themeSheet_.clear();
            return;
        }

        themeSheet_ = CssParser::parseFile(path);
        spdlog::info(
            "Loaded CSS '{}' with {} rule(s)",
            path.string(),
            themeSheet_.getRules().size()
        );
    }

    const StyleSheet* StyleManager::getSheet(const std::string& componentName) const {
        const auto it = sheets_.find(componentName);
        return it == sheets_.end() ? nullptr : &it->second;
    }

    void StyleManager::applyTo(UIComponent& component) {
        loadTheme();

        // One combined cascade. Theme variables are visible in component sheets
        StyleSheet combined;

        for (const CssRule& rule : themeSheet_.getRules()) {
            combined.addRule(rule);
        }

        if (const StyleSheet* sheet = getSheet(component.getComponentName())) {
            for (const CssRule& rule : sheet->getRules()) {
                combined.addRule(rule);
            }
        }

        component.applyStyle(StyleResolver::resolve(combined, component));
    }

    void StyleManager::reload() {
        themeLoaded_ = false;
        loadTheme();

        const auto componentNames = [this]() {
            std::vector<std::string> names;
            names.reserve(paths_.size());
            for (const auto& entry : paths_) {
                names.push_back(entry.first);
            }
            return names;
        }();

        sheets_.clear();

        for (const auto& name : componentNames) {
            loadComponent(name);
        }
    }

} // emgui

// Created by epax on 22.11.26.

#include "emrldgui/resource/ResourceManager.h"

#include <SDL.h>

#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <vector>

#include "spdlog/spdlog.h"

namespace emgui {

    namespace {

        std::string lower(std::string value) {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
            );
            return value;
        }

        std::string fontFileName(const std::string& family, const std::string& style) {
            std::string base = family;

            if (!base.empty() && (base.back() == '"' || base.back() == '\'')) {
                base.pop_back();
            }
            if (!base.empty() && (base.front() == '"' || base.front() == '\'')) {
                base.erase(base.begin());
            }

            if (base.size() >= 4 && base.substr(base.size() - 4) == ".ttf") {
                return base;
            }

            const std::string normalizedStyle = lower(style);
            std::string suffix = "Regular";

            if (normalizedStyle == "bold" || normalizedStyle == "700") {
                suffix = "Bold";
            } else if (normalizedStyle == "italic" || normalizedStyle == "oblique") {
                suffix = "Italic";
            } else if (normalizedStyle == "bolditalic" ||
                       normalizedStyle == "bold italic" ||
                       normalizedStyle == "bold-italic") {
                suffix = "BoldItalic";
            }

            return base + "-" + suffix + ".ttf";
        }

    } // namespace

    std::optional<std::filesystem::path>& ResourceManager::rootCache() {
        static std::optional<std::filesystem::path> cache;
        return cache;
    }

    void ResourceManager::setRoot(std::filesystem::path root) {
        rootCache() = std::move(root);
        spdlog::info("Resource root overridden: {}", rootCache()->string());
    }

    void ResourceManager::clearCache() {
        rootCache().reset();
    }

    bool ResourceManager::looksLikeResourceRoot(const std::filesystem::path& path) {
        std::error_code error;
        return std::filesystem::is_directory(path, error) &&
               std::filesystem::is_directory(path / "style", error) &&
               std::filesystem::is_directory(path / "font", error) &&
               std::filesystem::is_directory(path / "textures", error);
    }

    std::filesystem::path ResourceManager::getRoot() {
        if (rootCache().has_value()) {
            return *rootCache();
        }

        std::vector<std::filesystem::path> candidates;

        if (const char* envRoot = std::getenv("EMERALD_RES")) {
            if (*envRoot != '\0') {
                candidates.emplace_back(envRoot);
            }
        }

        if (char* basePath = SDL_GetBasePath()) {
            candidates.emplace_back(std::filesystem::path(basePath) / "res");
            SDL_free(basePath);
        }

#ifdef EMERLDGUI_RES_DIR
        candidates.emplace_back(EMERLDGUI_RES_DIR);
#endif
        candidates.emplace_back("res");

        for (const auto& candidate : candidates) {
            if (looksLikeResourceRoot(candidate)) {
                const auto absolute = std::filesystem::absolute(candidate).lexically_normal();
                rootCache() = absolute;
                spdlog::info("Using resource root {}", absolute.string());
                return absolute;
            }
        }

        rootCache() = candidates.empty()
            ? std::filesystem::path("res")
            : std::filesystem::absolute(candidates.back()).lexically_normal();

        spdlog::warn(
            "No complete resource directory found, falling back to {}",
            rootCache()->string()
        );

        return *rootCache();
    }

    std::filesystem::path ResourceManager::stylePath(const std::string& componentName) {
        return getRoot() / "style" / (componentName + ".css");
    }

    std::filesystem::path ResourceManager::fontPath(
        const std::string& family,
        const std::string& style
    ) {
        const auto fontDir = getRoot() / "font";
        const auto requested = fontDir / fontFileName(family, style);

        if (std::filesystem::exists(requested)) {
            return requested;
        }

        const auto regular = fontDir / fontFileName(family, "normal");

        if (std::filesystem::exists(regular)) {
            if (requested != regular) {
                spdlog::warn(
                    "Font '{}' not found, falling back to {}",
                    requested.string(),
                    regular.string()
                );
            }
            return regular;
        }

        spdlog::warn("Font file for family '{}' not found in {}", family, fontDir.string());
        return requested;
    }

    std::filesystem::path ResourceManager::texturePath(const std::string& name) {
        return getRoot() / "textures" / name;
    }

} // emgui

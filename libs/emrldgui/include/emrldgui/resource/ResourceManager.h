//
// Created by epax on 22.11.26.
//

#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace emgui {

    //finds res/ independent of the current working directory -> resolution order:
    //setRoot(), EMERALD_RES env, res/ next to the binary, compiled source path, ./res
    class ResourceManager {
    public:
        static void setRoot(std::filesystem::path root);
        static std::filesystem::path getRoot();
        static void clearCache();

        static std::filesystem::path stylePath(const std::string& componentName);
        static std::filesystem::path fontPath(
            const std::string& family,
            const std::string& style = "normal"
        );
        static std::filesystem::path texturePath(const std::string& name);

    private:
        static std::optional<std::filesystem::path>& rootCache();
        static bool looksLikeResourceRoot(const std::filesystem::path& path);
    };

} // emgui

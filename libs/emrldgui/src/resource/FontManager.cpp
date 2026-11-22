//
// Created by epax on 22.11.26.
//

#include "emrldgui/resource/FontManager.h"

#include "emrldgui/resource/ResourceManager.h"
#include "spdlog/spdlog.h"

#include <filesystem>
#include <string>

namespace emgui {

    Font* FontManager::getFont(const FontSpec& spec) {
        auto it = fonts_.find(spec);

        if (it != fonts_.end()) {
            return it->second.get();
        }

        std::filesystem::path path;

        if (spec.family.size() >= 4 &&
            spec.family.substr(spec.family.size() - 4) == ".ttf" &&
            std::filesystem::exists(spec.family)) {
            path = spec.family;
        } else {
            path = ResourceManager::fontPath(spec.family, spec.style);
        }

        auto font = std::make_unique<Font>(spec.size, path.string());

        if (!font->isValid()) {
            spdlog::warn(
                "Could not load font family='{}' style='{}' size={} from {}",
                spec.family,
                spec.style,
                spec.size,
                path.string()
            );
        }

        Font* result = font.get();
        fonts_.emplace(spec, std::move(font));
        return result;
    }

    void FontManager::clear() {
        fonts_.clear();
    }

} // emgui

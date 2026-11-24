#include "flatpak/Flatpak.h"

#include "FlatpakParse.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace fpk {

namespace {

bool PathExists(const std::filesystem::path& candidate) {
    std::error_code error;
    return std::filesystem::exists(candidate, error) && !error;
}

std::filesystem::path FindIconInLocation(
    const std::filesystem::path& location, const std::string& id
) {
    const std::filesystem::path hicolor =
        location / "share" / "icons" / "hicolor";
    const std::vector<std::filesystem::path> candidates = {
        hicolor / "scalable" / "apps" / (id + ".svg"),
        hicolor / "48x48" / "apps" / (id + ".png"),
        hicolor / "64x64" / "apps" / (id + ".png"),
        hicolor / "128x128" / "apps" / (id + ".png")
    };

    for (const std::filesystem::path& candidate : candidates) {
        if (PathExists(candidate)) {
            return candidate;
        }
    }

    std::error_code error;
    std::filesystem::directory_iterator iterator(hicolor, error);
    const std::filesystem::directory_iterator end;
    for (; !error && iterator != end; iterator.increment(error)) {
        if (!iterator->is_directory(error) || error) {
            continue;
        }

        const std::filesystem::path png =
            iterator->path() / "apps" / (id + ".png");
        if (PathExists(png)) {
            return png;
        }

        const std::filesystem::path svg =
            iterator->path() / "apps" / (id + ".svg");
        if (PathExists(svg)) {
            return svg;
        }
    }

    return {};
}

} // namespace

void Flatpak::SetIconCacheDirectory(std::filesystem::path directory) {
    iconCacheDirectory_ = std::move(directory);
}

const std::filesystem::path& Flatpak::IconCacheDirectory() const noexcept {
    return iconCacheDirectory_;
}

std::filesystem::path Flatpak::GetApplicationIconPath(
    const std::string& id
) const {
    if (id.empty() || iconCacheDirectory_.empty()) {
        return {};
    }
    return iconCacheDirectory_ / (id + ".png");
}

std::filesystem::path Flatpak::GetApplicationIcon(
    const std::string& id
) const {
    if (id.empty()) {
        return {};
    }

    const auto overrideFound = iconOverrides_.find(id);
    if (overrideFound != iconOverrides_.end()) {
        return overrideFound->second;
    }

    if (!iconCacheDirectory_.empty()) {
        const std::filesystem::path png =
            iconCacheDirectory_ / (id + ".png");
        if (PathExists(png)) {
            return png;
        }

        const std::filesystem::path svg =
            iconCacheDirectory_ / (id + ".svg");
        if (PathExists(svg)) {
            return svg;
        }
    }

    const Result result = command_.Run({"info", "--show-location", id});
    if (!result.Ok) {
        return {};
    }

    const std::string location = detail::Trim(result.Output);
    if (location.empty()) {
        return {};
    }
    return FindIconInLocation(location, id);
}

bool Flatpak::HasApplicationIcon(const std::string& id) const {
    if (id.empty()) {
        return false;
    }
    if (iconOverrides_.find(id) != iconOverrides_.end()) {
        return true;
    }
    if (iconCacheDirectory_.empty()) {
        return false;
    }

    return PathExists(iconCacheDirectory_ / (id + ".png")) ||
           PathExists(iconCacheDirectory_ / (id + ".svg"));
}

bool Flatpak::SetApplicationIcon(
    const std::string& id, std::filesystem::path iconPath
) {
    if (id.empty()) {
        return false;
    }
    iconOverrides_[id] = std::move(iconPath);
    return true;
}

bool Flatpak::ClearApplicationIcon(const std::string& id) {
    return iconOverrides_.erase(id) > 0U;
}

void Flatpak::ClearApplicationIcons() {
    iconOverrides_.clear();
}

} // namespace fpk

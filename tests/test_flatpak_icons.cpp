#include "TestFramework.hpp"

#include "flatpak/Flatpak.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

fpk::Result Failed() {
    fpk::Result result;
    result.ExitCode = 1;
    result.Error = "offline";
    return result;
}

void RemoveTree(const std::filesystem::path& target) {
    std::error_code error;
    std::filesystem::remove_all(target, error);
}

void WriteFile(const std::filesystem::path& target) {
    std::ofstream stream(target);
    stream << "icon";
}

} // namespace

TEST(flatpak_icon_cache_and_overrides) {
    const std::filesystem::path cache =
        std::filesystem::temp_directory_path() /
        "emerald_flatpak_icon_cache_test";
    RemoveTree(cache);

    std::error_code error;
    std::filesystem::create_directories(cache, error);
    CHECK(!error);

    fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [](const std::vector<std::string>&) {
            return Failed();
        }
    );

    flatpak.SetIconCacheDirectory(cache);
    CHECK(flatpak.IconCacheDirectory() == cache);

    const std::filesystem::path expected =
        flatpak.GetApplicationIconPath("org.test.App");
    CHECK(expected == cache / "org.test.App.png");
    CHECK(flatpak.GetApplicationIconPath(std::string{}).empty());
    CHECK(!flatpak.HasApplicationIcon("org.test.App"));

    WriteFile(expected);
    CHECK(flatpak.HasApplicationIcon("org.test.App"));
    CHECK(flatpak.GetApplicationIcon("org.test.App") == expected);

    const std::filesystem::path overridePath = cache / "override.svg";
    WriteFile(overridePath);
    CHECK(flatpak.SetApplicationIcon("org.test.Other", overridePath));
    CHECK(flatpak.HasApplicationIcon("org.test.Other"));
    CHECK(flatpak.GetApplicationIcon("org.test.Other") == overridePath);
    CHECK(!flatpak.SetApplicationIcon(std::string{}, overridePath));

    CHECK(flatpak.ClearApplicationIcon("org.test.Other"));
    CHECK(!flatpak.ClearApplicationIcon("org.test.Other"));
    CHECK(!flatpak.HasApplicationIcon("org.test.Other"));

    CHECK(flatpak.SetApplicationIcon("org.test.Third", overridePath));
    flatpak.ClearApplicationIcons();
    CHECK(!flatpak.HasApplicationIcon("org.test.Third"));

    RemoveTree(cache);
}

TEST(flatpak_icon_lookup_from_installed_location) {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        "emerald_flatpak_icon_location_test";
    RemoveTree(root);

    const std::filesystem::path icon =
        root / "share" / "icons" / "hicolor" / "128x128" / "apps" /
        "org.test.App.png";
    std::error_code error;
    std::filesystem::create_directories(icon.parent_path(), error);
    CHECK(!error);
    WriteFile(icon);

    std::vector<std::vector<std::string>> seen;
    const fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [&seen, &root](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            fpk::Result result;
            result.Ok = true;
            result.ExitCode = 0;
            result.Output = root.string() + "\n";
            return result;
        }
    );

    CHECK(flatpak.GetApplicationIcon("org.test.App") == icon);
    CHECK(seen.size() == 1U);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "info", "--show-location", "org.test.App"
    }));

    CHECK(flatpak.GetApplicationIcon(std::string{}).empty());
    CHECK(seen.size() == 1U);

    RemoveTree(root);
}

TEST(flatpak_icon_failure_paths) {
    const fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [](const std::vector<std::string>&) {
            return Failed();
        }
    );

    CHECK(flatpak.GetApplicationIcon("missing").empty());
    CHECK(!flatpak.HasApplicationIcon("missing"));
}

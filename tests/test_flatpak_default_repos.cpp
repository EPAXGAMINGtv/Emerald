#include "TestFramework.hpp"

#include "flatpak/Flatpak.h"

#include <string>
#include <vector>

namespace {

fpk::Result VersionResult() {
    fpk::Result result;
    result.Ok = true;
    result.ExitCode = 0;
    result.Output = "Flatpak 1.18.4\n";
    return result;
}

} // namespace

TEST(flatpak_ensure_available_adds_flathub_when_missing) {
    bool configured = false;
    std::vector<std::vector<std::string>> seen;
    fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [&configured, &seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            if (argv[1] == "--version") {
                return VersionResult();
            }

            fpk::Result result;
            result.Ok = true;
            result.ExitCode = 0;
            if (argv[1] == "remote-list") {
                if (configured) {
                    result.Output =
                        "flathub\thttps://dl.flathub.org/repo/\t1\tsystem\n";
                }
                return result;
            }

            if (argv[1] == "remote-add") {
                configured = true;
            }
            return result;
        }
    );

    CHECK(flatpak.EnsureDefaultReposOnStartup());
    const fpk::Result first = flatpak.EnsureAvailable();
    CHECK(first.Ok);
    CHECK(first.Output == "default repositories ready");
    CHECK(seen.size() == 3U);
    CHECK(seen[1][1] == "remote-list");
    CHECK(seen[2][1] == "remote-add");
    CHECK(seen[2][3] == "flathub");
    CHECK(seen[2][4] == "https://dl.flathub.org/repo/flathub.flatpakrepo");

    seen.clear();
    const fpk::Result second = flatpak.EnsureAvailable();
    CHECK(second.Ok);
    CHECK(seen.size() == 2U);
    CHECK(seen[1][1] == "remote-list");
}

TEST(flatpak_ensure_available_can_skip_default_repos) {
    std::vector<std::vector<std::string>> seen;
    fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            return VersionResult();
        }
    );

    flatpak.SetEnsureDefaultReposOnStartup(false);
    CHECK(!flatpak.EnsureDefaultReposOnStartup());

    const fpk::Result result = flatpak.EnsureAvailable();
    CHECK(result.Ok);
    CHECK(result.Output == "1.18.4");
    CHECK(seen.size() == 1U);
    CHECK(seen[0][1] == "--version");
}

TEST(flatpak_ensure_available_default_repo_failure) {
    fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [](const std::vector<std::string>& argv) {
            if (argv[1] == "--version") {
                return VersionResult();
            }
            if (argv[1] == "remote-list") {
                fpk::Result empty;
                empty.Ok = true;
                empty.ExitCode = 0;
                return empty;
            }

            fpk::Result failed;
            failed.ExitCode = 1;
            failed.Error = "permission denied";
            return failed;
        }
    );

    const fpk::Result result = flatpak.EnsureAvailable();
    CHECK(!result.Ok);
    CHECK(result.Error.find("failed to add flathub") != std::string::npos);
}

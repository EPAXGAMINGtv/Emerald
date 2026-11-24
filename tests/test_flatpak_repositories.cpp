#include "TestFramework.hpp"

#include "flatpak/Flatpak.h"

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

fpk::Result Ok(const std::string& output = {}) {
    fpk::Result result;
    result.Ok = true;
    result.ExitCode = 0;
    result.Output = output;
    return result;
}

fpk::Flatpak MakeMock(
    std::function<fpk::Result(const std::vector<std::string>&)> executor
) {
    return fpk::Flatpak("flatpak-mock", std::move(executor));
}

} // namespace

TEST(flatpak_repository_commands) {
    std::vector<std::vector<std::string>> seen;
    fpk::Flatpak flatpak = MakeMock(
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            return Ok();
        }
    );

    CHECK(flatpak.AddFlatpakRepo("test", "https://example/repo").Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "remote-add", "--if-not-exists",
        "test", "https://example/repo"
    }));

    CHECK(flatpak.AddFlatpakRepo("test2", "https://example/2", false).Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "remote-add", "test2", "https://example/2"
    }));

    CHECK(flatpak.RemoveFlatpakRepo("test").Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "remote-delete", "test"
    }));

    CHECK(flatpak.EnableFlatpakRepo("test").Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "remote-modify", "--enable", "test"
    }));

    CHECK(flatpak.DisableFlatpakRepo("test").Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "remote-modify", "--disable", "test"
    }));

    CHECK(flatpak.SetFlatpakRepoPriority("test", 7).Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "remote-modify", "--prio=7", "test"
    }));

    CHECK(flatpak.UpdateFlatpakRepo("test").Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "remote-update", "test"
    }));

    CHECK(flatpak.UpdateAllFlatpakRepos().Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "remote-update"
    }));
}

TEST(flatpak_repository_listing_and_lookup) {
    std::vector<std::vector<std::string>> seen;
    fpk::Flatpak flatpak = MakeMock(
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            return Ok(
                "flathub\thttps://dl.flathub.org/repo/\t1\tsystem\n"
                "broken\thttps://example/repo\t0\tuser,disabled\n"
            );
        }
    );

    const std::vector<fpk::Repository> repos = flatpak.ListFlatpakRepos();
    CHECK(repos.size() == 2U);
    CHECK(repos[0].Name == "flathub");
    CHECK(repos[0].Url == "https://dl.flathub.org/repo/");
    CHECK(repos[0].Priority == 1);
    CHECK(repos[0].Enabled);
    CHECK(repos[1].Name == "broken");
    CHECK(repos[1].Priority == 0);
    CHECK(!repos[1].Enabled);

    CHECK(seen.back()[1] == "remote-list");
    CHECK(seen.back()[3] == "--show-disabled");
    (void)flatpak.ListFlatpakRepos(false);
    CHECK(seen.back().size() == 3U);

    CHECK(flatpak.FlatpakRepoExists("flathub"));
    CHECK(flatpak.FlatpakRepoExists("broken"));
    CHECK(!flatpak.FlatpakRepoExists("missing"));

    const std::optional<fpk::Repository> found = flatpak.GetFlatpakRepo("broken");
    CHECK(found.has_value());
    CHECK(found.has_value() && found->Name == "broken");
    CHECK(!flatpak.GetFlatpakRepo("missing").has_value());

    CHECK(flatpak.GetFlatpakRepoUrl("flathub") ==
          "https://dl.flathub.org/repo/");
    CHECK(flatpak.GetFlatpakRepoUrl("missing").empty());
}

TEST(flatpak_default_repos_and_refs) {
    std::vector<std::vector<std::string>> seen;
    fpk::Flatpak flatpak = MakeMock(
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            if (argv.size() > 1U && argv[1] == "remote-ls") {
                return Ok(
                    "app/org.gnome.Rows/x86_64/stable\n"
                    "app/org.test.App/x86_64/stable\n"
                );
            }
            return Ok();
        }
    );

    CHECK(flatpak.IsDefaultRepoConfigured("flathub"));
    CHECK(!flatpak.IsDefaultRepoConfigured("other"));

    const fpk::Result added = flatpak.AddDefaultRepos();
    CHECK(added.Ok);
    CHECK(seen.back().size() == 5U);
    CHECK(seen.back()[1] == "remote-add");
    CHECK(seen.back()[3] == "flathub");
    CHECK(seen.back()[4] == "https://dl.flathub.org/repo/flathub.flatpakrepo");

    const std::vector<std::string> refs = flatpak.ListFlatpakRefs("flathub");
    CHECK(refs.size() == 2U);
    CHECK(refs[0].starts_with("app/"));
    CHECK(refs[1].find("org.test.App") != std::string::npos);
    CHECK(seen.back().back() == "flathub");

    (void)flatpak.ListFlatpakRefs({});
    CHECK(seen.back().size() == 4U);

    fpk::Flatpak failing = MakeMock(
        [](const std::vector<std::string>&) {
            fpk::Result result;
            result.ExitCode = 1;
            result.Error = "denied";
            return result;
        }
    );
    CHECK(!failing.AddDefaultRepos().Ok);
}

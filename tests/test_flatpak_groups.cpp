#include "TestFramework.hpp"

#include "flatpak/Flatpak.h"

#include <string>
#include <vector>

namespace {

fpk::Result Ok(const std::string& output = {}) {
    fpk::Result result;
    result.Ok = true;
    result.ExitCode = 0;
    result.Output = output;
    return result;
}

fpk::Flatpak MakeGroupMock() {
    return fpk::Flatpak(
        "flatpak-mock",
        [](const std::vector<std::string>& argv) {
            if (argv.size() > 1U && argv[1] == "remote-ls") {
                return Ok(
                    "org.test.App\tTest App\tDescription\t1.0\tstable\t"
                    "flathub\tsystem\n"
                );
            }
            return Ok();
        }
    );
}

} // namespace

TEST(flatpak_group_crud) {
    fpk::Flatpak flatpak = MakeGroupMock();

    CHECK(flatpak.CreateFlatpakGroup("productivity", "Productivity").Ok);
    CHECK(flatpak.FlatpakGroupExists("productivity"));
    CHECK(!flatpak.CreateFlatpakGroup("productivity").Ok);
    CHECK(!flatpak.CreateFlatpakGroup(std::string{}).Ok);

    const auto found = flatpak.GetFlatpakGroup("productivity");
    CHECK(found.has_value());
    CHECK(found.has_value() && found->Name == "Productivity");
    CHECK(!flatpak.GetFlatpakGroup("missing").has_value());

    CHECK(flatpak.RenameFlatpakGroup("productivity", "Office"));
    CHECK(flatpak.GetFlatpakGroup("productivity").has_value());
    CHECK(flatpak.GetFlatpakGroup("productivity")->Name == "Office");
    CHECK(!flatpak.RenameFlatpakGroup("productivity", std::string{}));
    CHECK(!flatpak.RenameFlatpakGroup("missing", "Other"));

    CHECK(flatpak.ListFlatpakGroupIds() ==
          std::vector<std::string>({"productivity"}));
    CHECK(flatpak.ListFlatpakGroups().size() == 1U);

    CHECK(flatpak.CreateFlatpakGroup("games").Ok);
    CHECK(flatpak.ListFlatpakGroups().size() == 2U);
    CHECK(!flatpak.RemoveFlatpakGroup("missing"));
    CHECK(flatpak.RemoveFlatpakGroup("games"));
    CHECK(flatpak.ListFlatpakGroups().size() == 1U);
}

TEST(flatpak_group_membership_and_metadata) {
    fpk::Flatpak flatpak = MakeGroupMock();

    CHECK(!flatpak.AddApplicationToGroup("missing", "org.test.App"));
    CHECK(flatpak.CreateFlatpakGroup("office", "Office").Ok);
    CHECK(flatpak.AddApplicationToGroup("office", "org.test.App"));
    CHECK(!flatpak.AddApplicationToGroup("office", "org.test.App"));
    CHECK(!flatpak.AddApplicationToGroup(std::string{}, "org.test.App"));
    CHECK(!flatpak.AddApplicationToGroup("office", std::string{}));

    CHECK(flatpak.GetApplicationsInGroup("office") ==
          std::vector<std::string>({"org.test.App"}));
    CHECK(flatpak.GetGroupsForApplication("org.test.App") ==
          std::vector<std::string>({"office"}));
    CHECK(flatpak.GetApplicationsInGroup("missing").empty());
    CHECK(flatpak.GetGroupsForApplication("missing").empty());

    const std::vector<std::string> categories = {"Office", "Utilities"};
    CHECK(flatpak.SetApplicationCategories("org.test.App", categories));
    CHECK(flatpak.GetApplicationCategories("org.test.App") == categories);
    CHECK(!flatpak.SetApplicationCategories(std::string{}, categories));

    const auto application = flatpak.GetApplicationById("org.test.App");
    CHECK(application.has_value());
    CHECK(application.has_value() && application->Categories == categories);
    CHECK(application.has_value() &&
          application->Groups == std::vector<std::string>({"office"}));

    CHECK(flatpak.GetApplicationsByCategory("Office").size() == 1U);
    CHECK(flatpak.GetApplicationsByCategory("Games").empty());
    CHECK(flatpak.GetApplicationsByCategory(std::string{}).empty());

    CHECK(flatpak.RemoveApplicationFromGroup("office", "org.test.App"));
    CHECK(!flatpak.RemoveApplicationFromGroup("office", "org.test.App"));
    CHECK(flatpak.GetGroupsForApplication("org.test.App").empty());

    CHECK(flatpak.ClearApplicationCategories("org.test.App"));
    CHECK(!flatpak.ClearApplicationCategories("org.test.App"));
    CHECK(flatpak.GetApplicationCategories("org.test.App").empty());

    flatpak.ClearFlatpakGroups();
    CHECK(flatpak.ListFlatpakGroups().empty());
}

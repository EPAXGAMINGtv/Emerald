#include "TestFramework.hpp"

#include "flatpak/Flatpak.h"

#include <functional>
#include <optional>
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

const char* const kAvailable =
    "org.gnome.Rows\tRows\tSpreadsheet app\t46.0\tstable\tflathub\tsystem\n"
    "org.test.App\tTest App\tTest description\t1.2\tbeta\ttestrepo\tuser\n";
const char* const kInstalled =
    "org.gnome.Rows\tRows\tSpreadsheet app\t46.0\tstable\tflathub\tsystem\n";

fpk::Flatpak MakeCatalogMock(std::vector<std::vector<std::string>>& seen) {
    return fpk::Flatpak(
        "flatpak-mock",
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            if (argv[1] == "remote-ls") {
                return Ok(kAvailable);
            }
            if (argv[1] == "list") {
                return Ok(kInstalled);
            }
            return Ok();
        }
    );
}

} // namespace

TEST(flatpak_catalog_parses_available_and_installed) {
    std::vector<std::vector<std::string>> seen;
    const fpk::Flatpak flatpak = MakeCatalogMock(seen);

    const std::vector<fpk::Application> available =
        flatpak.GetAllApplicationsAvailable();
    CHECK(available.size() == 2U);
    CHECK(available[0].Id == "org.gnome.Rows");
    CHECK(available[0].Name == "Rows");
    CHECK(available[0].Summary == "Spreadsheet app");
    CHECK(available[0].Description == "Spreadsheet app");
    CHECK(available[0].Version == "46.0");
    CHECK(available[0].Branch == "stable");
    CHECK(available[0].Remote == "flathub");
    CHECK(available[1].Name == "Test App");
    CHECK(available[1].Remote == "testrepo");
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "remote-ls", "--app",
        "--columns=application,name,description,version,branch,origin,options"
    }));

    const std::vector<fpk::Application> installed =
        flatpak.GetAllInstalledApplications();
    CHECK(installed.size() == 1U);
    CHECK(installed[0].Id == "org.gnome.Rows");
    CHECK(seen.back()[1] == "list");
}

TEST(flatpak_catalog_lookup_and_filters) {
    std::vector<std::vector<std::string>> seen;
    const fpk::Flatpak flatpak = MakeCatalogMock(seen);

    const std::optional<fpk::Application> found =
        flatpak.GetApplicationById("org.test.App");
    CHECK(found.has_value());
    CHECK(found.has_value() && found->Name == "Test App");
    CHECK(!flatpak.GetApplicationById(std::string{}).has_value());

    CHECK(flatpak.IsApplicationAvailable("org.test.App"));
    CHECK(!flatpak.IsApplicationAvailable("missing"));
    CHECK(flatpak.IsApplicationInstalled("org.gnome.Rows"));
    CHECK(!flatpak.IsApplicationInstalled("org.test.App"));

    CHECK(flatpak.GetApplicationName("org.gnome.Rows") == "Rows");
    CHECK(flatpak.GetApplicationSummary("org.test.App") == "Test description");
    CHECK(flatpak.GetApplicationDescription("org.test.App") ==
          "Test description");
    CHECK(flatpak.GetApplicationVersion("org.test.App") == "1.2");
    CHECK(flatpak.GetApplicationCategories("org.test.App").empty());
    CHECK(flatpak.GetApplicationName("missing").empty());

    const std::vector<std::string> ids = flatpak.ListAvailableApplicationIds();
    CHECK(ids == std::vector<std::string>({
        "org.gnome.Rows", "org.test.App"
    }));
    CHECK(flatpak.ListInstalledApplicationIds() ==
          std::vector<std::string>({"org.gnome.Rows"}));

    const std::vector<fpk::Application> byRemote =
        flatpak.GetApplicationsByRemote("testrepo");
    CHECK(byRemote.size() == 1U);
    CHECK(byRemote[0].Id == "org.test.App");
    CHECK(flatpak.GetApplicationsByRemote(std::string{}).empty());
    CHECK(flatpak.GetApplicationsByRemote("missing").empty());
}

TEST(flatpak_catalog_search) {
    std::vector<std::vector<std::string>> seen;
    fpk::Flatpak flatpak = MakeCatalogMock(
        seen
    );
    flatpak.SetExecutor(
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            return Ok(
                "Rows\tSpreadsheet app\torg.gnome.Rows\t46.0\tstable\t"
                "flathub,other\n"
            );
        }
    );

    const std::vector<fpk::Application> results =
        flatpak.SearchFlatpakApplications("rows");
    CHECK(results.size() == 1U);
    CHECK(results[0].Name == "Rows");
    CHECK(results[0].Id == "org.gnome.Rows");
    CHECK(results[0].Version == "46.0");
    CHECK(results[0].Remote == "flathub");
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "search",
        "--columns=name,description,application,version,branch,remotes",
        "rows"
    }));

    CHECK(flatpak.SearchFlatpakApplications(std::string{}).empty());
    CHECK(seen.size() == 1U);
}

TEST(flatpak_catalog_failure_paths) {
    const fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [](const std::vector<std::string>&) {
            fpk::Result failed;
            failed.ExitCode = 1;
            failed.Error = "offline";
            return failed;
        }
    );

    CHECK(flatpak.GetAllApplicationsAvailable().empty());
    CHECK(flatpak.GetAllInstalledApplications().empty());
    CHECK(!flatpak.GetApplicationById("org.test.App").has_value());
    CHECK(!flatpak.IsApplicationAvailable("org.test.App"));
    CHECK(!flatpak.IsApplicationInstalled("org.test.App"));
    CHECK(flatpak.GetApplicationName("org.test.App").empty());
    CHECK(flatpak.GetApplicationSummary("org.test.App").empty());
    CHECK(flatpak.GetApplicationDescription("org.test.App").empty());
    CHECK(flatpak.GetApplicationVersion("org.test.App").empty());
    CHECK(flatpak.GetApplicationCategories("org.test.App").empty());
    CHECK(flatpak.SearchFlatpakApplications("test").empty());
    CHECK(flatpak.GetApplicationsByRemote("flathub").empty());
    CHECK(flatpak.ListAvailableApplicationIds().empty());
    CHECK(flatpak.ListInstalledApplicationIds().empty());
}

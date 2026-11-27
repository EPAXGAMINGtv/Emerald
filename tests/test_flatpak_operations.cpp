#include "TestFramework.hpp"

#include "flatpak/Flatpak.h"

#include <chrono>
#include <string>
#include <vector>

namespace {

fpk::Result Ok(
    const std::string& output = {},
    std::chrono::milliseconds duration = std::chrono::milliseconds{0}
) {
    fpk::Result result;
    result.Ok = true;
    result.ExitCode = 0;
    result.Output = output;
    result.Duration = duration;
    return result;
}

fpk::Result Failed() {
    fpk::Result result;
    result.ExitCode = 1;
    result.Error = "offline";
    return result;
}

} // namespace

TEST(flatpak_install_tracks_progress_and_time) {
    std::vector<std::vector<std::string>> seen;
    fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            return Ok("25% downloaded, 00:12 remaining\n",
                      std::chrono::milliseconds{2500});
        }
    );

    const fpk::Result installed =
        flatpak.InstallFlatpakPackage("org.test.App", "testrepo", "beta");
    CHECK(installed.Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "install", "-y", "--noninteractive",
        "testrepo", "org.test.App", "beta"
    }));

    CHECK(flatpak.GetPackageOperationStatus("org.test.App") ==
          fpk::OperationStatus::Succeeded);
    CHECK(!flatpak.IsPackageOperationRunning("org.test.App"));

    const fpk::Progress progress =
        flatpak.GetPackageInstallProgress("org.test.App");
    CHECK(progress.PackageId == "org.test.App");
    CHECK(progress.Phase == "install");
    CHECK_NEAR(progress.Fraction, 0.25);
    CHECK(progress.Elapsed == std::chrono::milliseconds{2500});

    const auto remaining =
        flatpak.GetRemainingPackageInstallTimeForPackage("org.test.App");
    CHECK(remaining.has_value());
    CHECK(remaining.has_value() &&
          *remaining == std::chrono::seconds{12});

    const auto elapsed =
        flatpak.GetElapsedPackageInstallTimeForPackage("org.test.App");
    CHECK(elapsed.has_value());
    CHECK(elapsed.has_value() && *elapsed == std::chrono::milliseconds{2500});
    CHECK_NEAR(flatpak.GetPackageInstallFraction("org.test.App"), 0.25);

    const std::vector<fpk::Operation> operations =
        flatpak.ListPackageOperations();
    CHECK(operations.size() == 1U);
    CHECK(operations[0].PackageId == "org.test.App");
    CHECK(operations[0].Status == fpk::OperationStatus::Succeeded);
}

TEST(flatpak_install_from_ref_and_run) {
    std::vector<std::vector<std::string>> seen;
    fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            return Ok();
        }
    );

    CHECK(flatpak.InstallFlatpakPackageFromRef(
              "testrepo/org.test.App/beta"
          ).Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "install", "-y", "--noninteractive",
        "testrepo", "org.test.App", "beta"
    }));

    CHECK(flatpak.InstallFlatpakPackageFromRef(
              "testrepo/org.test.App"
          ).Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "install", "-y", "--noninteractive",
        "testrepo", "org.test.App", "stable"
    }));

    CHECK(flatpak.InstallFlatpakPackageFromRef("org.test.App").Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "install", "-y", "--noninteractive",
        "flathub", "org.test.App", "stable"
    }));

    const std::size_t before = seen.size();
    CHECK(!flatpak.InstallFlatpakPackageFromRef("a/b/c/d").Ok);
    CHECK(seen.size() == before);

    CHECK(flatpak.RunFlatpakPackage(
              "org.test.App", {"--debug", "file://test"}
          ).Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "run", "org.test.App", "--debug", "file://test"
    }));
    CHECK(!flatpak.RunFlatpakPackage(std::string{}).Ok);
}

TEST(flatpak_update_uninstall_and_update_list) {
    std::vector<std::vector<std::string>> seen;
    fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            if (argv[1] == "remote-ls") {
                return Ok("org.test.App\norg.other.App\n");
            }
            return Ok("update 50% complete\n",
                      std::chrono::milliseconds{1000});
        }
    );

    CHECK(flatpak.UpdateFlatpakPackage("org.test.App").Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "update", "-y", "--noninteractive", "org.test.App"
    }));
    CHECK(flatpak.GetPackageInstallProgress("org.test.App").Phase ==
          "update");
    CHECK_NEAR(flatpak.GetPackageInstallFraction("org.test.App"), 0.5);

    CHECK(flatpak.UninstallFlatpakPackage("org.test.App").Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "uninstall", "-y", "--noninteractive", "org.test.App"
    }));
    CHECK(flatpak.GetPackageInstallProgress("org.test.App").Phase ==
          "uninstall");

    CHECK(flatpak.UpdateAllFlatpakPackages().Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "update", "-y", "--noninteractive"
    }));
    CHECK(flatpak.GetPackageOperationStatus("all") ==
          fpk::OperationStatus::Succeeded);

    const std::vector<std::string> updates = flatpak.ListFlatpakUpdates();
    CHECK(updates.size() == 2U);
    CHECK(updates[0] == "org.test.App");
    CHECK(updates[1] == "org.other.App");
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "remote-ls", "--updates", "--app",
        "--columns=application"
    }));

    CHECK(!flatpak.UpdateFlatpakPackage(std::string{}).Ok);
    CHECK(!flatpak.UninstallFlatpakPackage(std::string{}).Ok);
}

TEST(flatpak_operation_queue_cancel_clear_and_failure) {
    fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [](const std::vector<std::string>&) {
            return Failed();
        }
    );

    CHECK(flatpak.GetPackageInstallProgress("missing").Phase == "none");
    CHECK_NEAR(flatpak.GetPackageInstallFraction("missing"), 0.0);
    CHECK(flatpak.GetPackageOperationStatus("missing") ==
          fpk::OperationStatus::None);
    CHECK(!flatpak.GetRemainingPackageInstallTimeForPackage(
              "missing"
          ).has_value());
    CHECK(!flatpak.GetElapsedPackageInstallTimeForPackage(
              "missing"
          ).has_value());
    CHECK(flatpak.ListPackageOperations().empty());
    CHECK(!flatpak.IsPackageOperationRunning("missing"));
    CHECK(!flatpak.CancelPackageOperation("missing"));
    CHECK(!flatpak.QueueFlatpakPackageOperation(std::string{}));

    CHECK(flatpak.QueueFlatpakPackageOperation("org.test.App"));
    CHECK(flatpak.GetPackageOperationStatus("org.test.App") ==
          fpk::OperationStatus::Queued);
    CHECK(flatpak.IsPackageOperationRunning("org.test.App"));
    CHECK(flatpak.CancelPackageOperation("org.test.App"));
    CHECK(flatpak.GetPackageOperationStatus("org.test.App") ==
          fpk::OperationStatus::Cancelled);
    CHECK(!flatpak.IsPackageOperationRunning("org.test.App"));
    CHECK(!flatpak.CancelPackageOperation("org.test.App"));

    CHECK(flatpak.QueueFlatpakPackageOperation("org.test.App"));
    flatpak.ClearPackageOperations();
    CHECK(flatpak.ListPackageOperations().empty());
    CHECK(flatpak.GetPackageOperationStatus("org.test.App") ==
          fpk::OperationStatus::None);

    CHECK(!flatpak.InstallFlatpakPackage("org.test.App").Ok);
    CHECK(flatpak.GetPackageOperationStatus("org.test.App") ==
          fpk::OperationStatus::Failed);
}

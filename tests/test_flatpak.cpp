#include "TestFramework.hpp"

#include "flatpak/Flatpak.h"
#include "flatpak/FlatpakCommand.h"

#include <string>
#include <vector>

TEST(flatpak_command_uses_injected_executor) {
    std::vector<std::string> seen;
    fpk::FlatpakCommand command(
        "flatpak-mock",
        [&seen](const std::vector<std::string>& argv) {
            seen = argv;
            fpk::Result result;
            result.Ok = true;
            result.ExitCode = 0;
            result.Output = "mock-output";
            return result;
        }
    );

    CHECK(command.BinaryPath() == "flatpak-mock");
    const fpk::Result result = command.Run({"--version"});
    CHECK(result.Ok);
    CHECK(result.Output == "mock-output");
    CHECK(seen.size() == 2U);
    CHECK(seen[0] == "flatpak-mock");
    CHECK(seen[1] == "--version");

    command.SetBinaryPath("other-flatpak");
    CHECK(command.BinaryPath() == "other-flatpak");
    command.SetExecutor([](const std::vector<std::string>& argv) {
        fpk::Result replacement;
        replacement.Ok = argv.size() == 2U;
        replacement.Output = "replaced";
        return replacement;
    });
    const fpk::Result replaced = command.Run({"info"});
    CHECK(replaced.Ok);
    CHECK(replaced.Output == "replaced");
    CHECK(replaced.Command[0] == "other-flatpak");

    fpk::FlatpakCommand empty("");
    CHECK(!empty.Run({"--version"}).Ok);
}

TEST(flatpak_core_with_mock_executor) {
    fpk::Flatpak flatpak(
        "flatpak",
        [](const std::vector<std::string>& argv) {
            fpk::Result result;
            if (argv.size() == 2U && argv[1] == "--version") {
                result.Ok = true;
                result.ExitCode = 0;
                result.Output = "Flatpak 1.18.4\n";
            } else if (argv.size() > 1U && argv[1] == "remote-list") {
                result.Ok = true;
                result.ExitCode = 0;
                result.Output =
                    "flathub\thttps://dl.flathub.org/repo/\t1\tsystem\n";
            } else {
                result.Error = "unexpected command";
            }
            return result;
        }
    );

    CHECK(flatpak.BinaryPath() == "flatpak");
    CHECK(flatpak.IsAvailable());
    CHECK(flatpak.Version() == "1.18.4");
    CHECK(flatpak.EnsureAvailable().Ok);
    CHECK(flatpak.EnsureAvailable().Output == "default repositories ready");

    const std::vector<std::string> defaults = flatpak.DefaultRepositories();
    CHECK(defaults.size() == 1U);
    CHECK(defaults[0] == "flathub");

    flatpak.SetBinaryPath("flatpak-custom");
    CHECK(flatpak.BinaryPath() == "flatpak-custom");
}

TEST(flatpak_core_failure_and_status_strings) {
    fpk::Flatpak flatpak(
        "flatpak",
        [](const std::vector<std::string>&) {
            fpk::Result result;
            result.ExitCode = 1;
            result.Error = "not installed";
            return result;
        }
    );

    CHECK(!flatpak.IsAvailable());
    CHECK(flatpak.Version().empty());
    CHECK(!flatpak.EnsureAvailable().Ok);
    CHECK(flatpak.EnsureAvailable().Error == "flatpak is not available");

    CHECK(std::string(fpk::ToString(fpk::OperationStatus::None)) == "none");
    CHECK(std::string(fpk::ToString(fpk::OperationStatus::Queued)) == "queued");
    CHECK(std::string(fpk::ToString(fpk::OperationStatus::Running)) == "running");
    CHECK(std::string(fpk::ToString(fpk::OperationStatus::Succeeded)) == "succeeded");
    CHECK(std::string(fpk::ToString(fpk::OperationStatus::Failed)) == "failed");
    CHECK(std::string(fpk::ToString(fpk::OperationStatus::Cancelled)) == "cancelled");
}

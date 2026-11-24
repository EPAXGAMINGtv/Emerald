#include "TestFramework.hpp"

#include "flatpak/Flatpak.h"
#include "flatpak/streaming/OutputCallback.h"

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

} // namespace

TEST(flatpak_output_line_buffer_splits_lines_and_progress) {
    std::vector<std::string> lines;
    fpk::stream::OutputLineBuffer buffer(
        [&lines](const std::string& line) {
            lines.push_back(line);
        }
    );

    buffer.Push("one\ntwo");
    buffer.Push("\rprogress 10%");
    buffer.Push("\r\nthree\n");
    buffer.Flush();

    CHECK(lines == std::vector<std::string>({
        "one\n", "two\r", "progress 10%\r", "three\n"
    }));
}

TEST(flatpak_install_streams_output_live) {
    std::vector<std::string> chunks;
    std::vector<std::vector<std::string>> seen;
    fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [](const std::vector<std::string>&) {
            return Ok();
        }
    );

    flatpak.SetStreamingExecutor(
        [&seen](const std::vector<std::string>& argv,
                const fpk::stream::OutputCallback& onOutput) {
            seen.push_back(argv);
            onOutput("10% done\r");
            onOutput("20% done\n");
            return Ok("20% done\n");
        }
    );

    const fpk::Result result = flatpak.InstallFlatpakPackage(
        "org.test.App",
        "flathub",
        "stable",
        [&chunks](const std::string& chunk) {
            chunks.push_back(chunk);
        }
    );

    CHECK(result.Ok);
    CHECK(chunks == std::vector<std::string>({"10% done\r", "20% done\n"}));
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "install", "-y", "--noninteractive",
        "flathub", "org.test.App", "stable"
    }));
    CHECK(flatpak.GetPackageOperationStatus("org.test.App") ==
          fpk::OperationStatus::Succeeded);
}

TEST(flatpak_global_output_callback_and_command_helper) {
    std::vector<std::string> globalChunks;
    fpk::Flatpak flatpak = fpk::Flatpak(
        "flatpak-mock",
        [](const std::vector<std::string>&) {
            return Ok("captured\n");
        }
    );

    flatpak.SetOutputCallback(
        [&globalChunks](const std::string& chunk) {
            globalChunks.push_back(chunk);
        }
    );
    CHECK(static_cast<bool>(flatpak.OutputCallbackHandler()));

    CHECK(flatpak.InstallFlatpakPackage("org.test.App").Ok);
    CHECK(globalChunks == std::vector<std::string>({"captured\n"}));

    globalChunks.clear();
    std::vector<std::string> directChunks;
    const fpk::Result direct = flatpak.RunFlatpakCommandWithOutput(
        {"info", "org.test.App"},
        [&directChunks](const std::string& chunk) {
            directChunks.push_back(chunk);
        }
    );
    CHECK(direct.Ok);
    CHECK(directChunks == std::vector<std::string>({"captured\n"}));
    CHECK(direct.Command == std::vector<std::string>({
        "flatpak-mock", "info", "org.test.App"
    }));
}

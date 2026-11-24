#include "TestFramework.hpp"

#include "flatpak/Flatpak.h"
#include "flatpak/terminal/TerminalSession.h"

#include <string>
#include <vector>

namespace {

fpk::Result Ok() {
    fpk::Result result;
    result.Ok = true;
    result.ExitCode = 0;
    return result;
}

} // namespace

TEST(flatpak_terminal_session_injected) {
    std::vector<std::vector<std::string>> seen;
    fpk::terminal::TerminalSession session(
        {},
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            return Ok();
        }
    );

    CHECK(session.Options().Shell.empty());
    CHECK(!session.ResolveShell().empty());
    CHECK(session.ShellArguments().size() == 2U);
    CHECK(session.ShellArguments()[1] == "-i");

    CHECK(session.RunInteractive({"printf", "hello"}).Ok);
    CHECK(seen.back() == std::vector<std::string>({"printf", "hello"}));

    CHECK(session.OpenShell().Ok);
    CHECK(seen.back()[0] == session.ResolveShell());
    CHECK(seen.back()[1] == "-i");

    CHECK(!session.RunInteractive({}).Ok);

    fpk::terminal::TerminalOptions options;
    options.Shell = "/bin/bash";
    options.WorkingDirectory = "/tmp";
    session.SetOptions(options);
    CHECK(session.Options().Shell == "/bin/bash");
    CHECK(session.Options().WorkingDirectory == "/tmp");
    CHECK(session.ResolveShell() == "/bin/bash");

    session.SetExecutor(
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            return Ok();
        }
    );
    CHECK(session.OpenShell().Ok);
    CHECK(seen.back() == std::vector<std::string>({"/bin/bash", "-i"}));
}

TEST(flatpak_facade_terminal_calls) {
    std::vector<std::vector<std::string>> seen;
    fpk::Flatpak flatpak(
        "flatpak-mock",
        fpk::CommandExecutor{},
        [&seen](const std::vector<std::string>& argv) {
            seen.push_back(argv);
            return Ok();
        }
    );

    CHECK(flatpak.RunFlatpakCommandInTerminal({
              "install", "-y", "flathub", "org.test.App"
          }).Ok);
    CHECK(seen.back() == std::vector<std::string>({
        "flatpak-mock", "install", "-y", "flathub", "org.test.App"
    }));

    fpk::terminal::TerminalOptions options;
    options.Shell = "/bin/sh";
    flatpak.SetTerminalOptions(options);
    CHECK(flatpak.TerminalConfiguration().Shell == "/bin/sh");

    CHECK(flatpak.OpenFlatpakConsole().Ok);
    CHECK(seen.back() == std::vector<std::string>({"/bin/sh", "-i"}));
}

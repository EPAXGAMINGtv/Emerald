#include "flatpak/Flatpak.h"

#include "FlatpakParse.h"

#include <algorithm>
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace fpk {

namespace {

std::vector<Repository> ParseRepositories(const std::string& output) {
    std::vector<Repository> repositories;

    for (const std::string& line : detail::SplitLines(output)) {
        const std::vector<std::string> fields = detail::SplitTabs(line);
        if (fields.size() < 3U || fields[0].empty()) {
            continue;
        }

        Repository repository;
        repository.Name = fields[0];
        repository.Url = fields[1];

        try {
            repository.Priority = std::stoi(fields[2]);
        } catch (const std::exception&) {
            repository.Priority = 0;
        }

        if (fields.size() > 3U &&
            fields[3].find("disabled") != std::string::npos) {
            repository.Enabled = false;
        }
        repositories.push_back(std::move(repository));
    }

    return repositories;
}

std::string DefaultRepoUrl(const std::string& name) {
    if (name == "flathub") {
        return "https://dl.flathub.org/repo/flathub.flatpakrepo";
    }
    return {};
}

} // namespace

Result Flatpak::EnsureDefaultReposConfigured() const {
    Result result;
    result.Ok = true;

    for (const std::string& name : DefaultRepositories()) {
        if (!IsDefaultRepoConfigured(name) || FlatpakRepoExists(name)) {
            continue;
        }

        const std::string url = DefaultRepoUrl(name);
        if (url.empty()) {
            result.Ok = false;
            result.Error = "no default url for repository " + name;
            return result;
        }

        const Result step = AddFlatpakRepo(name, url);
        if (!step.Ok) {
            result.Ok = false;
            result.Error = "failed to add " + name + ": " + step.Error;
            result.Command = step.Command;
            result.ExitCode = step.ExitCode;
            return result;
        }
    }

    result.Output = "default repositories ready";
    return result;
}

Result Flatpak::AddFlatpakRepo(
    const std::string& name, const std::string& url, bool ifNotExists
) const {
    std::vector<std::string> arguments = {"remote-add"};
    if (ifNotExists) {
        arguments.push_back("--if-not-exists");
    }
    arguments.push_back(name);
    arguments.push_back(url);
    return command_.Run(std::move(arguments));
}

Result Flatpak::AddDefaultRepos() const {
    Result result;
    result.Ok = true;

    for (const std::string& name : DefaultRepositories()) {
        const std::string url = DefaultRepoUrl(name);
        if (url.empty()) {
            result.Ok = false;
            result.Error = "no default url for repository " + name;
            return result;
        }

        const Result step = AddFlatpakRepo(name, url);
        if (!step.Ok) {
            result.Ok = false;
            result.Error = "failed to add " + name + ": " + step.Error;
            result.Command = step.Command;
            result.ExitCode = step.ExitCode;
            return result;
        }
    }

    return result;
}

bool Flatpak::IsDefaultRepoConfigured(const std::string& name) const {
    const std::vector<std::string> defaults = DefaultRepositories();
    return std::find(defaults.begin(), defaults.end(), name) != defaults.end();
}

Result Flatpak::RemoveFlatpakRepo(const std::string& name) {
    return command_.Run({"remote-delete", name});
}

std::vector<Repository> Flatpak::ListFlatpakRepos(
    bool includeDisabled
) const {
    std::vector<std::string> arguments = {
        "remote-list",
        "--columns=name,url,priority,options"
    };
    if (includeDisabled) {
        arguments.push_back("--show-disabled");
    }

    const Result result = command_.Run(std::move(arguments));
    if (!result.Ok) {
        return {};
    }
    return ParseRepositories(result.Output);
}

bool Flatpak::FlatpakRepoExists(const std::string& name) const {
    return GetFlatpakRepo(name).has_value();
}

std::optional<Repository> Flatpak::GetFlatpakRepo(
    const std::string& name
) const {
    const std::vector<Repository> repositories = ListFlatpakRepos();
    const auto found = std::find_if(
        repositories.begin(),
        repositories.end(),
        [&name](const Repository& repository) {
            return repository.Name == name;
        }
    );
    if (found == repositories.end()) {
        return std::nullopt;
    }
    return *found;
}

std::string Flatpak::GetFlatpakRepoUrl(const std::string& name) const {
    const std::optional<Repository> repository = GetFlatpakRepo(name);
    return repository.has_value() ? repository->Url : std::string{};
}

Result Flatpak::EnableFlatpakRepo(const std::string& name) {
    return command_.Run({"remote-modify", "--enable", name});
}

Result Flatpak::DisableFlatpakRepo(const std::string& name) {
    return command_.Run({"remote-modify", "--disable", name});
}

Result Flatpak::UpdateFlatpakRepo(const std::string& name) {
    return command_.Run({"remote-update", name});
}

Result Flatpak::UpdateAllFlatpakRepos() {
    return command_.Run({"remote-update"});
}

Result Flatpak::SetFlatpakRepoPriority(
    const std::string& name, int priority
) {
    return command_.Run({
        "remote-modify",
        "--prio=" + std::to_string(priority),
        name
    });
}

std::vector<std::string> Flatpak::ListFlatpakRefs(
    const std::string& remote
) const {
    std::vector<std::string> arguments = {
        "remote-ls",
        "--app",
        "--columns=ref"
    };
    if (!remote.empty()) {
        arguments.push_back(remote);
    }

    const Result result = command_.Run(std::move(arguments));
    if (!result.Ok) {
        return {};
    }

    std::vector<std::string> refs;
    for (const std::string& line : detail::SplitLines(result.Output)) {
        if (!line.empty()) {
            refs.push_back(line);
        }
    }
    return refs;
}

} // namespace fpk

#include "flatpak/Flatpak.h"

#include "FlatpakParse.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fpk {

namespace {

const char* const kCatalogColumns =
    "application,name,description,version,branch,origin,options";
const char* const kSearchColumns =
    "name,description,application,version,branch,remotes";

std::string FirstLine(const std::string& text) {
    const std::size_t newline = text.find('\n');
    return newline == std::string::npos ? text : text.substr(0, newline);
}

std::string FirstToken(const std::string& text, char delimiter) {
    const std::size_t found = text.find(delimiter);
    return found == std::string::npos ? text : text.substr(0, found);
}

std::vector<Application> ParseCatalog(const std::string& output) {
    std::vector<Application> applications;

    for (const std::string& line : detail::SplitLines(output)) {
        const std::vector<std::string> fields = detail::SplitTabs(line);
        if (fields.size() < 6U || fields[0].empty()) {
            continue;
        }

        Application application;
        application.Id = fields[0];
        application.Name = fields[1];
        application.Summary = FirstLine(fields[2]);
        application.Description = fields[2];
        application.Version = fields[3];
        application.Branch = fields[4];
        application.Remote = fields[5];
        applications.push_back(std::move(application));
    }

    return applications;
}

std::vector<Application> ParseSearchResults(const std::string& output) {
    std::vector<Application> applications;

    for (const std::string& line : detail::SplitLines(output)) {
        const std::vector<std::string> fields = detail::SplitTabs(line);
        if (fields.size() < 6U || fields[2].empty()) {
            continue;
        }

        Application application;
        application.Name = fields[0];
        application.Summary = FirstLine(fields[1]);
        application.Description = fields[1];
        application.Id = fields[2];
        application.Version = fields[3];
        application.Branch = fields[4];
        application.Remote = FirstToken(fields[5], ',');
        applications.push_back(std::move(application));
    }

    return applications;
}

std::optional<Application> FindById(
    const std::vector<Application>& applications, const std::string& id
) {
    const auto found = std::find_if(
        applications.begin(),
        applications.end(),
        [&id](const Application& application) {
            return application.Id == id;
        }
    );
    if (found == applications.end()) {
        return std::nullopt;
    }
    return *found;
}

std::vector<std::string> IdsOf(const std::vector<Application>& applications) {
    std::vector<std::string> ids;
    ids.reserve(applications.size());
    for (const Application& application : applications) {
        ids.push_back(application.Id);
    }
    return ids;
}

} // namespace

std::vector<Application> Flatpak::GetAllApplicationsAvailable() const {
    const Result result = command_.Run({
        "remote-ls",
        "--app",
        std::string("--columns=") + kCatalogColumns
    });
    if (!result.Ok) {
        return {};
    }
    return ParseCatalog(result.Output);
}

std::vector<Application> Flatpak::GetAllInstalledApplications() const {
    const Result result = command_.Run({
        "list",
        "--app",
        std::string("--columns=") + kCatalogColumns
    });
    if (!result.Ok) {
        return {};
    }
    return ParseCatalog(result.Output);
}

std::optional<Application> Flatpak::GetApplicationById(
    const std::string& id
) const {
    if (id.empty()) {
        return std::nullopt;
    }

    const std::optional<Application> available =
        FindById(GetAllApplicationsAvailable(), id);
    if (available.has_value()) {
        return available;
    }
    return FindById(GetAllInstalledApplications(), id);
}

bool Flatpak::IsApplicationAvailable(const std::string& id) const {
    if (id.empty()) {
        return false;
    }
    return FindById(GetAllApplicationsAvailable(), id).has_value();
}

bool Flatpak::IsApplicationInstalled(const std::string& id) const {
    if (id.empty()) {
        return false;
    }
    return FindById(GetAllInstalledApplications(), id).has_value();
}

std::string Flatpak::GetApplicationName(const std::string& id) const {
    const std::optional<Application> application = GetApplicationById(id);
    return application.has_value() ? application->Name : std::string{};
}

std::string Flatpak::GetApplicationSummary(const std::string& id) const {
    const std::optional<Application> application = GetApplicationById(id);
    return application.has_value() ? application->Summary : std::string{};
}

std::string Flatpak::GetApplicationDescription(const std::string& id) const {
    const std::optional<Application> application = GetApplicationById(id);
    return application.has_value() ? application->Description : std::string{};
}

std::string Flatpak::GetApplicationVersion(const std::string& id) const {
    const std::optional<Application> application = GetApplicationById(id);
    return application.has_value() ? application->Version : std::string{};
}

std::vector<std::string> Flatpak::GetApplicationCategories(
    const std::string& id
) const {
    const std::optional<Application> application = GetApplicationById(id);
    return application.has_value() ? application->Categories
                                   : std::vector<std::string>{};
}

std::vector<Application> Flatpak::SearchFlatpakApplications(
    const std::string& query
) const {
    if (query.empty()) {
        return {};
    }

    const Result result = command_.Run({
        "search",
        std::string("--columns=") + kSearchColumns,
        query
    });
    if (!result.Ok) {
        return {};
    }
    return ParseSearchResults(result.Output);
}

std::vector<Application> Flatpak::GetApplicationsByRemote(
    const std::string& remote
) const {
    if (remote.empty()) {
        return {};
    }

    std::vector<Application> matches;
    for (const Application& application : GetAllApplicationsAvailable()) {
        if (application.Remote == remote) {
            matches.push_back(application);
        }
    }
    return matches;
}

std::vector<std::string> Flatpak::ListAvailableApplicationIds() const {
    return IdsOf(GetAllApplicationsAvailable());
}

std::vector<std::string> Flatpak::ListInstalledApplicationIds() const {
    return IdsOf(GetAllInstalledApplications());
}

} // namespace fpk

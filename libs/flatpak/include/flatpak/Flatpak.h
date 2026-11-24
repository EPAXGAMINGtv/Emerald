#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "flatpak/FlatpakCommand.h"
#include "flatpak/FlatpakTypes.h"

namespace fpk {

// Central facade for the Flatpak app-store library. More grouped APIs are
// added on this class, so callers only need one object.
class Flatpak {
public:
    explicit Flatpak(
        std::string binaryPath = "flatpak",
        CommandExecutor executor = {}
    );

    void SetBinaryPath(std::string binaryPath);
    const std::string& BinaryPath() const noexcept;
    void SetExecutor(CommandExecutor executor);

    bool IsAvailable() const;
    std::string Version() const;
    Result EnsureAvailable() const;

    std::vector<std::string> DefaultRepositories() const;

    // Repository group
    Result AddFlatpakRepo(
        const std::string& name,
        const std::string& url,
        bool ifNotExists = true
    );
    Result AddDefaultRepos();
    bool IsDefaultRepoConfigured(const std::string& name) const;
    Result RemoveFlatpakRepo(const std::string& name);
    std::vector<Repository> ListFlatpakRepos(
        bool includeDisabled = true
    ) const;
    bool FlatpakRepoExists(const std::string& name) const;
    std::optional<Repository> GetFlatpakRepo(const std::string& name) const;
    std::string GetFlatpakRepoUrl(const std::string& name) const;
    Result EnableFlatpakRepo(const std::string& name);
    Result DisableFlatpakRepo(const std::string& name);
    Result UpdateFlatpakRepo(const std::string& name);
    Result UpdateAllFlatpakRepos();
    Result SetFlatpakRepoPriority(const std::string& name, int priority);
    std::vector<std::string> ListFlatpakRefs(const std::string& remote) const;

    // Application/catalog group
    std::vector<Application> GetAllApplicationsAvailable() const;
    std::vector<Application> GetAllInstalledApplications() const;
    std::optional<Application> GetApplicationById(const std::string& id) const;
    bool IsApplicationAvailable(const std::string& id) const;
    bool IsApplicationInstalled(const std::string& id) const;
    std::string GetApplicationName(const std::string& id) const;
    std::string GetApplicationSummary(const std::string& id) const;
    std::string GetApplicationDescription(const std::string& id) const;
    std::string GetApplicationVersion(const std::string& id) const;
    std::vector<std::string> GetApplicationCategories(
        const std::string& id
    ) const;
    std::vector<Application> SearchFlatpakApplications(
        const std::string& query
    ) const;
    std::vector<Application> GetApplicationsByRemote(
        const std::string& remote
    ) const;
    std::vector<Application> GetApplicationsByCategory(
        const std::string& category
    ) const;
    std::vector<std::string> ListAvailableApplicationIds() const;
    std::vector<std::string> ListInstalledApplicationIds() const;

    // Icon group
    void SetIconCacheDirectory(std::filesystem::path directory);
    const std::filesystem::path& IconCacheDirectory() const noexcept;
    std::filesystem::path GetApplicationIconPath(
        const std::string& id
    ) const;
    std::filesystem::path GetApplicationIcon(const std::string& id) const;
    bool HasApplicationIcon(const std::string& id) const;
    bool SetApplicationIcon(
        const std::string& id, std::filesystem::path path
    );
    bool ClearApplicationIcon(const std::string& id);
    void ClearApplicationIcons();

    // Install/update/progress group
    Result InstallFlatpakPackage(
        const std::string& packageId,
        const std::string& remote = "flathub",
        const std::string& branch = "stable"
    );
    Result InstallFlatpakPackageFromRef(const std::string& ref);
    Result UninstallFlatpakPackage(const std::string& packageId);
    Result UpdateFlatpakPackage(const std::string& packageId);
    Result UpdateAllFlatpakPackages();
    std::vector<std::string> ListFlatpakUpdates() const;
    Result RunFlatpakPackage(
        const std::string& packageId,
        const std::vector<std::string>& arguments = {}
    );

    Progress GetPackageInstallProgress(const std::string& packageId) const;
    std::optional<std::chrono::milliseconds>
    GetRemainingPackageInstallTimeForPackage(
        const std::string& packageId
    ) const;
    std::optional<std::chrono::milliseconds>
    GetElapsedPackageInstallTimeForPackage(const std::string& packageId) const;
    double GetPackageInstallFraction(const std::string& packageId) const;
    OperationStatus GetPackageOperationStatus(
        const std::string& packageId
    ) const;
    std::vector<Operation> ListPackageOperations() const;
    bool IsPackageOperationRunning(const std::string& packageId) const;
    bool QueueFlatpakPackageOperation(const std::string& packageId);
    bool CancelPackageOperation(const std::string& packageId);
    void ClearPackageOperations();

    // In-memory grouping group
    Result CreateFlatpakGroup(
        const std::string& id, const std::string& name = {}
    );
    bool RemoveFlatpakGroup(const std::string& id);
    bool RenameFlatpakGroup(const std::string& id, const std::string& name);
    bool AddApplicationToGroup(
        const std::string& groupId, const std::string& applicationId
    );
    bool RemoveApplicationFromGroup(
        const std::string& groupId, const std::string& applicationId
    );
    std::optional<Group> GetFlatpakGroup(const std::string& id) const;
    std::vector<Group> ListFlatpakGroups() const;
    std::vector<std::string> ListFlatpakGroupIds() const;
    std::vector<std::string> GetApplicationsInGroup(
        const std::string& groupId
    ) const;
    std::vector<std::string> GetGroupsForApplication(
        const std::string& applicationId
    ) const;
    bool FlatpakGroupExists(const std::string& id) const;
    bool SetApplicationCategories(
        const std::string& applicationId,
        std::vector<std::string> categories
    );
    bool ClearApplicationCategories(const std::string& applicationId);
    void ClearFlatpakGroups();

private:
    FlatpakCommand command_;
    std::filesystem::path iconCacheDirectory_;
    std::unordered_map<std::string, std::filesystem::path> iconOverrides_;
    std::unordered_map<std::string, Operation> operations_;
    std::unordered_map<std::string, Group> groups_;
    std::unordered_map<std::string, std::vector<std::string>>
        categoryOverrides_;
};

} // namespace fpk

#include "flatpak/Flatpak.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace fpk {

namespace {

Result GroupFailure(std::string message) {
    Result result;
    result.Error = std::move(message);
    return result;
}

} // namespace

Result Flatpak::CreateFlatpakGroup(
    const std::string& id, const std::string& name
) {
    if (id.empty()) {
        return GroupFailure("group id is required");
    }
    if (groups_.find(id) != groups_.end()) {
        return GroupFailure("group already exists: " + id);
    }

    Group group;
    group.Id = id;
    group.Name = name.empty() ? id : name;
    groups_[id] = std::move(group);

    Result result;
    result.Ok = true;
    result.Output = id;
    return result;
}

bool Flatpak::RemoveFlatpakGroup(const std::string& id) {
    return groups_.erase(id) > 0U;
}

bool Flatpak::RenameFlatpakGroup(
    const std::string& id, const std::string& name
) {
    if (name.empty()) {
        return false;
    }

    const auto found = groups_.find(id);
    if (found == groups_.end()) {
        return false;
    }
    found->second.Name = name;
    return true;
}

bool Flatpak::AddApplicationToGroup(
    const std::string& groupId, const std::string& applicationId
) {
    if (groupId.empty() || applicationId.empty()) {
        return false;
    }

    const auto found = groups_.find(groupId);
    if (found == groups_.end()) {
        return false;
    }

    const std::vector<std::string>& members = found->second.ApplicationIds;
    if (std::find(members.begin(), members.end(), applicationId) !=
        members.end()) {
        return false;
    }

    found->second.ApplicationIds.push_back(applicationId);
    return true;
}

bool Flatpak::RemoveApplicationFromGroup(
    const std::string& groupId, const std::string& applicationId
) {
    const auto found = groups_.find(groupId);
    if (found == groups_.end()) {
        return false;
    }

    std::vector<std::string>& members = found->second.ApplicationIds;
    const auto member = std::find(
        members.begin(), members.end(), applicationId
    );
    if (member == members.end()) {
        return false;
    }

    members.erase(member);
    return true;
}

std::optional<Group> Flatpak::GetFlatpakGroup(const std::string& id) const {
    const auto found = groups_.find(id);
    if (found == groups_.end()) {
        return std::nullopt;
    }
    return found->second;
}

std::vector<Group> Flatpak::ListFlatpakGroups() const {
    std::vector<Group> groups;
    groups.reserve(groups_.size());
    for (const auto& entry : groups_) {
        groups.push_back(entry.second);
    }
    return groups;
}

std::vector<std::string> Flatpak::ListFlatpakGroupIds() const {
    std::vector<std::string> ids;
    ids.reserve(groups_.size());
    for (const auto& entry : groups_) {
        ids.push_back(entry.first);
    }
    return ids;
}

std::vector<std::string> Flatpak::GetApplicationsInGroup(
    const std::string& groupId
) const {
    const auto found = groups_.find(groupId);
    if (found == groups_.end()) {
        return {};
    }
    return found->second.ApplicationIds;
}

std::vector<std::string> Flatpak::GetGroupsForApplication(
    const std::string& applicationId
) const {
    std::vector<std::string> groupIds;
    for (const auto& entry : groups_) {
        const std::vector<std::string>& members = entry.second.ApplicationIds;
        const auto member = std::find(
            members.begin(), members.end(), applicationId
        );
        if (member != members.end()) {
            groupIds.push_back(entry.first);
        }
    }
    return groupIds;
}

bool Flatpak::FlatpakGroupExists(const std::string& id) const {
    return groups_.find(id) != groups_.end();
}

bool Flatpak::SetApplicationCategories(
    const std::string& applicationId, std::vector<std::string> categories
) {
    if (applicationId.empty()) {
        return false;
    }
    categoryOverrides_[applicationId] = std::move(categories);
    return true;
}

bool Flatpak::ClearApplicationCategories(
    const std::string& applicationId
) {
    return categoryOverrides_.erase(applicationId) > 0U;
}

void Flatpak::ClearFlatpakGroups() {
    groups_.clear();
}

} // namespace fpk

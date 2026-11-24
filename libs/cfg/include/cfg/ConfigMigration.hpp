#pragma once

#include <functional>
#include <string>
#include <vector>

#include "cfg/Config.hpp"

namespace cfg {

// Sequential, version-keyed migrations. Each step upgrades one version and
// receives the Config, so callers decide how their keys change. Steps are only
// applied forward and the version is written after a successful step.
class ConfigMigration {
public:
    struct Migration {
        int from = 0;
        int to = 0;
        std::string description;
        std::function<bool(Config&)> action;
    };

    struct Result {
        bool ok = false;
        int from = 0;
        int to = 0;
        std::vector<std::string> applied;
        std::string error;
    };

    explicit ConfigMigration(std::string version_key = "schema.version");

    bool add(
        int from,
        int to,
        std::function<bool(Config&)> action,
        std::string description = {}
    );

    const std::string& version_key() const noexcept;
    const std::vector<Migration>& migrations() const noexcept;

    int current_version(const Config& config) const;
    void set_version(Config& config, int version) const;
    int latest_version() const noexcept;

    std::vector<const Migration*> path(int from, int to) const;
    bool can_migrate(int from, int to) const;

    Result migrate(Config& config, int target) const;
    Result migrate_to_latest(Config& config) const;

private:
    const Migration* find_step(int from) const noexcept;

    std::string version_key_;
    std::vector<Migration> migrations_;
};

} // namespace cfg

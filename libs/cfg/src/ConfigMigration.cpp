#include "cfg/ConfigMigration.hpp"

#include <algorithm>
#include <utility>

namespace cfg {

ConfigMigration::ConfigMigration(std::string version_key)
    : version_key_(std::move(version_key)) {}

bool ConfigMigration::add(
    int from,
    int to,
    std::function<bool(Config&)> action,
    std::string description
) {
    if (from >= to || !action || find_step(from) != nullptr) {
        return false;
    }

    Migration migration;
    migration.from = from;
    migration.to = to;
    migration.description = std::move(description);
    migration.action = std::move(action);
    migrations_.push_back(std::move(migration));
    return true;
}

const std::string& ConfigMigration::version_key() const noexcept {
    return version_key_;
}

const std::vector<ConfigMigration::Migration>& ConfigMigration::migrations(
) const noexcept {
    return migrations_;
}

int ConfigMigration::current_version(const Config& config) const {
    return config.get_int(version_key_, 0);
}

void ConfigMigration::set_version(Config& config, int version) const {
    config.set(version_key_, version);
}

int ConfigMigration::latest_version() const noexcept {
    int latest = 0;
    for (const Migration& migration : migrations_) {
        if (migration.to > latest) {
            latest = migration.to;
        }
    }
    return latest;
}

std::vector<const ConfigMigration::Migration*> ConfigMigration::path(
    int from, int to
) const {
    std::vector<const Migration*> result;
    if (from >= to) {
        return result;
    }

    int current = from;
    while (current < to) {
        const Migration* step = find_step(current);
        if (step == nullptr || step->to <= current || step->to > to) {
            return {};
        }
        result.push_back(step);
        current = step->to;
    }
    return result;
}

bool ConfigMigration::can_migrate(int from, int to) const {
    return !path(from, to).empty();
}

ConfigMigration::Result ConfigMigration::migrate(
    Config& config, int target
) const {
    Result result;
    result.from = current_version(config);
    result.to = target;

    if (result.from == target) {
        result.ok = true;
        return result;
    }

    const std::vector<const Migration*> steps = path(result.from, target);
    if (steps.empty()) {
        result.error = "no migration path";
        return result;
    }

    for (const Migration* step : steps) {
        if (!step->action) {
            result.error = "migration has no action";
            return result;
        }
        if (!step->action(config)) {
            result.error = "migration failed at version " +
                std::to_string(step->from);
            return result;
        }

        set_version(config, step->to);
        if (step->description.empty()) {
            result.applied.push_back(
                std::to_string(step->from) + " -> " + std::to_string(step->to)
            );
        } else {
            result.applied.push_back(step->description);
        }
    }

    result.ok = true;
    return result;
}

ConfigMigration::Result ConfigMigration::migrate_to_latest(Config& config) const {
    return migrate(config, latest_version());
}

const ConfigMigration::Migration* ConfigMigration::find_step(
    int from
) const noexcept {
    const auto found = std::find_if(
        migrations_.begin(),
        migrations_.end(),
        [from](const Migration& migration) {
            return migration.from == from;
        }
    );
    return found == migrations_.end() ? nullptr : &*found;
}

} // namespace cfg

#include "TestFramework.hpp"

#include "cfg/ConfigMigration.hpp"

#include <string>

TEST(config_migration_applies_steps_in_order) {
    cfg::ConfigMigration migrations;
    CHECK(migrations.version_key() == "schema.version");
    CHECK(migrations.latest_version() == 0);

    CHECK(migrations.add(
        1,
        2,
        [](cfg::Config& config) {
            config.set("name", config.get_string("old.name"));
            config.remove("old.name");
            return true;
        },
        "move name"
    ));
    CHECK(migrations.add(
        2,
        3,
        [](cfg::Config& config) {
            config.set("flag", true);
            return true;
        },
        "enable flag"
    ));
    CHECK(migrations.latest_version() == 3);
    CHECK(migrations.migrations().size() == 2U);
    CHECK(migrations.can_migrate(1, 3));
    CHECK(!migrations.can_migrate(3, 1));
    CHECK(migrations.path(1, 3).size() == 2U);

    cfg::Config config;
    config.set("schema.version", 1);
    config.set("old.name", std::string("Legacy"));
    CHECK(migrations.current_version(config) == 1);

    const cfg::ConfigMigration::Result result = migrations.migrate(config, 3);
    CHECK(result.ok);
    CHECK(result.from == 1);
    CHECK(result.to == 3);
    CHECK(result.applied.size() == 2U);
    CHECK(result.applied[0] == "move name");
    CHECK(result.applied[1] == "enable flag");
    CHECK(result.error.empty());
    CHECK(config.get_string("name") == "Legacy");
    CHECK(!config.contains("old.name"));
    CHECK(config.get_bool("flag"));
    CHECK(migrations.current_version(config) == 3);

    const cfg::ConfigMigration::Result again =
        migrations.migrate_to_latest(config);
    CHECK(again.ok);
    CHECK(again.from == 3);
    CHECK(again.to == 3);
    CHECK(again.applied.empty());
}

TEST(config_migration_reports_failures) {
    cfg::ConfigMigration migrations;
    CHECK(migrations.add(1, 2, [](cfg::Config&) { return true; }));
    CHECK(migrations.add(2, 3, [](cfg::Config&) { return true; }));
    CHECK(migrations.add(3, 4, [](cfg::Config&) { return false; }, "broken"));

    cfg::Config config;
    config.set("schema.version", 1);
    const cfg::ConfigMigration::Result failed = migrations.migrate(config, 4);

    CHECK(!failed.ok);
    CHECK(failed.from == 1);
    CHECK(failed.to == 4);
    CHECK(failed.applied.size() == 2U);
    CHECK(!failed.error.empty());
    CHECK(migrations.current_version(config) == 3);

    cfg::Config missing;
    missing.set("schema.version", 5);
    const cfg::ConfigMigration::Result no_path = migrations.migrate(missing, 9);
    CHECK(!no_path.ok);
    CHECK(no_path.error == "no migration path");
    CHECK(no_path.applied.empty());
}

TEST(config_migration_registration_and_custom_key) {
    cfg::ConfigMigration migrations;
    CHECK(!migrations.add(2, 2, [](cfg::Config&) { return true; }));
    CHECK(!migrations.add(3, 1, [](cfg::Config&) { return true; }));
    CHECK(migrations.migrations().empty());

    CHECK(migrations.add(1, 2, [](cfg::Config&) { return true; }));
    CHECK(!migrations.add(1, 2, [](cfg::Config&) { return true; }));
    CHECK(migrations.migrations().size() == 1U);
    CHECK(migrations.path(1, 2).size() == 1U);
    CHECK(migrations.path(2, 3).empty());
    CHECK(!migrations.can_migrate(2, 1));
    CHECK(migrations.latest_version() == 2);

    cfg::Config config;
    config.set("meta.version", 1);
    cfg::ConfigMigration custom("meta.version");
    CHECK(custom.version_key() == "meta.version");
    CHECK(custom.current_version(config) == 1);
    CHECK(custom.add(1, 2, [](cfg::Config&) { return true; }));

    custom.set_version(config, 7);
    CHECK(config.get_int("meta.version") == 7);
    custom.set_version(config, 1);

    const cfg::ConfigMigration::Result result = custom.migrate_to_latest(config);
    CHECK(result.ok);
    CHECK(result.applied.size() == 1U);
    CHECK(result.applied[0] == "1 -> 2");
    CHECK(config.get_int("meta.version") == 2);
}

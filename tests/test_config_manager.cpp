#include "TestFramework.hpp"
#include "TestPaths.hpp"

#include "cfg/ConfigManager.hpp"

#include <chrono>
#include <filesystem>

TEST(manager_named_files) {
    test::TempDir dir("manager_files");
    cfg::ConfigManager manager(dir.path());

    cfg::Config& settings = manager.add("settings", "settings.yaml",
        [](cfg::Config& config) {
            config.set_default("app.name", std::string("Emerald"));
        });
    manager.add("layout", "layout.yaml");

    CHECK(manager.contains("settings"));
    CHECK(manager.names().size() == 2U);
    CHECK(manager.get("settings").get_string("app.name") == "Emerald");
    CHECK(settings.path() == dir.path() / "settings.yaml");
    CHECK(manager.find("missing") == nullptr);

    CHECK(manager.load_all_or_create());
    CHECK(std::filesystem::exists(dir.path() / "settings.yaml"));
    CHECK(std::filesystem::exists(dir.path() / "layout.yaml"));

    manager.get("layout").set("columns", 3);
    CHECK(manager.save_all());
    CHECK(manager.save("settings", cfg::SaveMode::Overrides));
    CHECK(manager.save("missing") == false);

    CHECK(manager.remove("layout"));
    CHECK(!manager.contains("layout"));
}

TEST(manager_base_dir_change) {
    test::TempDir first("manager_dir_a");
    test::TempDir second("manager_dir_b");

    cfg::ConfigManager manager(first.path());
    manager.add("settings", "settings.yaml");
    CHECK(manager.get("settings").path() == first.path() / "settings.yaml");

    manager.set_base_dir(second.path());
    CHECK(manager.base_dir() == second.path());
    CHECK(manager.get("settings").path() == second.path() / "settings.yaml");
    CHECK(manager.load_or_create("settings"));
    CHECK(std::filesystem::exists(second.path() / "settings.yaml"));
}

TEST(manager_watch_and_poll) {
    test::TempDir dir("manager_watch");
    cfg::ConfigManager manager(dir.path());
    manager.add("settings", "settings.yaml", [](cfg::Config& config) {
        config.set_default("app.name", std::string("Emerald"));
    });
    CHECK(manager.load_or_create("settings"));

    int notifications = 0;
    CHECK(manager.watch("settings", std::chrono::seconds(30),
        [&notifications](std::string_view name, const cfg::Config&) {
            CHECK(name == "settings");
            ++notifications;
        }));
    CHECK(!manager.poll());
    CHECK(manager.unwatch("settings"));
    CHECK(!manager.unwatch("settings"));
    CHECK(!manager.watch("missing", std::chrono::seconds(30)));
}

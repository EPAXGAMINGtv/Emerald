#include "TestFramework.hpp"
#include "TestPaths.hpp"

#include "cfg/Config.hpp"

#include <filesystem>
#include <fstream>
#include <string>

TEST(config_defaults_and_overrides) {
    cfg::Config config;
    config.define<std::string>("app.name", "Emerald", cfg::non_empty());
    config.define<bool>("gui.enabled", true);
    config.define<double>("audio.volume", 35.0, cfg::range(0.0, 100.0));

    CHECK(config.get_string("app.name") == "Emerald");
    CHECK(config.get_bool("gui.enabled"));
    CHECK_NEAR(config.get_double("audio.volume"), 35.0);
    CHECK(config.contains("app.name"));

    config.set("audio.volume", 42.0);
    CHECK_NEAR(config.get_double("audio.volume"), 42.0);
    CHECK_NEAR(config.get_optional<double>("audio.volume").value(), 42.0);

    config.reset("audio.volume");
    CHECK_NEAR(config.get_double("audio.volume"), 35.0);
    CHECK(!config.get_optional<int>("app.name").has_value());

    config.remove("app.name");
    CHECK(!config.contains("app.name"));
    CHECK(config.get_string("app.name", "fallback") == "fallback");
}

TEST(config_nested_keys) {
    cfg::Config config;
    config.set("server.host", std::string("localhost"));
    config.set("server.port", 8080);
    config.set_default("logging.file", std::string("emerald.log"));

    CHECK(config.get_string("server.host") == "localhost");
    CHECK(config.get_int("server.port") == 8080);
    CHECK(config.get_string("logging.file") == "emerald.log");
    CHECK(config.contains("server.port"));

    CHECK(config.reset("logging.file"));
    CHECK(config.get_string("logging.file") == "emerald.log");
    CHECK(config.remove("server.host"));
    CHECK(!config.contains("server.host"));
    CHECK(config.contains("server.port"));
}

TEST(config_validation) {
    cfg::Config config;
    config.define<std::string>("logging.level", "info",
                               cfg::one_of({"debug", "info", "warn", "error"}));
    config.define<int>("ui.max_results", 50, cfg::range(10, 100));
    config.define<std::string>("app.name", "Emerald", cfg::non_empty());

    CHECK(config.validate().empty());

    config.set_string("logging.level", "verbose");
    config.set("ui.max_results", 5);
    CHECK(config.validate().size() == 2U);

    config.set_string("logging.level", "warn");
    config.set("ui.max_results", 50);
    CHECK(config.validate().empty());
}

TEST(config_change_callbacks) {
    cfg::Config config;
    config.set_default("audio.volume", 35.0);

    int changes = 0;
    double last = 0.0;
    config.on_change_any("audio.volume", [&changes](std::string_view, const YAML::Node&) noexcept {
        ++changes;
    });
    config.on_change<double>("audio.volume", [&last](double value) noexcept {
        last = value;
    });

    config.set("audio.volume", 55.0);
    CHECK(changes == 1);
    CHECK_NEAR(last, 55.0);

    config.set("unrelated.key", 1);
    CHECK(changes == 1);

    config.reset("audio.volume");
    CHECK(changes == 2);
    CHECK_NEAR(last, 35.0);

    config.remove("audio.volume");
    CHECK(changes == 3);
}

TEST(config_save_modes_and_atomic_write) {
    test::TempDir dir("save_modes");
    const std::filesystem::path file = dir.path() / "settings.yaml";

    cfg::Config config(file);
    config.define<std::string>("app.name", "Emerald");
    config.define<bool>("gui.enabled", true);
    config.set("gui.enabled", false);
    CHECK(config.save(cfg::SaveMode::Effective));
    CHECK(!std::filesystem::exists(file.string() + ".tmp"));

    cfg::Config reloaded(file);
    reloaded.define<std::string>("app.name", "Emerald");
    reloaded.define<bool>("gui.enabled", true);
    CHECK(reloaded.load());
    CHECK(reloaded.get_string("app.name") == "Emerald");
    CHECK(!reloaded.get_bool("gui.enabled"));
    CHECK(reloaded.path() == file);

    cfg::Config overrides;
    overrides.set_save_mode(cfg::SaveMode::Overrides);
    overrides.define<std::string>("app.name", "Emerald");
    overrides.define<bool>("gui.enabled", true);
    overrides.set("gui.enabled", false);
    const std::string dumped = overrides.dump();
    cfg::Config view;
    CHECK(view.load_from_string(dumped));
    CHECK(!view.get_bool("gui.enabled", true));
    CHECK(!view.contains("app.name"));
    CHECK(overrides.save_mode() == cfg::SaveMode::Overrides);
}

TEST(config_strict_load_and_reload) {
    test::TempDir dir("strict_load");
    const std::filesystem::path file = dir.path() / "settings.yaml";
    const std::filesystem::path bad = dir.path() / "bad.yaml";

    cfg::Config config(file);
    config.define<std::string>("app.name", "Emerald");
    CHECK(!config.load());
    CHECK(config.get_string("app.name") == "Emerald");
    CHECK(config.load_or_create());
    CHECK(std::filesystem::exists(file));

    config.set_string("app.name", "Updated");
    CHECK(config.save());

    int changes = 0;
    config.on_change_any("app.name", [&changes](std::string_view, const YAML::Node&) noexcept {
        ++changes;
    });

    {
        std::ofstream disk_out(file, std::ios::out | std::ios::trunc);
        disk_out << "app:\n  name: FromDisk\n";
    }
    CHECK(config.reload());
    CHECK(changes == 1);
    CHECK(config.get_string("app.name") == "FromDisk");

    {
        std::ofstream bad_out(bad, std::ios::out | std::ios::trunc);
        bad_out << "::not: [valid\n";
    }
    CHECK(!config.load(bad));
    CHECK(config.get_string("app.name") == "FromDisk");
    CHECK(config.path() == file);
}

TEST(config_list_and_conversion_helpers) {
    cfg::Config config;
    config.set_string_list("repositories", {"gentoo", "local"});
    CHECK(config.get_string_list("repositories").size() == 2U);
    CHECK(config.get_string_list("repositories")[0] == "gentoo");
    CHECK(config.get_string_list("missing", {"fallback"}).size() == 1U);

    config.set("port", std::string("not-a-number"));
    CHECK(config.get_int("port", 42) == 42);
    CHECK(!config.get_optional<int>("port").has_value());

    config.set("enabled", true);
    CHECK(config.get_bool("enabled", false));
    config.clear();
    CHECK(!config.contains("enabled"));
}

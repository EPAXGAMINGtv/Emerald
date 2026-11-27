#include "TestFramework.hpp"
#include "TestPaths.hpp"

#include "cfg/Config.hpp"

#include <filesystem>
#include <string>

TEST(config_exhaustive_scalar_roundtrip) {
    for (int index = 0; index < 200; ++index) {
        cfg::Config config;
        const std::string suffix = std::to_string(index);
        const std::string text = "value-" + suffix;
        const double fractional = static_cast<double>(index) / 4.0;

        config.set_default("scalars.count" + suffix, -1);
        config.set("scalars.count" + suffix, index);
        config.set("scalars.ratio" + suffix, fractional);
        config.set("scalars.enabled" + suffix, (index % 2) == 0);
        config.set_string("scalars.text" + suffix, text);

        CHECK(config.get_int("scalars.count" + suffix, -1) == index);
        CHECK_NEAR(config.get_double("scalars.ratio" + suffix, -1.0), fractional);
        CHECK(config.get_bool("scalars.enabled" + suffix, false) == ((index % 2) == 0));
        CHECK(config.get_string("scalars.text" + suffix, "missing") == text);
        CHECK(config.get_optional<int>("scalars.count" + suffix).value() == index);

        CHECK(config.reset("scalars.count" + suffix));
        CHECK(config.contains("scalars.count" + suffix));
        CHECK(config.get_int("scalars.count" + suffix, -2) == -1);
        CHECK(config.remove("scalars.count" + suffix));
        CHECK(!config.contains("scalars.count" + suffix));
    }
}

TEST(config_exhaustive_nested_paths) {
    for (int index = 0; index < 100; ++index) {
        cfg::Config config;
        const std::string leaf = "tree.item" + std::to_string(index) + ".value";
        config.set_default(leaf, 7);
        config.set(leaf, index);

        CHECK(config.contains(leaf));
        CHECK(config.get_int(leaf, -1) == index);
        CHECK(config.reset(leaf));
        CHECK(config.get_int(leaf, -1) == 7);
        CHECK(config.remove(leaf));
        CHECK(!config.contains(leaf));
    }
}

TEST(config_exhaustive_validation) {
    for (int index = 0; index < 100; ++index) {
        cfg::Config config;
        config.define<int>("limits.count", 50, cfg::range(0, 100));
        config.set("limits.count", index);
        CHECK(config.validate().empty());

        config.set("limits.count", index + 101);
        CHECK(config.validate().size() == 1U);
    }
}

TEST(config_exhaustive_callbacks) {
    for (int index = 0; index < 100; ++index) {
        cfg::Config config;
        config.set_default("audio.volume", 0.0);
        int notifications = 0;
        config.on_change<double>("audio.volume", [&notifications](double) noexcept {
            ++notifications;
        });

        config.set("audio.volume", static_cast<double>(index) / 10.0);
        CHECK(notifications == 1);
        CHECK_NEAR(config.get_double("audio.volume", -1.0), static_cast<double>(index) / 10.0);
    }
}

TEST(config_exhaustive_save_roundtrip) {
    test::TempDir dir("exhaustive_roundtrip");
    for (int index = 0; index < 20; ++index) {
        const std::filesystem::path file =
            dir.path() / ("config-" + std::to_string(index) + ".yaml");
        const std::string name = "project-" + std::to_string(index);

        cfg::Config output(file);
        output.set_default("app.name", std::string("Emerald"));
        output.set_string("app.name", name);
        output.set("app.index", index);
        CHECK(output.save(cfg::SaveMode::Effective));

        cfg::Config input(file);
        input.set_default("app.name", std::string("Emerald"));
        CHECK(input.load());
        CHECK(input.get_string("app.name", "missing") == name);
        CHECK(input.get_int("app.index", -1) == index);
    }
}

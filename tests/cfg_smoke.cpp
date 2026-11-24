#include "cfg/Config.hpp"
#include "cfg/ConfigManager.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "cfg_smoke: FAIL: " << message << '\n';
        ++failures;
    }
}

std::filesystem::path make_temp_dir() {
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path() / "emerald_cfg_smoke";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace

int main() {
    const std::filesystem::path dir = make_temp_dir();
    const std::filesystem::path file = dir / "settings.yaml";

    cfg::Config config(file);
    config.define<std::string>("app.name", "Emerald", cfg::non_empty());
    config.define<bool>("gui.enabled", true);
    config.define<double>("audio.volume", 35.0, cfg::range(0.0, 100.0));

    check(config.get_string("app.name") == "Emerald", "default string");
    check(config.get_bool("gui.enabled"), "default bool");
    check(config.get_double("audio.volume") == 35.0, "default double");

    int changes = 0;
    config.on_change<double>("audio.volume", [&changes](double) {
        ++changes;
    });
    config.set("audio.volume", 42.0);
    check(config.get_double("audio.volume") == 42.0, "explicit override");
    check(changes == 1, "change callback after set");

    config.reset("audio.volume");
    check(config.get_double("audio.volume") == 35.0, "reset restores default");
    check(changes == 2, "change callback after reset");

    config.set_string("logging.level", "warn");
    check(config.validate().empty(), "valid config has no issues");
    config.set("audio.volume", 200.0);
    check(config.validate().size() == 1U, "range validator reports issue");
    config.reset("audio.volume");

    check(config.save(), "atomic save succeeds");
    check(!std::filesystem::exists(file.string() + ".tmp"), "no temp file left");

    cfg::Config loaded(file);
    loaded.define<std::string>("app.name", "Emerald");
    loaded.define<bool>("gui.enabled", true);
    loaded.define<double>("audio.volume", 35.0);
    check(loaded.load(), "load saved file");
    check(loaded.get_string("app.name") == "Emerald", "loaded string");
    check(loaded.get_double("audio.volume") == 35.0, "loaded default value");
    check(loaded.get_string("logging.level") == "warn", "loaded override");

    cfg::Config overrides;
    overrides.set_save_mode(cfg::SaveMode::Overrides);
    overrides.define<std::string>("app.name", "Emerald");
    overrides.define<bool>("gui.enabled", true);
    overrides.set("gui.enabled", false);
    const std::string override_dump = overrides.dump(cfg::SaveMode::Overrides);
    cfg::Config override_view;
    check(override_view.load_from_string(override_dump), "overrides dump parses");
    check(
        !override_view.get_bool("gui.enabled", true),
        "overrides dump contains explicit value"
    );
    check(
        !override_view.contains("app.name"),
        "overrides dump omits untouched default"
    );

    cfg::Config strict(file);
    strict.define<std::string>("app.name", "Emerald");
    check(strict.load(), "load for strict checks");
    const std::string original_name = strict.get_string("app.name");

    const std::filesystem::path bad_file = dir / "bad.yaml";
    {
        std::ofstream bad_out(bad_file, std::ios::out | std::ios::trunc);
        bad_out << "::not: [valid\n";
    }
    check(!strict.load(bad_file), "invalid yaml is rejected");
    check(
        strict.get_string("app.name") == original_name,
        "last valid config survives parse error"
    );
    check(!strict.load(dir / "missing.yaml"), "missing file is rejected");

    strict.set_string_list("repositories", {"gentoo", "local"});
    check(strict.get_string_list("repositories").size() == 2U, "string list set");
    check(
        strict.get_string_list("missing", {"fallback"}).size() == 1U,
        "string list default"
    );

    cfg::ConfigManager manager(dir);
    manager.add("settings", "settings.yaml", [](cfg::Config& c) {
        c.set_default("ui.max_results", 50);
    });
    manager.add("layout", "layout.yaml");
    check(manager.load_all_or_create(), "manager loads all configs");
    check(manager.get("settings").get_int("ui.max_results") == 50, "manager default");
    check(manager.save_all(), "manager saves all configs");
    check(manager.watch("settings", std::chrono::seconds(1)), "watch registers");
    check(!manager.poll(), "poll before interval does nothing");
    check(manager.unwatch("settings"), "unwatch removes registration");

    std::filesystem::remove_all(dir);

    if (failures == 0) {
        std::cout << "cfg_smoke: all checks passed\n";
    }
    return failures == 0 ? 0 : 1;
}

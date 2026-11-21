#include "cfg/Config.hpp"
#include "cfg/Log.hpp"
#include "eemerge/Emerge.hpp"
#include "emrldgui/Gui.hpp"

#include <spdlog/spdlog.h>

int main() {
    cfg::log::Options log_options;
    log_options.logger_name = "emerald";
    log_options.level = cfg::log::Level::Info;
    log_options.console = true;
    log_options.color = true;
    log_options.file_path = "emerald.log";
    cfg::log::init(log_options);

    spdlog::info("Emerald starting");

    cfg::Config config("emerald.yaml");
    if (!config.load()) {
        // Create a simple default config on first run.
        config.set_string("app.name", "Emerald");
        config.set_string("logging.level", "info");
        config.set("gui.enabled", true);
        config.save();
        spdlog::info("Default config created at {}", config.path().string());
    } else {
        spdlog::info("Config loaded: app.name={}", config.get_string("app.name", "Emerald"));
    }

    eemerge::Emerge emerge;
    emerge.sync();

    emrldgui::Gui gui;
    if (gui.init()) {
        gui.run();
        gui.shutdown();
    }

    spdlog::info("Emerald exiting");
    cfg::log::shutdown();
    return 0;
}

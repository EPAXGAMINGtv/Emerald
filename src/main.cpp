#include "cfg/Config.hpp"
#include "cfg/Log.hpp"
#include "eemerge/Emerge.h"
#include "emrldgui/Gui.hpp"

#include <spdlog/spdlog.h>

int main() {
    cfg::log::Options log_options;
    log_options.logger_name = "emerald";
    log_options.level = cfg::log::Level::Info;
    log_options.console = true;
    log_options.color = true;
    cfg::log::init(log_options);

    spdlog::info("Emerald starting");

    cfg::Config config("emerald.yaml");
    config.set_string("app.name", "Emerald");
    config.set_string("logging.level", "info");
    config.set("gui.enabled", true);

    if (config.load_or_create()) {
        spdlog::info("Config ready: {} (app.name={})", config.path().string(),
                     config.get_string("app.name", "Emerald"));
    } else {
        spdlog::error("Config could not be loaded or created: {}", config.path().string());
    }

    emgr::Emerge emerge;
    emerge.EmergeSync();

    emrldgui::Gui gui;
    if (gui.init()) {
        gui.run();
        gui.shutdown();
    }

    spdlog::info("Emerald exiting");
    cfg::log::shutdown();
    return 0;
}

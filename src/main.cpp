#include <unistd.h>

#include "cfg/Config.hpp"
#include "eemerge/Emerge.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <X11/Xproto.h>

#include "emrldgui/EmeraldGUI.h"
#include "emrldgui/Window.h"

int main() {
    auto logger = spdlog::stdout_color_mt("emerald");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

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

    emrldgui::EmeraldGUI emeraldGUI;
    emgui::Window window= emgui::Window("test",1920,1080,static_cast<SDL_WindowFlags>(SDL_WINDOW_MAXIMIZED|SDL_WINDOW_RESIZABLE|SDL_WINDOW_SHOWN));

    while (!window.shouldClose()) {
        window.updateWindow();
    }

    window.~Window();
    spdlog::info("Emerald exiting");
    spdlog::shutdown();
    return 0;
}

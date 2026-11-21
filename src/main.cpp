//
// Created by epax on 21.11.26.
//

//
// small test for the per window event system
//

#include <unistd.h>

#include "cfg/Config.hpp"
#include "eemerge/Emerge.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <X11/Xproto.h>

#include "emrldgui/EmeraldGUI.h"
#include "emrldgui/Window.h"
#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/input/Keyboard.h"
#include "emrldgui/input/Mouse.h"



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

    emgui::Keyboard keyboard(window);

    //Mouse uses the EventMouse that is registered by default ->
    emgui::Mouse mouse(window);



    while (!window.shouldClose()) {
        window.updateWindow();

        if (keyboard.isPressed(SDLK_ESCAPE)) {
            spdlog::info("Escape pressed, closing window");
            window.closeWindow();
        }

        if (mouse.isButtonPressed(SDL_BUTTON_LEFT)) {
            spdlog::info("Left mouse button pressed at {} {}", mouse.getX(), mouse.getY());
        }

        spdlog::info("Mouse pos x:{} and y:{}", mouse.getX(), mouse.getY());
        keyboard.clearFrameState();
        mouse.clearFrameState();
    }

    spdlog::info("Emerald exiting");
    spdlog::shutdown();
    return 0;
}

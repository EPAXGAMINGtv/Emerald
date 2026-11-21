#include "emrldgui/Gui.hpp"

#include <SDL.h>
#include <spdlog/spdlog.h>

namespace emrldgui {

bool Gui::init() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        spdlog::error("SDL_Init failed: {}", SDL_GetError());
        return false;
    }

    initialized_ = true;
    spdlog::info("emrldgui initialized (SDL video)");
    return true;
}

void Gui::run() {
    if (!initialized_) {
        spdlog::warn("emrldgui::run called before init");
        return;
    }

    spdlog::info("emrldgui run loop stub (GUI not implemented yet)");
}

void Gui::shutdown() {
    if (!initialized_) {
        return;
    }

    SDL_Quit();
    initialized_ = false;
    spdlog::info("emrldgui shutdown complete");
}

} // namespace emrldgui

//
// Created by epax on 21.11.26.
//

#include "../include/emrldgui/EmeraldGUI.h"

#include <SDL.h>

#include "spdlog/spdlog.h"

namespace emrldgui {
    EmeraldGUI::EmeraldGUI() {
        spdlog::info("Initalizing SDL2!");
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }

    EmeraldGUI::~EmeraldGUI() {

    }
} // emrldgui
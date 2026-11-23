// Created by epax on 21.11.26.

#include "emrldgui/EmeraldGUI.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "emrldgui/Window.h"
#include "spdlog/spdlog.h"

namespace emrldgui {

    namespace {
        EmeraldGUI*& activeInstance() {
            static EmeraldGUI* instance = nullptr;
            return instance;
        }
    } // namespace

    EmeraldGUI::EmeraldGUI() {
        // Make SDL use its Wayland backend on compositors like niri. Without
        // this, a stale DISPLAY can make SDL try X11 first and never map a
        // window on the Wayland session.
        const char* sessionType = std::getenv("XDG_SESSION_TYPE");
        if (std::getenv("WAYLAND_DISPLAY") != nullptr ||
            (sessionType != nullptr && std::string(sessionType) == "wayland")) {
            SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland");
        }

        if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
            const std::string error = SDL_GetError();
            spdlog::error("SDL_InitSubSystem(SDL_INIT_VIDEO) failed: {}", error);
            throw std::runtime_error("SDL_InitSubSystem(SDL_INIT_VIDEO) failed: " + error);
        }
        videoInitialized_ = true;

        if (TTF_Init() != 0) {
            const std::string error = TTF_GetError();
            spdlog::error("TTF_Init failed: {}", error);
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            videoInitialized_ = false;
            throw std::runtime_error("TTF_Init failed: " + error);
        }
        ttfInitialized_ = true;

        activeInstance() = this;
        emgui::Window::resetGlobalQuit();

        spdlog::info("Initialized SDL2 and SDL_ttf");
    }

    EmeraldGUI::~EmeraldGUI() {
        if (activeInstance() == this) {
            activeInstance() = nullptr;
        }

        // All renderers and SDL windows must be gone before SDL shuts down
        for (emgui::Window* window : windows_) {
            if (window != nullptr) {
                window->shutdown();
            }
        }
        windows_.clear();

        if (ttfInitialized_) {
            TTF_Quit();
            ttfInitialized_ = false;
        }

        if (videoInitialized_) {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            videoInitialized_ = false;
        }

        spdlog::info("Shutdown SDL2 and SDL_ttf");
    }

    EmeraldGUI* EmeraldGUI::active() {
        return activeInstance();
    }

    void EmeraldGUI::registerWindow(emgui::Window* window) {
        if (window == nullptr) {
            return;
        }

        windows_.push_back(window);
    }

    void EmeraldGUI::unregisterWindow(emgui::Window* window) {
        if (window == nullptr) {
            return;
        }

        windows_.erase(
            std::remove(windows_.begin(), windows_.end(), window),
            windows_.end()
        );
    }

} // emrldgui

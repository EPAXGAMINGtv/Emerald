#pragma once

#include <SDL_video.h>

namespace emgui {
    class RenderContext {
    public:
        explicit RenderContext(SDL_Window* window)
            : window_(window) {}

        SDL_Window* getWindow() const {
            return window_;
        }

        int getWindowWidth() const {
            int width = 0;
            int height = 0;
            SDL_GetWindowSize(window_, &width, &height);
            return width;
        }

        int getWindowHeight() const {
            int width = 0;
            int height = 0;
            SDL_GetWindowSize(window_, &width, &height);
            return height;
        }

    private:
        SDL_Window* window_;
    };
} // emgui

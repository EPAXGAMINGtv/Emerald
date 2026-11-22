
#pragma once

#include <SDL_video.h>

class RenderContext {
public:
    explicit RenderContext(SDL_Window* window)
        : window_(window) {}

    SDL_Window* getWindow() const {
        return window_;
    }

    int getWindowWidth() const {
        int width, height;
        SDL_GetWindowSize(window_, &width, &height);
        return width;
    }

    int getWindowHeight() const {
        int width, height;
        SDL_GetWindowSize(window_, &width, &height);
        return height;
    }

private:
    SDL_Window* window_;
};

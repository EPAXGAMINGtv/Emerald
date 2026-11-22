// Created by epax on 21.11.26.

// Ttf font used by RendererOpts::drawText

#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <string>

namespace emgui {
    class Font {
    public:
        explicit Font(int fontSize = 24, std::string path = {});
        ~Font();

        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;

        // Returns an owned surface, caller must SDL_FreeSurface it
        SDL_Surface* renderSurface(
            const std::string& text,
            const SDL_Color& color
        ) const;

        bool measure(const std::string& text, int& width, int& height) const;

        void draw(
            SDL_Renderer* renderer,
            const std::string& text,
            const SDL_Point& position,
            const SDL_Color& color
        ) const;

        bool isValid() const {
            return font_ != nullptr;
        }

    private:
        // Variables
        TTF_Font* font_ = nullptr;
    };
} // emgui

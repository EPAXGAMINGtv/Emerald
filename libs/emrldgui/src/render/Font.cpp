//
// Created by epax on 21.11.26.
//

#include "../../include/emrldgui/render/Font.h"

#include <filesystem>

#include "spdlog/spdlog.h"

namespace emgui {

    namespace {

        //try a project local font first, then common system fonts ->
        std::string findDefaultFontPath() {
            static const char* candidates[] = {
                "assets/fonts/LiberationSans-Regular.ttf",
                "/usr/share/fonts/liberation-fonts/LiberationSans-Regular.ttf",
                "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
            };

            for (const char* candidate : candidates) {
                if (std::filesystem::exists(candidate)) {
                    return candidate;
                }
            }

            return candidates[0];
        }

    } // namespace

    //Constructor and Destructor here ->
    Font::Font(int fontSize, std::string path) {
        if (TTF_WasInit() == 0 && TTF_Init() != 0) {
            spdlog::error("TTF_Init failed: {}", TTF_GetError());
            return;
        }

        if (path.empty()) {
            path = findDefaultFontPath();
        }

        font_ = TTF_OpenFont(path.c_str(), fontSize);

        if (font_ == nullptr) {
            spdlog::error("TTF_OpenFont failed for {}: {}", path, TTF_GetError());
            return;
        }

        spdlog::info("Loaded TTF font from {}", path);
    }

    Font::~Font() {
        if (font_ != nullptr) {
            TTF_CloseFont(font_);
        }
    }

    //some functions here ->
    void Font::draw(
        SDL_Renderer* renderer,
        const std::string& text,
        const SDL_Point& position,
        const SDL_Color& color
    ) const {
        if (renderer == nullptr || font_ == nullptr) {
            return;
        }

        SDL_Surface* surface = TTF_RenderText_Blended(font_, text.c_str(), color);

        if (surface == nullptr) {
            spdlog::error("TTF_RenderText_Blended failed: {}", TTF_GetError());
            return;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

        if (texture == nullptr) {
            spdlog::error("SDL_CreateTextureFromSurface failed: {}", SDL_GetError());
            SDL_FreeSurface(surface);
            return;
        }

        const SDL_Rect destination = {
            position.x,
            position.y,
            surface->w,
            surface->h
        };

        SDL_RenderCopy(renderer, texture, nullptr, &destination);

        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);
    }

} // emgui

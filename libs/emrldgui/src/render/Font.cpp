//
// Created by epax on 21.11.26.
//

#include "emrldgui/render/Font.h"

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
        //TTF lifetime is owned by EmeraldGUI, Font only opens/closes its face ->
        if (TTF_WasInit() == 0) {
            spdlog::warn("TTF is not initialized; create emrldgui::EmeraldGUI before using Font");
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
    SDL_Surface* Font::renderSurface(
        const std::string& text,
        const SDL_Color& color
    ) const {
        if (font_ == nullptr) {
            return nullptr;
        }

        SDL_Surface* surface = TTF_RenderText_Blended(font_, text.c_str(), color);

        if (surface == nullptr) {
            spdlog::error("TTF_RenderText_Blended failed: {}", TTF_GetError());
        }

        return surface;
    }

    bool Font::measure(const std::string& text, int& width, int& height) const {
        width = 0;
        height = 0;

        if (font_ == nullptr) {
            return false;
        }

        if (TTF_SizeText(font_, text.c_str(), &width, &height) != 0) {
            spdlog::error("TTF_SizeText failed: {}", TTF_GetError());
            return false;
        }

        return true;
    }

    void Font::draw(
        SDL_Renderer* renderer,
        const std::string& text,
        const SDL_Point& position,
        const SDL_Color& color
    ) const {
        if (renderer == nullptr) {
            return;
        }

        SDL_Surface* surface = renderSurface(text, color);

        if (surface == nullptr) {
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

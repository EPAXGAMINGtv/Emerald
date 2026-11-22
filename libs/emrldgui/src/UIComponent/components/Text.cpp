//
// Created by epax on 22.11.26.
//

#include "../../../include/emrldgui/ui_component/components/Text.h"

#include "../../../../../cmake-build-release/_deps/spdlog-src/include/spdlog/spdlog.h"

namespace emgui {
    Text::Text(const std::string &text, int x, int y) : text_(text), x_(x), y_(y) {
        //inits a basic coolor
        color_ = SDL_Color(255, 255, 255, 255);
        position_ = SDL_Point{x, y};
        spdlog::info("Created new Text Component with text: {} and Colors: r:{} g:{} b:{}", text_, color_.r, color_.g,
                     color_.b);
    }

    void render(RendererOpts &renderer) {
        renderer.drawText(
            text_,
            position_,
            color_
        );
    }

    void setText(const std::string &text) {
        text_ = text;
    }

    std::string getText() const {
        return text_;
    }

    void setColor(const SDL_Color &color) {
        color_ = color;
    }
} // emgui

//
// Created by epax on 22.11.26.
//

#include "emrldgui/ui_component/components/Text.h"

#include <spdlog/spdlog.h>

namespace emgui {

    Text::Text(const std::string& text, int x, int y)
        : UIComponent(x, y), text_(text) {
        spdlog::info(
            "Created Text component '{}' at {} {}",
            text_, x_, y_
        );
    }

    void Text::render(RendererOpts& renderer) {
        renderer.drawText(text_, getPosition(), color_);
    }

    void Text::setText(const std::string& text) {
        text_ = text;
    }

    std::string Text::getText() const {
        return text_;
    }

    void Text::setColor(const SDL_Color& color) {
        color_ = color;
    }

} // emgui

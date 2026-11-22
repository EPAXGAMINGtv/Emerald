//
// Created by epax on 22.11.26.
//

#include "emrldgui/ui_component/components/Text.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

namespace emgui {

    Text::Text(const std::string& text, int x, int y)
        : UIComponent(x, y), text_(text) {
        spdlog::info(
            "Created Text component '{}' at {} {}",
            text_, x_, y_
        );
    }

    void Text::render(RendererOpts& renderer) {
        const FontSpec fontSpec{fontFamily_, fontSize_, fontStyle_};
        renderer.drawText(text_, getPosition(), color_, fontSpec);
    }

    void Text::applyStyle(const StyleValues& values) {
        color_ = values.getColor("color", color_);
        fontFamily_ = values.getString("font-family", fontFamily_);

        const int size = values.getInt("font-size", fontSize_);

        if (size > 0) {
            fontSize_ = size;
        }

        fontStyle_ = values.getString("font-style", fontStyle_);

        spdlog::debug(
            "Text style: family='{}' size={} style='{}'",
            fontFamily_,
            fontSize_,
            fontStyle_
        );
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

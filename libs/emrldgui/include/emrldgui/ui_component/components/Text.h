//
// Created by epax on 22.11.26.
//

#pragma once

#include <string>

#include "emrldgui/ui_component/UIComponent.h"

namespace emgui {
    class Text : public UIComponent {
    public:
        explicit Text(const std::string& text, int x, int y);

        void render(RendererOpts& renderer) override;

        void setText(const std::string& text);
        std::string getText() const;

        void setColor(const SDL_Color& color);
        SDL_Color getColor() const {
            return color_;
        }

    private:
        std::string text_;
        SDL_Color color_{255, 255, 255, 255};
    };
} // emgui

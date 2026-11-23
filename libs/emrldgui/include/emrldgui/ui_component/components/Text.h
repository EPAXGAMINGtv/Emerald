// Created by epax on 22.11.26.

#pragma once

#include <algorithm>
#include <string>

#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {
    class StyleValues;

    class Text : public UIComponent {
    public:
        explicit Text(const std::string& text, int x, int y);

        std::string getComponentName() const override {
            return "Text";
        }

        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;

        // Approximate natural size so Text can live inside Layout containers
        SDL_Point getLayoutSize() const override {
            const int width = std::max(
                1,
                static_cast<int>(text_.size()) * std::max(1, fontSize_) * 3 / 5 +
                    std::max(0, textStyle_.letterSpacing) *
                        std::max(0, static_cast<int>(text_.size()) - 1)
            );
            return {width, std::max(1, fontSize_ + 8)};
        }

        void setText(const std::string& text);
        std::string getText() const;

        void setColor(const SDL_Color& color);
        SDL_Color getColor() const {
            return color_;
        }

        const std::string& getFontFamily() const {
            return fontFamily_;
        }

        int getFontSize() const {
            return fontSize_;
        }

        const std::string& getFontStyle() const {
            return fontStyle_;
        }

    private:
        std::string text_;
        SDL_Color color_{255, 255, 255, 255};

        std::string fontFamily_ = "LiberationSans";
        int fontSize_ = 24;
        std::string fontStyle_ = "normal";
        TextStyle textStyle_;
    };
} // emgui

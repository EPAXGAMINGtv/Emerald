// Created by epax on 22.11.26.

#pragma once

#include <algorithm>
#include <string>
#include <utility>

#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {

    class StyleValues;

    // Styled rectangle that can be used as a container background
    class Panel : public UIComponent {
    public:
        Panel(int x, int y, int width, int height, std::string title = {});

        std::string getComponentName() const override {
            return "Panel";
        }

        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;

        void setTitle(std::string title) {
            title_ = std::move(title);
        }

        const std::string& getTitle() const {
            return title_;
        }

        void setSize(int width, int height) {
            width_ = width;
            height_ = height;
        }

        int getWidth() const {
            return width_;
        }

        int getHeight() const {
            return height_;
        }

        SDL_Point getContentOrigin() const {
            return {x_ + padding_.left, y_ + padding_.top + titleOffset_};
        }

        SDL_Point getContentSize() const {
            return {
                std::max(0, width_ - padding_.horizontal()),
                std::max(0, height_ - padding_.vertical() - titleOffset_)
            };
        }

    private:
        std::string title_;
        int width_ = 320;
        int height_ = 220;

        SurfaceStyle surface_;
        GeometryStyle geometry_;
        Padding padding_;

        SDL_Color titleColor_{220, 228, 255, 255};

        int titleSpacing_ = 8;
        int titleOffset_ = 0;

        TextStyle titleStyle_;

        FontSpec fontSpec_{"LiberationSans", 20, "bold"};
    };

} // emgui

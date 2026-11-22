//
// Created by epax on 22.11.26.
//

#pragma once

#include <algorithm>
#include <string>
#include <utility>

#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {

    class StyleValues;

    //styled rectangle that can be used as a container background
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
            return {x_ + padding_, y_ + padding_ + titleOffset_};
        }

        SDL_Point getContentSize() const {
            return {
                std::max(0, width_ - 2 * padding_),
                std::max(0, height_ - 2 * padding_ - titleOffset_)
            };
        }

    private:
        std::string title_;
        int width_ = 320;
        int height_ = 220;

        SDL_Color backgroundColor_{30, 30, 46, 235};
        SDL_Color gradientTopColor_{};
        SDL_Color gradientBottomColor_{};
        bool useGradient_ = false;
        SDL_Color borderColor_{70, 74, 96, 255};
        SDL_Color titleColor_{220, 228, 255, 255};

        int borderWidth_ = 1;
        int borderRadius_ = 12;
        int padding_ = 14;
        int titleSpacing_ = 8;
        int titleOffset_ = 0;

        ShadowStyle shadow_;

        FontSpec fontSpec_{"LiberationSans", 20, "bold"};
    };

} // emgui

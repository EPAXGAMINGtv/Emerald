//
// Created by epax on 22.11.26.
//

#pragma once

#include "emrldgui/render/RendererOpts.h"

namespace emgui {
    class UIComponent {
    public:
        UIComponent() = default;
        UIComponent(int x, int y) : x_(x), y_(y) {}

        virtual ~UIComponent() = default;

        virtual void render(RendererOpts& renderer) = 0;
        virtual void update() {}

        void setPosition(int x, int y) {
            x_ = x;
            y_ = y;
        }

        SDL_Point getPosition() const {
            return SDL_Point{x_, y_};
        }

    protected:
        int x_ = 0;
        int y_ = 0;
    };
} // emgui

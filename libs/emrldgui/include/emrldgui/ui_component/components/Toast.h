// Created by epax on 24.11.26.

#pragma once

#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace emgui {

    class StyleValues;

    // Non-interactive notification stack drawn entirely in the overlay pass.
    class ToastManager : public UIComponent {
    public:
        // x/y are the anchor corner; width is the toast width. A positive
        // height caps how much of the stack is visible at once.
        ToastManager(int x = 0, int y = 0, int width = 360, int height = 0);

        ToastManager(const ToastManager&) = delete;
        ToastManager& operator=(const ToastManager&) = delete;
        ToastManager(ToastManager&&) = delete;
        ToastManager& operator=(ToastManager&&) = delete;

        std::string getComponentName() const override {
            return "ToastManager";
        }

        void render(RendererOpts& renderer) override;
        void renderOverlay(RendererOpts& renderer) override;
        void update() override;
        void applyStyle(const StyleValues& values) override;

        void show(std::string message, Uint32 durationMs = 3000);
        void show(std::string title, std::string message, Uint32 durationMs = 3000);
        void clear();
        std::size_t getCount() const {
            return toasts_.size();
        }

    private:
        struct Toast {
            std::string title;
            std::string message;
            Uint32 durationMs = 0;
            Uint32 startTicks = 0;
        };

        SDL_Rect getViewportRect(RendererOpts& renderer) const;

        std::vector<Toast> toasts_;

        int width_ = 360;
        int height_ = 0;
        int borderWidth_ = 1;
        int borderRadius_ = 8;
        int gap_ = 10;
        int maxLines_ = 4;
        int progressHeight_ = 4;
        Uint32 defaultDurationMs_ = 3000;
        bool bottomUp_ = false;

        Padding padding_;
        ShadowStyle shadow_;
        FontSpec fontSpec_{"LiberationSans", 15, "normal"};
        FontSpec titleFontSpec_{"LiberationSans", 17, "bold"};

        SDL_Color backgroundColor_{255, 255, 255, 255};
        SDL_Color borderColor_{223, 223, 223, 255};
        SDL_Color titleColor_{36, 36, 36, 255};
        SDL_Color textColor_{36, 36, 36, 255};
        SDL_Color progressColor_{53, 132, 228, 255};
        float opacity_ = 1.0f;
    };

} // emgui

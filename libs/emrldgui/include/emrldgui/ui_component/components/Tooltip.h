// Created by epax on 24.11.26.

#pragma once

#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventManager.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

#include <string>
#include <utility>

namespace emgui {

    class StyleValues;

    // CSS-timed hover card; drawn in the overlay pass so it stays above widgets
    class Tooltip : public UIComponent {
    public:
        explicit Tooltip(std::string text = {});
        ~Tooltip() override;

        Tooltip(const Tooltip&) = delete;
        Tooltip& operator=(const Tooltip&) = delete;
        Tooltip(Tooltip&&) = delete;
        Tooltip& operator=(Tooltip&&) = delete;

        std::string getComponentName() const override {
            return "Tooltip";
        }

        void render(RendererOpts& renderer) override;
        void renderOverlay(RendererOpts& renderer) override;
        void update() override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        void setText(std::string text) {
            text_ = std::move(text);
        }

        const std::string& getText() const {
            return text_;
        }

        // Explicit popups: text/top-left position, or position only
        void show(std::string text, int x, int y);
        void showAt(int x, int y);
        void hide();

        // Polls the anchor every update; opens while it is hovered
        void attachTo(UIComponent* anchor) {
            anchor_ = anchor;
            shown_ = false;
            anchorShown_ = false;
            anchorExpired_ = false;
            setVisible(true);
        }

        UIComponent* getAnchor() const {
            return anchor_;
        }

    private:
        void onMouseEvent(EventMouse& event);
        void onKeyboardEvent(EventKeyboard& event);
        void beginShow();
        static bool containsPoint(const SDL_Rect& rect, int x, int y);
        SDL_Rect getCardRect(RendererOpts& renderer) const;

        EventManager::ListenerId mouseListener_ = EventManager::kInvalidListener;
        EventManager::ListenerId keyboardListener_ = EventManager::kInvalidListener;

        std::string text_;
        UIComponent* anchor_ = nullptr;
        Uint32 lastShowTick_ = 0;

        bool shown_ = false;
        bool anchorShown_ = false;
        bool anchorExpired_ = false;

        SDL_Color backgroundColor_{255, 255, 255, 255};
        SDL_Color borderColor_{223, 223, 223, 255};
        SDL_Color textColor_{36, 36, 36, 255};

        int borderWidth_ = 1;
        int borderRadius_ = 8;
        int delayMs_ = 350;
        int displayMs_ = 3200;
        int fadeMs_ = 140;
        int offsetX_ = 0;
        int offsetY_ = 10;
        int maxWidth_ = 320;

        Padding padding_;
        ShadowStyle shadow_;
        FontSpec fontSpec_{"LiberationSans", 15, "normal"};
    };

} // emgui

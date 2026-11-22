//
// Created by epax on 22.11.26.
//

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "emrldgui/Window.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {

    class StyleValues;

    //clickable component with hover/press animation, actions and CSS styling
    class Button : public UIComponent {
    public:
        using Action = std::function<void(Button&)>;

        enum class ButtonType {
            Normal,
            Primary,
            Secondary,
            Success,
            Warning,
            Danger,
            Ghost
        };

        Button(
            std::string text,
            int x,
            int y,
            int width = 180,
            int height = 52
        );
        ~Button() override;

        //the mouse listener captures this, so copies and moves would dangle
        Button(const Button&) = delete;
        Button& operator=(const Button&) = delete;
        Button(Button&&) = delete;
        Button& operator=(Button&&) = delete;

        std::string getComponentName() const override {
            return "Button";
        }

        void update() override;
        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        //multiple actions can be registered for every button state
        void onClick(Action action);
        void onPressed(Action action);
        void onReleased(Action action);

        void setButtonType(ButtonType type);
        ButtonType getButtonType() const {
            return type_;
        }

        void setText(std::string text) {
            text_ = std::move(text);
        }

        const std::string& getText() const {
            return text_;
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

        void setEnabled(bool enabled) {
            enabled_ = enabled;

            if (!enabled_) {
                pressed_ = false;
                hovered_ = false;
            }
        }

        bool isEnabled() const {
            return enabled_;
        }

        bool isHovered() const {
            return hovered_;
        }

        bool isPressed() const {
            return pressed_;
        }

        //useful for tests/debugging of the CSS-driven style
        float getPressProgress() const {
            return pressProgress_;
        }

        const SDL_Color& getBackgroundColor() const {
            return backgroundColor_;
        }

        const SDL_Color& getTextColor() const {
            return textColor_;
        }

        const std::string& getFontFamily() const {
            return fontSpec_.family;
        }

        int getFontSize() const {
            return fontSpec_.size;
        }

    private:
        void onMouseEvent(EventMouse& event);
        bool containsPoint(int x, int y) const;
        void runActions(const std::vector<Action>& actions);

        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;

        std::string text_;
        ButtonType type_ = ButtonType::Normal;
        int width_ = 180;
        int height_ = 52;

        bool enabled_ = true;
        bool hovered_ = false;
        bool pressed_ = false;

        SDL_Color backgroundColor_{45, 116, 218, 255};
        SDL_Color gradientTopColor_{};
        SDL_Color gradientBottomColor_{};
        bool useGradient_ = false;
        SDL_Color backgroundHoverColor_{63, 140, 245, 255};
        SDL_Color backgroundPressedColor_{27, 79, 156, 255};
        SDL_Color disabledBackgroundColor_{90, 90, 90, 255};

        SDL_Color borderColor_{27, 79, 156, 255};
        SDL_Color borderHoverColor_{120, 190, 255, 255};
        SDL_Color borderPressedColor_{20, 60, 120, 255};
        int borderWidth_ = 2;
        int borderRadius_ = 10;

        SDL_Color textColor_{255, 255, 255, 255};
        SDL_Color textHoverColor_{255, 255, 255, 255};
        SDL_Color textPressedColor_{235, 245, 255, 255};
        SDL_Color textDisabledColor_{190, 190, 190, 255};

        ShadowStyle shadow_;
        FocusRingStyle focusRing_;

        FontSpec fontSpec_{"LiberationSans", 20, "bold"};
        int padding_ = 10;
        std::string textAlign_ = "center";

        int pressOffset_ = 2;
        int pressDurationMs_ = 90;
        int releaseDurationMs_ = 150;

        float pressProgress_ = 0.0f;
        float hoverProgress_ = 0.0f;
        Uint32 lastUpdateTicks_ = 0;

        std::vector<Action> clickActions_;
        std::vector<Action> pressActions_;
        std::vector<Action> releaseActions_;
    };

} // emgui

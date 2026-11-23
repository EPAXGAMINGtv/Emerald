// Created by epax on 22.11.26.

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

    // Clickable component with hover/press animation, actions and CSS styling
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

        // The mouse listener captures this, so copies and moves would dangle
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

        SDL_Point getLayoutSize() const override;
        void setLayoutRect(const SDL_Rect& rect) override;

        // Multiple actions can be registered for every button state
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
            syncCssState();
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

        // Useful for tests/debugging of the CSS-driven style
        float getPressProgress() const {
            return pressProgress_;
        }

        const SDL_Color& getBackgroundColor() const {
            return surface_.color;
        }

        const SDL_Color& getTextColor() const {
            return textColor_;
        }

        int getRenderedWidth() const {
            return computedWidth_;
        }

        int getRenderedHeight() const {
            return computedHeight_;
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
        void syncCssState();

        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;

        std::string text_;
        ButtonType type_ = ButtonType::Normal;
        int width_ = 180;
        int height_ = 52;

        bool enabled_ = true;
        bool hovered_ = false;
        bool pressed_ = false;

        SurfaceStyle surface_;
        GeometryStyle geometry_;
        Padding padding_;

        std::vector<SDL_Color> hoverGradient_;
        std::vector<SDL_Color> pressedGradient_;
        std::vector<SDL_Color> disabledGradient_;
        SDL_Color backgroundHoverColor_{63, 140, 245, 255};
        SDL_Color backgroundPressedColor_{27, 79, 156, 255};
        SDL_Color disabledBackgroundColor_{90, 90, 90, 255};

        SDL_Color borderHoverColor_{120, 190, 255, 255};
        SDL_Color borderPressedColor_{20, 60, 120, 255};

        SDL_Color textColor_{255, 255, 255, 255};
        SDL_Color textHoverColor_{255, 255, 255, 255};
        SDL_Color textPressedColor_{235, 245, 255, 255};
        SDL_Color textDisabledColor_{190, 190, 190, 255};

        TextStyle textStyle_;

        FontSpec fontSpec_{"LiberationSans", 20, "bold"};
        std::string textAlign_ = "center";
        int computedWidth_ = 180;
        int computedHeight_ = 52;

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

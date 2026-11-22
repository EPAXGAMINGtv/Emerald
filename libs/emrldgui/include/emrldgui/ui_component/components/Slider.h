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

    //horizontal slider with drag handling, snapping and CSS styling
    class Slider : public UIComponent {
    public:
        using ValueAction = std::function<void(Slider&)>;

        Slider(
            float minimum,
            float maximum,
            float value,
            int x,
            int y,
            int width = 260,
            int height = 56
        );
        ~Slider() override;

        Slider(const Slider&) = delete;
        Slider& operator=(const Slider&) = delete;
        Slider(Slider&&) = delete;
        Slider& operator=(Slider&&) = delete;

        std::string getComponentName() const override {
            return "Slider";
        }

        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        void onValueChanged(ValueAction action);

        void setRange(float minimum, float maximum);
        void setValue(float value);
        float getValue() const {
            return value_;
        }

        float getMinimum() const {
            return minimum_;
        }

        float getMaximum() const {
            return maximum_;
        }

        float getNormalizedValue() const;

        void setStep(float step) {
            step_ = step > 0.0f ? step : 0.0f;
        }

        float getStep() const {
            return step_;
        }

        void setEnabled(bool enabled) {
            enabled_ = enabled;
            if (!enabled_) {
                dragging_ = false;
            }
        }

        bool isEnabled() const {
            return enabled_;
        }

    private:
        void onMouseEvent(EventMouse& event);
        void updateValueFromMouseX(int mouseX);
        void notifyValueChanged();
        void runActions(const std::vector<ValueAction>& actions);
        bool containsPoint(int x, int y) const;
        std::string formatValue() const;

        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;

        float minimum_ = 0.0f;
        float maximum_ = 100.0f;
        float value_ = 0.0f;
        float step_ = 1.0f;

        int width_ = 260;
        int height_ = 56;
        bool enabled_ = true;
        bool hovered_ = false;
        bool dragging_ = false;

        SDL_Color backgroundColor_{0, 0, 0, 0};
        SDL_Color trackColor_{70, 74, 92, 255};
        SDL_Color trackHoverColor_{90, 96, 118, 255};
        SDL_Color trackFilledColor_{45, 116, 218, 255};
        SDL_Color trackFilledHoverColor_{63, 140, 245, 255};
        SDL_Color handleColor_{240, 244, 255, 255};
        SDL_Color handleHoverColor_{255, 255, 255, 255};
        SDL_Color handleBorderColor_{27, 79, 156, 255};
        SDL_Color disabledColor_{110, 110, 110, 255};
        SDL_Color valueColor_{225, 232, 255, 255};

        int backgroundRadius_ = 10;
        int trackHeight_ = 8;
        int handleSize_ = 22;
        int handleBorderWidth_ = 2;
        int handleRadius_ = 8;
        int padding_ = 8;
        bool showValue_ = true;
        int valueOffset_ = 6;

        ShadowStyle shadow_;
        FocusRingStyle focusRing_;

        FontSpec fontSpec_{"LiberationSans", 16, "normal"};
        std::vector<ValueAction> valueActions_;
    };

} // emgui

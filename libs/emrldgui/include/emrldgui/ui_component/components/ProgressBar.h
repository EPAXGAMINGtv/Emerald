//
// Created by epax on 22.11.26.
//

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {

    class StyleValues;

    //animated determinate/indeterminate progress bar with optional label
    class ProgressBar : public UIComponent {
    public:
        using ValueAction = std::function<void(ProgressBar&)>;

        ProgressBar(
            float minimum,
            float maximum,
            float value,
            int x,
            int y,
            int width = 320,
            int height = 32
        );

        std::string getComponentName() const override {
            return "ProgressBar";
        }

        void update() override;
        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;

        void onValueChanged(ValueAction action);

        void setRange(float minimum, float maximum);
        void setValue(float value);
        float getValue() const {
            return value_;
        }
        float getDisplayedValue() const {
            return displayedValue_;
        }
        float getMinimum() const {
            return minimum_;
        }
        float getMaximum() const {
            return maximum_;
        }
        float getNormalizedValue() const;

        void setIndeterminate(bool indeterminate) {
            indeterminate_ = indeterminate;
        }
        bool isIndeterminate() const {
            return indeterminate_;
        }

        void setValueLabel(std::string label) {
            valueLabel_ = std::move(label);
        }
        const std::string& getValueLabel() const {
            return valueLabel_;
        }

    private:
        void notifyValueChanged();
        void runActions(const std::vector<ValueAction>& actions);
        std::string formatValue() const;

        float minimum_ = 0.0f;
        float maximum_ = 100.0f;
        float value_ = 0.0f;
        float displayedValue_ = 0.0f;
        bool indeterminate_ = false;
        float phase_ = 0.0f;

        int width_ = 320;
        int height_ = 32;
        Uint32 lastUpdateTicks_ = 0;

        SDL_Color trackColor_{38, 42, 60, 255};
        SDL_Color fillColor_{45, 116, 218, 255};
        SDL_Color fillColor2_{110, 168, 254, 255};
        SDL_Color shimmerColor_{255, 255, 255, 70};
        SDL_Color textColor_{235, 242, 255, 255};
        int trackHeight_ = 14;
        int radius_ = 8;
        int padding_ = 2;
        bool showValue_ = true;
        std::string valueFormat_ = "percent";
        std::string valueLabel_;
        float animationSpeed_ = 8.0f;

        ShadowStyle shadow_;
        FontSpec fontSpec_{"LiberationSans", 14, "bold"};

        std::vector<ValueAction> valueActions_;
    };

} // emgui

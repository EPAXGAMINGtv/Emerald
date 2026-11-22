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

    //clickable checkbox with label, hover colors and checked CSS state
    class CheckBox : public UIComponent {
    public:
        using CheckedAction = std::function<void(CheckBox&)>;

        CheckBox(
            std::string label,
            bool checked,
            int x,
            int y
        );
        ~CheckBox() override;

        CheckBox(const CheckBox&) = delete;
        CheckBox& operator=(const CheckBox&) = delete;
        CheckBox(CheckBox&&) = delete;
        CheckBox& operator=(CheckBox&&) = delete;

        std::string getComponentName() const override {
            return "CheckBox";
        }

        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        void onCheckedChanged(CheckedAction action);

        void setChecked(bool checked);
        bool isChecked() const {
            return checked_;
        }

        void toggle() {
            setChecked(!checked_);
        }

        void setLabel(std::string label) {
            label_ = std::move(label);
        }

        const std::string& getLabel() const {
            return label_;
        }

        void setEnabled(bool enabled) {
            enabled_ = enabled;
        }

        bool isEnabled() const {
            return enabled_;
        }

    private:
        void onMouseEvent(EventMouse& event);
        void runCheckedActions();
        bool containsPoint(int x, int y) const;

        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;

        std::string label_;
        bool checked_ = false;
        bool enabled_ = true;
        bool hovered_ = false;
        int contentWidth_ = 0;
        int contentHeight_ = 0;

        int boxSize_ = 24;
        int borderWidth_ = 2;
        int borderRadius_ = 5;
        int spacing_ = 10;

        SDL_Color boxColor_{45, 116, 218, 255};
        SDL_Color boxHoverColor_{63, 140, 245, 255};
        SDL_Color boxCheckedColor_{31, 157, 85, 255};
        SDL_Color boxCheckedHoverColor_{39, 184, 102, 255};
        SDL_Color disabledBoxColor_{90, 90, 90, 255};

        SDL_Color borderColor_{27, 79, 156, 255};
        SDL_Color borderHoverColor_{120, 190, 255, 255};
        SDL_Color borderCheckedColor_{19, 118, 62, 255};
        SDL_Color checkmarkColor_{255, 255, 255, 255};

        SDL_Color textColor_{235, 240, 255, 255};
        SDL_Color textHoverColor_{255, 255, 255, 255};
        SDL_Color disabledTextColor_{180, 180, 180, 255};

        ShadowStyle shadow_;
        FocusRingStyle focusRing_;

        FontSpec fontSpec_{"LiberationSans", 20, "normal"};
        std::vector<CheckedAction> checkedActions_;
    };

} // emgui

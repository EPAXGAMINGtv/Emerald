//
// Created by epax on 22.11.26.
//

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "emrldgui/Window.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {

    class StyleValues;

    //radio button that unchecks every other member of its named group
    class RadioButton : public UIComponent {
    public:
        struct Group;

        using CheckedAction = std::function<void(RadioButton&)>;

        RadioButton(
            std::string label,
            std::string group,
            bool checked,
            int x,
            int y
        );
        ~RadioButton() override;

        RadioButton(const RadioButton&) = delete;
        RadioButton& operator=(const RadioButton&) = delete;
        RadioButton(RadioButton&&) = delete;
        RadioButton& operator=(RadioButton&&) = delete;

        std::string getComponentName() const override {
            return "RadioButton";
        }

        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        void onCheckedChanged(CheckedAction action);

        void setGroup(std::string group);
        const std::string& getGroup() const {
            return groupName_;
        }

        void setChecked(bool checked);
        bool isChecked() const {
            return checked_;
        }

        void setLabel(std::string label) {
            label_ = std::move(label);
        }

        const std::string& getLabel() const {
            return label_;
        }

        void setEnabled(bool enabled) {
            enabled_ = enabled;

            if (!enabled_) {
                hovered_ = false;
            }
        }

        bool isEnabled() const {
            return enabled_;
        }

    private:
        void onMouseEvent(EventMouse& event);
        void joinGroup();
        void runCheckedActions();
        bool containsPoint(int x, int y) const;

        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;
        std::shared_ptr<Group> group_;

        std::string groupName_;
        std::string label_;
        bool checked_ = false;
        bool enabled_ = true;
        bool hovered_ = false;
        int contentWidth_ = 0;
        int contentHeight_ = 0;

        int radioSize_ = 24;
        int borderWidth_ = 2;
        int spacing_ = 10;

        SDL_Color radioColor_{45, 116, 218, 255};
        SDL_Color radioHoverColor_{63, 140, 245, 255};
        SDL_Color radioCheckedColor_{31, 157, 85, 255};
        SDL_Color radioCheckedHoverColor_{39, 184, 102, 255};
        SDL_Color disabledRadioColor_{90, 90, 90, 255};

        SDL_Color borderColor_{27, 79, 156, 255};
        SDL_Color borderHoverColor_{120, 190, 255, 255};
        SDL_Color borderCheckedColor_{19, 118, 62, 255};
        SDL_Color dotColor_{255, 255, 255, 255};

        SDL_Color textColor_{235, 240, 255, 255};
        SDL_Color textHoverColor_{255, 255, 255, 255};
        SDL_Color disabledTextColor_{180, 180, 180, 255};

        ShadowStyle shadow_;
        FocusRingStyle focusRing_;

        FontSpec fontSpec_{"LiberationSans", 20, "normal"};
        std::vector<CheckedAction> checkedActions_;
    };

} // emgui

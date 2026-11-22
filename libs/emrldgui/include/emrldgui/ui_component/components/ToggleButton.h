// Created by epax on 22.11.26.

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Button.h"

namespace emgui {

    // Button that keeps a checked state and switches its CSS class
    class ToggleButton : public Button {
    public:
        using CheckedAction = std::function<void(ToggleButton&)>;

        ToggleButton(
            std::string text,
            int x,
            int y,
            int width = 180,
            int height = 52
        );

        std::string getComponentName() const override {
            return "ToggleButton";
        }

        void setChecked(bool checked);
        bool isChecked() const {
            return checked_;
        }

        void toggle() {
            setChecked(!checked_);
        }

        void onCheckedChanged(CheckedAction action);

    private:
        void runCheckedActions();

        bool checked_ = false;
        std::vector<CheckedAction> checkedActions_;
    };

} // emgui

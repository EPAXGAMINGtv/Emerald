//
// Created by epax on 22.11.26.
//

#include "emrldgui/ui_component/components/ToggleButton.h"

#include <utility>

#include "spdlog/spdlog.h"

namespace emgui {

    ToggleButton::ToggleButton(
        std::string text,
        int x,
        int y,
        int width,
        int height
    )
        : Button(std::move(text), x, y, width, height) {
        onClick([this](Button&) {
            toggle();
        });
    }

    void ToggleButton::onCheckedChanged(CheckedAction action) {
        if (action) {
            checkedActions_.push_back(std::move(action));
        }
    }

    void ToggleButton::setChecked(bool checked) {
        if (checked_ == checked) {
            return;
        }

        checked_ = checked;
        setStyleClass(checked_ ? "checked" : "");
        runCheckedActions();
    }

    void ToggleButton::runCheckedActions() {
        const auto snapshot = checkedActions_;

        for (const CheckedAction& action : snapshot) {
            try {
                action(*this);
            } catch (const std::exception& error) {
                spdlog::error("ToggleButton action failed: {}", error.what());
            }
        }
    }

} // emgui

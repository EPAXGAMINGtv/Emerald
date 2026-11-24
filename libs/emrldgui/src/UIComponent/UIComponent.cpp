// Created by epax on 23.11.26.

#include "emrldgui/ui_component/UIComponent.h"

#include "emrldgui/style/StyleValues.h"

#include <algorithm>

namespace emgui {

    void UIComponent::applyCommonStyle(const StyleValues& values) {
        flex_ = std::max(0.0f, values.getFloat("flex", flex_));
        alignSelf_ = values.getString("align-self", alignSelf_);
        visible_ = values.getBool("visible", visible_);
    }

    void UIComponent::setFocused(bool value) {
        setState(state_.focused, value);
    }

    void UIComponent::collectFocusable(std::vector<UIComponent*>& out) {
        if (isFocusable() && isVisible() && !isDisabled()) {
            out.push_back(this);
        }
    }

    bool UIComponent::hasState(const std::string& state) const {
        if (state == "hover") {
            return state_.hovered;
        }
        if (state == "active") {
            return state_.active;
        }
        if (state == "focus") {
            return state_.focused;
        }
        if (state == "selected") {
            return state_.selected;
        }
        if (state == "disabled") {
            return state_.disabled;
        }
        return false;
    }

} // emgui

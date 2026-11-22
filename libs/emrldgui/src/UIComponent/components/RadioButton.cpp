//
// Created by epax on 22.11.26.
//

#include "emrldgui/ui_component/components/RadioButton.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <exception>
#include <map>
#include <utility>
#include <vector>

namespace emgui {

    struct RadioButton::Group {
        RadioButton* checked = nullptr;
    };

    namespace {

        std::map<SDL_Window*, std::map<std::string, std::weak_ptr<RadioButton::Group>>>&
        radioGroups() {
            static std::map<
                SDL_Window*,
                std::map<std::string, std::weak_ptr<RadioButton::Group>>
            > groups;
            return groups;
        }

    } // namespace

    RadioButton::RadioButton(
        std::string label,
        std::string group,
        bool checked,
        int x,
        int y
    )
        : UIComponent(x, y),
          groupName_(std::move(group)),
          label_(std::move(label)),
          checked_(checked) {
        setStyleClass(checked_ ? "checked" : "");
    }

    RadioButton::~RadioButton() {
        if (group_ != nullptr && group_->checked == this) {
            group_->checked = nullptr;
        }

        if (listenerId_ == EventManager::kInvalidListener ||
            window_ == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            manager->removeListener(listenerId_);
        }
    }

    void RadioButton::onWindowAttached(Window& window) {
        if (listenerId_ != EventManager::kInvalidListener) {
            return;
        }

        SDL_Window* sdlWindow = window.getWindow();

        if (sdlWindow == nullptr) {
            return;
        }

        listenerId_ = EventManager::registerEvent(
            sdlWindow,
            EventMouse(sdlWindow),
            [this](EventMouse& event) {
                onMouseEvent(event);
            }
        );

        joinGroup();
    }

    void RadioButton::onCheckedChanged(CheckedAction action) {
        if (action) {
            checkedActions_.push_back(std::move(action));
        }
    }

    void RadioButton::joinGroup() {
        group_.reset();

        if (groupName_.empty() || window_ == nullptr ||
            window_->getWindow() == nullptr) {
            return;
        }

        auto& perWindow = radioGroups()[window_->getWindow()];
        auto existing = perWindow.find(groupName_);

        if (existing != perWindow.end()) {
            group_ = existing->second.lock();
        }

        if (group_ == nullptr) {
            group_ = std::make_shared<Group>();
            perWindow[groupName_] = group_;
        }

        if (checked_) {
            if (group_->checked != nullptr && group_->checked != this) {
                group_->checked->setChecked(false);
            }

            group_->checked = this;
        }
    }

    void RadioButton::setGroup(std::string group) {
        if (group_ != nullptr && group_->checked == this) {
            group_->checked = nullptr;
        }

        groupName_ = std::move(group);
        joinGroup();
    }

    void RadioButton::setChecked(bool checked) {
        if (checked_ == checked) {
            return;
        }

        if (checked && group_ != nullptr && group_->checked != nullptr &&
            group_->checked != this) {
            group_->checked->setChecked(false);
        }

        checked_ = checked;

        if (group_ != nullptr) {
            if (checked_) {
                group_->checked = this;
            } else if (group_->checked == this) {
                group_->checked = nullptr;
            }
        }

        setStyleClass(checked_ ? "checked" : "");
        runCheckedActions();
    }

    void RadioButton::applyStyle(const StyleValues& values) {
        radioSize_ = std::max(10, values.getInt("radio-size", radioSize_));
        borderWidth_ = std::max(0, values.getInt("border-width", borderWidth_));
        spacing_ = std::max(0, values.getInt("spacing", spacing_));

        radioColor_ = values.getColor("radio-color", radioColor_);
        radioHoverColor_ = values.getColor("radio-hover-color", radioHoverColor_);
        radioCheckedColor_ = values.getColor(
            "radio-checked-color",
            radioCheckedColor_
        );
        radioCheckedHoverColor_ = values.getColor(
            "radio-checked-hover-color",
            radioCheckedHoverColor_
        );
        disabledRadioColor_ = values.getColor(
            "disabled-radio-color",
            disabledRadioColor_
        );

        borderColor_ = values.getColor("border-color", borderColor_);
        borderHoverColor_ = values.getColor("border-hover-color", borderHoverColor_);
        borderCheckedColor_ = values.getColor(
            "border-checked-color",
            borderCheckedColor_
        );
        dotColor_ = values.getColor("dot-color", dotColor_);

        textColor_ = values.getColor("text-color", textColor_);
        textHoverColor_ = values.getColor("text-hover-color", textHoverColor_);
        disabledTextColor_ = values.getColor(
            "disabled-text-color",
            disabledTextColor_
        );

        shadow_.apply(values);
        focusRing_.apply(values);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);
    }

    void RadioButton::render(RendererOpts& renderer) {
        const SDL_Point textSize = renderer.measureText(label_, fontSpec_);

        contentWidth_ = radioSize_ + spacing_ + textSize.x;
        contentHeight_ = std::max(radioSize_, textSize.y);

        SDL_Color radio = radioColor_;
        SDL_Color border = borderColor_;
        SDL_Color text = textColor_;

        if (checked_) {
            radio = radioCheckedColor_;
            border = borderCheckedColor_;
        }

        if (hovered_ && enabled_) {
            radio = checked_ ? radioCheckedHoverColor_ : radioHoverColor_;
            border = borderHoverColor_;
            text = textHoverColor_;
        }

        if (!enabled_) {
            radio = disabledRadioColor_;
            border = disabledRadioColor_;
            text = disabledTextColor_;
        }

        const SDL_Point center{x_ + radioSize_ / 2, y_ + radioSize_ / 2};
        const int radius = radioSize_ / 2;

        shadow_.draw(
            renderer,
            {x_, y_, radioSize_, radioSize_},
            radius
        );

        renderer.fillCircle(center, radius, radio);

        for (int inset = 0; inset < borderWidth_; ++inset) {
            renderer.drawCircle(center, std::max(1, radius - inset), border);
        }

        focusRing_.draw(
            renderer,
            {x_, y_, radioSize_, radioSize_},
            radius,
            hovered_ && enabled_
        );

        if (checked_) {
            renderer.fillCircle(center, std::max(2, radius / 2), dotColor_);
        }

        if (!label_.empty()) {
            const int labelX = x_ + radioSize_ + spacing_;
            const int labelY = y_ + (contentHeight_ - textSize.y) / 2;
            renderer.drawText(label_, {labelX, labelY}, text, fontSpec_);
        }
    }

    bool RadioButton::containsPoint(int x, int y) const {
        return x >= x_ && x < x_ + contentWidth_ &&
               y >= y_ && y < y_ + contentHeight_;
    }

    void RadioButton::onMouseEvent(EventMouse& event) {
        if (!enabled_) {
            return;
        }

        hovered_ = containsPoint(event.getX(), event.getY());

        if (hovered_ && event.getButton() == SDL_BUTTON_LEFT && event.isPressed()) {
            setChecked(true);
        }
    }

    void RadioButton::runCheckedActions() {
        const auto snapshot = checkedActions_;

        for (const CheckedAction& action : snapshot) {
            try {
                action(*this);
            } catch (const std::exception& error) {
                spdlog::error("RadioButton action failed: {}", error.what());
            }
        }
    }

} // emgui

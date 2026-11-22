//
// Created by epax on 22.11.26.
//

#include "emrldgui/ui_component/components/CheckBox.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace emgui {

    CheckBox::CheckBox(
        std::string label,
        bool checked,
        int x,
        int y
    )
        : UIComponent(x, y),
          label_(std::move(label)),
          checked_(checked) {
        setStyleClass(checked_ ? "checked" : "");
    }

    CheckBox::~CheckBox() {
        if (listenerId_ == EventManager::kInvalidListener ||
            window_ == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            manager->removeListener(listenerId_);
        }
    }

    void CheckBox::onWindowAttached(Window& window) {
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
    }

    void CheckBox::onCheckedChanged(CheckedAction action) {
        if (action) {
            checkedActions_.push_back(std::move(action));
        }
    }

    void CheckBox::setChecked(bool checked) {
        if (checked_ == checked) {
            return;
        }

        checked_ = checked;
        setStyleClass(checked_ ? "checked" : "");
        runCheckedActions();
    }

    void CheckBox::applyStyle(const StyleValues& values) {
        boxSize_ = std::max(8, values.getInt("box-size", boxSize_));
        borderWidth_ = std::max(0, values.getInt("border-width", borderWidth_));
        borderRadius_ = std::max(0, values.getInt("border-radius", borderRadius_));
        spacing_ = std::max(0, values.getInt("spacing", spacing_));

        boxColor_ = values.getColor("box-color", boxColor_);
        boxHoverColor_ = values.getColor("box-hover-color", boxHoverColor_);
        boxCheckedColor_ = values.getColor("box-checked-color", boxCheckedColor_);
        boxCheckedHoverColor_ = values.getColor(
            "box-checked-hover-color",
            boxCheckedHoverColor_
        );
        disabledBoxColor_ = values.getColor("disabled-box-color", disabledBoxColor_);

        borderColor_ = values.getColor("border-color", borderColor_);
        borderHoverColor_ = values.getColor("border-hover-color", borderHoverColor_);
        borderCheckedColor_ = values.getColor(
            "border-checked-color",
            borderCheckedColor_
        );
        checkmarkColor_ = values.getColor("checkmark-color", checkmarkColor_);

        textColor_ = values.getColor("text-color", textColor_);
        textHoverColor_ = values.getColor("text-hover-color", textHoverColor_);
        disabledTextColor_ = values.getColor(
            "disabled-text-color",
            disabledTextColor_
        );

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        shadow_.apply(values);
        focusRing_.apply(values);

        spdlog::debug(
            "CheckBox style: box={} font={} size={}",
            boxSize_,
            fontSpec_.family,
            fontSpec_.size
        );
    }

    void CheckBox::render(RendererOpts& renderer) {
        const SDL_Point textSize = renderer.measureText(label_, fontSpec_);

        contentWidth_ = boxSize_ + spacing_ + textSize.x;
        contentHeight_ = std::max(boxSize_, textSize.y);

        SDL_Color box = boxColor_;
        SDL_Color border = borderColor_;
        SDL_Color text = textColor_;

        if (checked_) {
            box = boxCheckedColor_;
            border = borderCheckedColor_;
        }

        if (hovered_ && enabled_) {
            box = checked_ ? boxCheckedHoverColor_ : boxHoverColor_;
            border = borderHoverColor_;
            text = textHoverColor_;
        }

        if (!enabled_) {
            box = disabledBoxColor_;
            text = disabledTextColor_;
        }

        const SDL_Rect boxRect{x_, y_, boxSize_, boxSize_};

        shadow_.draw(renderer, boxRect, borderRadius_);
        renderer.fillRoundedRect(boxRect, borderRadius_, box);

        for (int inset = 0; inset < borderWidth_; ++inset) {
            const SDL_Rect borderRect{
                boxRect.x + inset,
                boxRect.y + inset,
                boxRect.w - 2 * inset,
                boxRect.h - 2 * inset
            };

            if (borderRect.w <= 0 || borderRect.h <= 0) {
                break;
            }

            renderer.drawRoundedRect(
                borderRect,
                std::max(0, borderRadius_ - inset),
                border
            );
        }

        focusRing_.draw(
            renderer,
            boxRect,
            borderRadius_,
            hovered_ && enabled_
        );

        if (checked_) {
            const int left = x_ + boxSize_ / 4;
            const int middle = x_ + boxSize_ * 2 / 5;
            const int right = x_ + boxSize_ * 3 / 4;
            const int upper = y_ + boxSize_ / 4;
            const int center = y_ + boxSize_ / 2;
            const int lower = y_ + boxSize_ * 3 / 4;

            renderer.drawLine({left, center}, {middle, lower}, checkmarkColor_);
            renderer.drawLine({middle, lower}, {right, upper}, checkmarkColor_);
        }

        if (!label_.empty()) {
            const int labelX = x_ + boxSize_ + spacing_;
            const int labelY = y_ + (contentHeight_ - textSize.y) / 2;
            renderer.drawText(label_, {labelX, labelY}, text, fontSpec_);
        }
    }

    bool CheckBox::containsPoint(int x, int y) const {
        return x >= x_ && x < x_ + contentWidth_ &&
               y >= y_ && y < y_ + contentHeight_;
    }

    void CheckBox::onMouseEvent(EventMouse& event) {
        if (!enabled_) {
            return;
        }

        hovered_ = containsPoint(event.getX(), event.getY());

        if (hovered_ && event.getButton() == SDL_BUTTON_LEFT && event.isPressed()) {
            toggle();
        }
    }

    void CheckBox::runCheckedActions() {
        const auto snapshot = checkedActions_;

        for (const CheckedAction& action : snapshot) {
            try {
                action(*this);
            } catch (const std::exception& error) {
                spdlog::error("CheckBox action failed: {}", error.what());
            }
        }
    }

} // emgui

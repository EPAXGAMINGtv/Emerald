// Created by epax on 22.11.26.

#include "emrldgui/ui_component/components/ComboBox.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace emgui {

    ComboBox::ComboBox(
        std::vector<std::string> items,
        int selectedIndex,
        int x,
        int y,
        int width,
        int height
    )
        : UIComponent(x, y),
          items_(std::move(items)),
          width_(width),
          height_(height) {
        if (!items_.empty()) {
            selectedIndex_ = std::clamp(selectedIndex, 0, static_cast<int>(items_.size()) - 1);
        } else {
            selectedIndex_ = -1;
        }
    }

    ComboBox::~ComboBox() {
        if (listenerId_ == EventManager::kInvalidListener ||
            window_ == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            manager->removeListener(listenerId_);
        }
    }

    void ComboBox::onWindowAttached(Window& window) {
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

    void ComboBox::applyStyle(const StyleValues& values) {
        backgroundColor_ = values.getColor("background-color", backgroundColor_);
        backgroundHoverColor_ = values.getColor(
            "background-hover-color",
            backgroundHoverColor_
        );
        disabledBackgroundColor_ = values.getColor(
            "disabled-background-color",
            disabledBackgroundColor_
        );
        borderColor_ = values.getColor("border-color", borderColor_);
        borderHoverColor_ = values.getColor("border-hover-color", borderHoverColor_);
        textColor_ = values.getColor("text-color", textColor_);
        disabledTextColor_ = values.getColor(
            "disabled-text-color",
            disabledTextColor_
        );
        placeholderColor_ = values.getColor("placeholder-color", placeholderColor_);

        popupBackgroundColor_ = values.getColor(
            "popup-background-color",
            popupBackgroundColor_
        );
        popupBorderColor_ = values.getColor(
            "popup-border-color",
            popupBorderColor_
        );
        itemHoverColor_ = values.getColor("item-hover-color", itemHoverColor_);
        itemHoverTextColor_ = values.getColor(
            "item-hover-text-color",
            itemHoverTextColor_
        );
        itemSelectedColor_ = values.getColor(
            "item-selected-color",
            itemSelectedColor_
        );
        itemTextColor_ = values.getColor("item-text-color", itemTextColor_);
        itemSelectedTextColor_ = values.getColor(
            "item-selected-text-color",
            itemSelectedTextColor_
        );
        arrowColor_ = values.getColor("arrow-color", arrowColor_);

        borderWidth_ = std::max(0, values.getInt("border-width", borderWidth_));
        borderRadius_ = std::max(0, values.getInt("border-radius", borderRadius_));
        padding_ = std::max(0, values.getInt("padding", padding_));
        arrowSize_ = std::max(3, values.getInt("arrow-size", arrowSize_));
        itemHeight_ = std::max(12, values.getInt("item-height", itemHeight_));
        itemPadding_ = std::max(0, values.getInt("item-padding", itemPadding_));
        itemRadius_ = std::max(0, values.getInt("item-radius", itemRadius_));
        maxVisibleItems_ = std::max(
            1,
            values.getInt("max-visible-items", maxVisibleItems_)
        );

        shadow_.apply(values);
        focusRing_.apply(values);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);
    }

    void ComboBox::onSelectionChanged(SelectionAction action) {
        if (action) {
            selectionActions_.push_back(std::move(action));
        }
    }

    void ComboBox::setItems(std::vector<std::string> items) {
        items_ = std::move(items);
        selectedIndex_ = items_.empty()
            ? -1
            : std::clamp(selectedIndex_, 0, static_cast<int>(items_.size()) - 1);
        hoveredIndex_ = -1;
        open_ = false;
    }

    void ComboBox::setSelectedIndex(int index) {
        if (index < 0 || index >= static_cast<int>(items_.size())) {
            index = -1;
        }

        if (selectedIndex_ == index) {
            return;
        }

        selectedIndex_ = index;
        runSelectionActions();
    }

    std::string ComboBox::getSelectedText() const {
        if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(items_.size())) {
            return {};
        }

        return items_[static_cast<std::size_t>(selectedIndex_)];
    }

    void ComboBox::setEnabled(bool enabled) {
        enabled_ = enabled;

        if (!enabled_) {
            open_ = false;
            hoveredBox_ = false;
            hoveredIndex_ = -1;
        }
    }

    void ComboBox::setOpen(bool open) {
        if (!enabled_) {
            return;
        }

        open_ = open;

        if (!open_) {
            hoveredIndex_ = -1;
        }
    }

    bool ComboBox::containsPoint(const SDL_Rect& rect, int x, int y) const {
        return x >= rect.x && x < rect.x + rect.w &&
               y >= rect.y && y < rect.y + rect.h;
    }

    SDL_Rect ComboBox::getBoxRect() const {
        return {x_, y_, width_, height_};
    }

    SDL_Rect ComboBox::getPopupRect() const {
        const int visible = std::min(
            static_cast<int>(items_.size()),
            maxVisibleItems_
        );
        return {
            x_,
            y_ + height_ + 2,
            width_,
            visible * itemHeight_
        };
    }

    int ComboBox::getItemIndexAt(int x, int y) const {
        if (!open_ || items_.empty()) {
            return -1;
        }

        const SDL_Rect popup = getPopupRect();

        if (!containsPoint(popup, x, y)) {
            return -1;
        }

        const int index = (y - popup.y) / itemHeight_;

        if (index < 0 || index >= static_cast<int>(items_.size())) {
            return -1;
        }

        return index;
    }

    void ComboBox::render(RendererOpts& renderer) {
        const SDL_Rect box = getBoxRect();

        SDL_Color background = hoveredBox_ || open_
            ? backgroundHoverColor_
            : backgroundColor_;
        SDL_Color border = hoveredBox_ || open_ ? borderHoverColor_ : borderColor_;
        SDL_Color foreground = textColor_;

        if (!enabled_) {
            background = disabledBackgroundColor_;
            border = borderColor_;
            foreground = disabledTextColor_;
        }

        shadow_.draw(renderer, box, borderRadius_);
        renderer.fillRoundedRect(box, borderRadius_, background);

        for (int inset = 0; inset < borderWidth_; ++inset) {
            const SDL_Rect borderRect{
                box.x + inset,
                box.y + inset,
                box.w - 2 * inset,
                box.h - 2 * inset
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

        const std::string selected = getSelectedText();
        const bool showPlaceholder = selected.empty();
        const std::string& displayed = showPlaceholder ? placeholder_ : selected;

        if (!displayed.empty()) {
            const SDL_Point textSize = renderer.measureText(displayed, fontSpec_);
            renderer.drawText(
                displayed,
                {
                    box.x + padding_,
                    box.y + (box.h - textSize.y) / 2
                },
                showPlaceholder ? placeholderColor_ : foreground,
                fontSpec_
            );
        }

        const int arrowCenterX = box.x + box.w - padding_ - arrowSize_;
        const int arrowCenterY = box.y + box.h / 2;

        if (open_) {
            renderer.drawLine(
                {arrowCenterX - arrowSize_, arrowCenterY + arrowSize_ / 2},
                {arrowCenterX, arrowCenterY - arrowSize_ / 2},
                arrowColor_
            );
            renderer.drawLine(
                {arrowCenterX, arrowCenterY - arrowSize_ / 2},
                {arrowCenterX + arrowSize_, arrowCenterY + arrowSize_ / 2},
                arrowColor_
            );
        } else {
            renderer.drawLine(
                {arrowCenterX - arrowSize_, arrowCenterY - arrowSize_ / 2},
                {arrowCenterX, arrowCenterY + arrowSize_ / 2},
                arrowColor_
            );
            renderer.drawLine(
                {arrowCenterX, arrowCenterY + arrowSize_ / 2},
                {arrowCenterX + arrowSize_, arrowCenterY - arrowSize_ / 2},
                arrowColor_
            );
        }

    }

    void ComboBox::renderOverlay(RendererOpts& renderer) {
        if (!open_ || items_.empty()) {
            return;
        }

        const SDL_Rect popup = getPopupRect();

        shadow_.draw(renderer, popup, itemRadius_);
        renderer.fillRoundedRect(popup, itemRadius_, popupBackgroundColor_);
        renderer.drawRoundedRect(popup, itemRadius_, popupBorderColor_);

        const int visible = std::min(
            static_cast<int>(items_.size()),
            maxVisibleItems_
        );

        for (int index = 0; index < visible; ++index) {
            const SDL_Rect itemRect{
                popup.x + itemPadding_,
                popup.y + index * itemHeight_,
                popup.w - 2 * itemPadding_,
                itemHeight_
            };

            const bool isSelected = index == selectedIndex_;
            const bool isHovered = index == hoveredIndex_;

            // item-hover-color / item-selected-color are backgrounds; text keeps
            // its own CSS color so it stays readable in every theme.
            if (isSelected || isHovered) {
                renderer.fillRoundedRect(
                    itemRect,
                    itemRadius_,
                    isSelected ? itemSelectedColor_ : itemHoverColor_
                );
            }

            SDL_Color itemText = itemTextColor_;
            if (isSelected) {
                itemText = itemSelectedTextColor_;
            } else if (isHovered) {
                itemText = itemHoverTextColor_;
            }

            const std::string& label = items_[static_cast<std::size_t>(index)];
            const SDL_Point textSize = renderer.measureText(label, fontSpec_);
            renderer.drawText(
                label,
                {
                    itemRect.x + itemPadding_,
                    itemRect.y + (itemRect.h - textSize.y) / 2
                },
                itemText,
                fontSpec_
            );
        }
    }

    void ComboBox::onMouseEvent(EventMouse& event) {
        if (!enabled_) {
            return;
        }

        const SDL_Rect box = getBoxRect();

        hoveredBox_ = containsPoint(box, event.getX(), event.getY());

        if (event.isMoved()) {
            hoveredIndex_ = open_ ? getItemIndexAt(event.getX(), event.getY()) : -1;
            return;
        }

        if (event.getButton() != SDL_BUTTON_LEFT || !event.isPressed()) {
            return;
        }

        if (open_) {
            const int index = getItemIndexAt(event.getX(), event.getY());

            if (index >= 0) {
                setSelectedIndex(index);
            }

            open_ = false;
            hoveredIndex_ = -1;
            return;
        }

        if (hoveredBox_) {
            open_ = true;
        }
    }

    void ComboBox::runSelectionActions() {
        const auto snapshot = selectionActions_;

        for (const SelectionAction& action : snapshot) {
            try {
                action(*this);
            } catch (const std::exception& error) {
                spdlog::error("ComboBox action failed: {}", error.what());
            }
        }
    }

} // emgui

// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/ListView.h"

#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace emgui {

    namespace {

        bool inside(const SDL_Rect& rect, int x, int y) {
            return x >= rect.x && x < rect.x + rect.w &&
                   y >= rect.y && y < rect.y + rect.h;
        }

    } // namespace

    ListView::ListView(int x, int y, int width, int height)
        : ScrollArea(x, y, width, height) {
        setContentRenderer(
            [this](RendererOpts& renderer, const SDL_Rect& viewport, int, int scrollY) {
                drawItems(renderer, viewport, scrollY);
            }
        );
    }

    ListView::~ListView() {
        if (window_ == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            if (mouseListener_ != EventManager::kInvalidListener) {
                manager->removeListener(mouseListener_);
            }
            if (keyListener_ != EventManager::kInvalidListener) {
                manager->removeListener(keyListener_);
            }
        }
    }

    void ListView::applyStyle(const StyleValues& values) {
        ScrollArea::applyStyle(values);

        itemTextColor_ = values.getColor("item-text-color", itemTextColor_);
        itemHoverColor_ = values.getColor("item-hover-color", itemHoverColor_);
        itemHoverTextColor_ = values.getColor(
            "item-hover-text-color",
            itemHoverTextColor_
        );
        itemSelectedColor_ = values.getColor(
            "item-selected-color",
            itemSelectedColor_
        );
        itemSelectedTextColor_ = values.getColor(
            "item-selected-text-color",
            itemSelectedTextColor_
        );

        itemHeight_ = std::max(12, values.getInt("item-height", itemHeight_));
        itemPadding_ = std::max(0, values.getInt("item-padding", itemPadding_));
        itemRadius_ = std::max(0, values.getInt("item-radius", itemRadius_));
        padding_ = std::max(0, values.getInt("padding", padding_));

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        const SDL_Point size = getLayoutSize();
        setContentSize(
            std::max(0, size.x),
            static_cast<int>(items_.size()) * itemHeight_
        );
    }

    void ListView::onWindowAttached(Window& window) {
        ScrollArea::onWindowAttached(window);

        if (mouseListener_ != EventManager::kInvalidListener ||
            keyListener_ != EventManager::kInvalidListener) {
            return;
        }

        SDL_Window* sdlWindow = window.getWindow();

        if (sdlWindow == nullptr) {
            return;
        }

        mouseListener_ = EventManager::registerEvent(
            sdlWindow,
            EventMouse(sdlWindow),
            [this](EventMouse& event) {
                onMouseEvent(event);
            }
        );

        keyListener_ = EventManager::registerEvent(
            sdlWindow,
            EventKeyboard(sdlWindow),
            [this](EventKeyboard& event) {
                onKeyEvent(event);
            }
        );
    }

    void ListView::setItems(std::vector<std::string> items) {
        items_ = std::move(items);
        selectedIndex_ = items_.empty()
            ? -1
            : std::clamp(selectedIndex_, 0, static_cast<int>(items_.size()) - 1);
        hoveredIndex_ = -1;

        const SDL_Point size = getLayoutSize();
        setContentSize(
            std::max(0, size.x),
            static_cast<int>(items_.size()) * itemHeight_
        );
    }

    void ListView::setSelectedIndex(int index) {
        if (index < 0 || index >= static_cast<int>(items_.size())) {
            index = -1;
        }

        if (selectedIndex_ == index) {
            return;
        }

        selectedIndex_ = index;
        setSelected(index >= 0);

        if (index >= 0) {
            ensureVisible(index);
        }

        runSelectionActions();
    }

    std::string ListView::getSelectedText() const {
        if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(items_.size())) {
            return {};
        }

        return items_[static_cast<std::size_t>(selectedIndex_)];
    }

    void ListView::onSelectionChanged(SelectionAction action) {
        if (action) {
            selectionActions_.push_back(std::move(action));
        }
    }

    void ListView::drawItems(
        RendererOpts& renderer,
        const SDL_Rect& viewport,
        int scrollY
    ) {
        if (items_.empty()) {
            return;
        }

        const int first = std::max(0, scrollY / itemHeight_ - 1);
        const int last = std::min(
            static_cast<int>(items_.size()) - 1,
            (scrollY + viewport.h) / itemHeight_ + 1
        );

        for (int index = first; index <= last; ++index) {
            const SDL_Rect itemRect{
                viewport.x + padding_,
                viewport.y + padding_ + index * itemHeight_ - scrollY,
                std::max(0, viewport.w - 2 * padding_),
                itemHeight_
            };

            const bool isSelected = index == selectedIndex_;
            const bool isHovered = index == hoveredIndex_;

            if (isSelected || isHovered) {
                renderer.fillRoundedRect(
                    itemRect,
                    itemRadius_,
                    isSelected ? itemSelectedColor_ : itemHoverColor_
                );
            }

            SDL_Color textColor = itemTextColor_;
            if (isSelected) {
                textColor = itemSelectedTextColor_;
            } else if (isHovered) {
                textColor = itemHoverTextColor_;
            }

            const std::string& label = items_[static_cast<std::size_t>(index)];
            const SDL_Point textSize = renderer.measureText(label, fontSpec_);
            renderer.drawText(
                label,
                {
                    itemRect.x + itemPadding_,
                    itemRect.y + (itemRect.h - textSize.y) / 2
                },
                textColor,
                fontSpec_
            );
        }
    }

    void ListView::onMouseEvent(EventMouse& event) {
        const SDL_Point size = getLayoutSize();
        const SDL_Point position = getPosition();
        const SDL_Rect viewport{position.x, position.y, size.x, size.y};
        const bool hovering = inside(viewport, event.getX(), event.getY());

        setHovered(hovering);

        int index = -1;

        if (hovering && !items_.empty()) {
            const int contentY = event.getY() - viewport.y - padding_ + getScrollY();
            index = contentY / itemHeight_;

            if (index < 0 || index >= static_cast<int>(items_.size())) {
                index = -1;
            }
        }

        hoveredIndex_ = index;

        if (event.isPressed() && event.getButton() == SDL_BUTTON_LEFT && index >= 0) {
            setSelectedIndex(index);
        }
    }

    void ListView::onKeyEvent(EventKeyboard& event) {
        if (!event.isPressed() || event.isRepeat() || items_.empty()) {
            return;
        }

        int index = selectedIndex_;

        switch (event.getKey()) {
            case SDLK_DOWN:
                index = index < 0 ? 0 : std::min(index + 1, static_cast<int>(items_.size()) - 1);
                break;
            case SDLK_UP:
                index = index < 0 ? static_cast<int>(items_.size()) - 1 : std::max(0, index - 1);
                break;
            case SDLK_HOME:
                index = 0;
                break;
            case SDLK_END:
                index = static_cast<int>(items_.size()) - 1;
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                runSelectionActions();
                return;
            default:
                return;
        }

        setSelectedIndex(index);
    }

    void ListView::ensureVisible(int index) {
        if (index < 0 || index >= static_cast<int>(items_.size())) {
            return;
        }

        const SDL_Point size = getLayoutSize();
        const int top = index * itemHeight_;
        const int bottom = top + itemHeight_;
        const int viewTop = getScrollY();
        const int viewBottom = viewTop + size.y;

        if (top < viewTop) {
            setScroll(getScrollX(), std::max(0, top - padding_));
        } else if (bottom > viewBottom) {
            setScroll(getScrollX(), bottom - size.y + padding_);
        }
    }

    void ListView::runSelectionActions() {
        const auto snapshot = selectionActions_;

        for (const SelectionAction& action : snapshot) {
            try {
                action(*this);
            } catch (const std::exception& error) {
                spdlog::error("ListView action failed: {}", error.what());
            }
        }
    }

} // emgui

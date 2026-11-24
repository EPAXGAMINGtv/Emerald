// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/MenuBar.h"

#include "emrldgui/Window.h"
#include "emrldgui/style/StyleValues.h"
#include "emrldgui/ui_component/components/ContextMenu.h"

#include <algorithm>
#include <utility>

namespace emgui {

    MenuBar::MenuBar(int x, int y, int width, int height)
        : UIComponent(x, y),
          width_(std::max(1, width)),
          height_(std::max(1, height)) {}

    MenuBar::~MenuBar() {
        if (listenerId_ == EventManager::kInvalidListener ||
            window_ == nullptr ||
            window_->getWindow() == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            manager->removeListener(listenerId_);
        }
    }

    void MenuBar::onWindowAttached(Window& window) {
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

    void MenuBar::applyStyle(const StyleValues& values) {
        backgroundColor_ = values.getColor("background-color", backgroundColor_);
        barColor_ = values.getColor("bar-color", barColor_);
        borderColor_ = values.getColor("border-color", borderColor_);
        textColor_ = values.getColor("text-color", textColor_);
        hoverBackgroundColor_ = values.getColor(
            "hover-background-color",
            hoverBackgroundColor_
        );
        activeBackgroundColor_ = values.getColor(
            "active-background-color",
            activeBackgroundColor_
        );
        hoverTextColor_ = values.getColor("hover-text-color", hoverTextColor_);
        activeTextColor_ = values.getColor("active-text-color", activeTextColor_);

        borderWidth_ = std::max(0, values.getInt("border-width", borderWidth_));
        borderRadius_ = std::max(0, values.getInt("border-radius", borderRadius_));
        itemSpacing_ = std::max(0, values.getInt("item-spacing", itemSpacing_));
        itemPadding_ = std::max(0, values.getInt("item-padding", itemPadding_));

        padding_.apply(values, 6);
        shadow_.apply(values);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);
    }

    bool MenuBar::containsPoint(const SDL_Rect& rect, int x, int y) {
        return x >= rect.x && x < rect.x + rect.w &&
               y >= rect.y && y < rect.y + rect.h;
    }

    SDL_Rect MenuBar::getBarRect() const {
        return {x_, y_, width_, height_};
    }

    SDL_Rect MenuBar::getItemRect(int index, RendererOpts& renderer) const {
        if (index < 0 || index >= static_cast<int>(entries_.size())) {
            return {0, 0, 0, 0};
        }

        const SDL_Rect bar = getBarRect();
        int cursor = bar.x + padding_.left;

        for (int current = 0; current < index; ++current) {
            const SDL_Point measured = renderer.measureText(
                entries_[static_cast<std::size_t>(current)].label,
                fontSpec_
            );
            cursor += measured.x + 2 * itemPadding_ + itemSpacing_;
        }

        const SDL_Point measured = renderer.measureText(
            entries_[static_cast<std::size_t>(index)].label,
            fontSpec_
        );
        const int itemHeight = std::max(1, bar.h - padding_.vertical());

        return {
            cursor,
            bar.y + padding_.top,
            measured.x + 2 * itemPadding_,
            itemHeight
        };
    }

    int MenuBar::getItemIndexAt(int x, int y) const {
        if (window_ == nullptr) {
            return -1;
        }

        RendererOpts& renderer = window_->getRendererOpts();

        for (int index = 0; index < static_cast<int>(entries_.size()); ++index) {
            const SDL_Rect item = getItemRect(index, renderer);

            if (item.w > 0 && containsPoint(item, x, y)) {
                return index;
            }
        }

        return -1;
    }

    void MenuBar::closeActiveMenu() {
        if (activeMenu_ != nullptr) {
            activeMenu_->closeAll();
        }

        activeMenu_ = nullptr;
        activeIndex_ = -1;
    }

    void MenuBar::update() {
        if (activeMenu_ != nullptr && !activeMenu_->isOpen()) {
            closeActiveMenu();
        }
    }

    void MenuBar::render(RendererOpts& renderer) {
        const SDL_Rect bar = getBarRect();

        shadow_.draw(renderer, bar, borderRadius_);
        renderer.fillRoundedRect(bar, borderRadius_, barColor_);

        for (int inset = 0; inset < borderWidth_; ++inset) {
            const SDL_Rect border{
                bar.x + inset,
                bar.y + inset,
                bar.w - 2 * inset,
                bar.h - 2 * inset
            };

            if (border.w <= 0 || border.h <= 0) {
                break;
            }

            renderer.drawRoundedRect(
                border,
                std::max(0, borderRadius_ - inset),
                borderColor_
            );
        }

        for (int index = 0; index < static_cast<int>(entries_.size()); ++index) {
            const SDL_Rect item = getItemRect(index, renderer);
            if (item.w <= 0 || item.x + item.w > bar.x + bar.w) {
                break;
            }

            const bool active = index == activeIndex_ &&
                                activeMenu_ != nullptr &&
                                activeMenu_->isOpen();
            const bool hovered = index == hoveredIndex_;

            if (active) {
                renderer.fillRoundedRect(item, borderRadius_, activeBackgroundColor_);
            } else if (hovered) {
                renderer.fillRoundedRect(item, borderRadius_, hoverBackgroundColor_);
            }

            SDL_Color foreground = textColor_;
            if (active) {
                foreground = activeTextColor_;
            } else if (hovered) {
                foreground = hoverTextColor_;
            }

            const std::string& label = entries_[static_cast<std::size_t>(index)].label;
            const SDL_Point measured = renderer.measureText(label, fontSpec_);
            renderer.drawText(
                label,
                {item.x + itemPadding_, item.y + (item.h - measured.y) / 2},
                foreground,
                fontSpec_
            );
        }
    }

    void MenuBar::onMouseEvent(EventMouse& event) {
        if (window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        if (event.isMoved()) {
            hoveredIndex_ = getItemIndexAt(event.getX(), event.getY());
            setHovered(hoveredIndex_ >= 0);
            return;
        }

        if (event.getButton() != SDL_BUTTON_LEFT || !event.isPressed()) {
            return;
        }

        const int index = getItemIndexAt(event.getX(), event.getY());
        const SDL_Rect bar = getBarRect();

        if (index < 0) {
            if (containsPoint(bar, event.getX(), event.getY())) {
                closeActiveMenu();
            }

            return;
        }

        const MenuEntry& entry = entries_[static_cast<std::size_t>(index)];

        if (index == activeIndex_ && activeMenu_ != nullptr && activeMenu_->isOpen()) {
            closeActiveMenu();
            return;
        }

        closeActiveMenu();
        activeIndex_ = index;
        activeMenu_ = entry.menu;

        if (activeMenu_ != nullptr) {
            const SDL_Rect item = getItemRect(index, window_->getRendererOpts());
            activeMenu_->setParent(nullptr);
            activeMenu_->showAt(item.x, bar.y + bar.h + 2);
        }
    }

} // emgui

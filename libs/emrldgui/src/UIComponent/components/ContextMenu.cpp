// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/ContextMenu.h"

#include "emrldgui/Window.h"
#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace emgui {

    namespace {

        SDL_Rect clampToViewport(SDL_Rect rect, SDL_Renderer* renderer, const Window* window) {
            int viewWidth = 0;
            int viewHeight = 0;

            if (renderer != nullptr) {
                SDL_RenderGetLogicalSize(renderer, &viewWidth, &viewHeight);
            }

            // A positive logical size (when set) is authoritative.
            if (viewWidth <= 0 || viewHeight <= 0) {
                viewWidth = 0;
                viewHeight = 0;

                if (window != nullptr) {
                    viewWidth = window->getWidth();
                    viewHeight = window->getHeight();
                }

                if ((viewWidth <= 0 || viewHeight <= 0) && renderer != nullptr) {
                    SDL_GetRendererOutputSize(renderer, &viewWidth, &viewHeight);
                }
            }

            if (viewWidth > 0 && viewHeight > 0) {
                rect.w = std::min(rect.w, viewWidth);
                rect.h = std::min(rect.h, viewHeight);
                rect.x = std::clamp(rect.x, 0, viewWidth - rect.w);
                rect.y = std::clamp(rect.y, 0, viewHeight - rect.h);
            }

            return rect;
        }

    } // namespace

    ContextMenu::~ContextMenu() {
        if (window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            if (mouseListener_ != EventManager::kInvalidListener) {
                manager->removeListener(mouseListener_);
            }
            if (keyboardListener_ != EventManager::kInvalidListener) {
                manager->removeListener(keyboardListener_);
            }
        }
    }

    void ContextMenu::onWindowAttached(Window& window) {
        SDL_Window* sdlWindow = window.getWindow();

        if (sdlWindow == nullptr) {
            return;
        }

        if (mouseListener_ == EventManager::kInvalidListener) {
            mouseListener_ = EventManager::registerEvent(
                sdlWindow,
                EventMouse(sdlWindow),
                [this](EventMouse& event) {
                    onMouseEvent(event);
                }
            );
        }

        if (keyboardListener_ == EventManager::kInvalidListener) {
            keyboardListener_ = EventManager::registerEvent(
                sdlWindow,
                EventKeyboard(sdlWindow),
                [this](EventKeyboard& event) {
                    onKeyboardEvent(event);
                }
            );
        }
    }

    void ContextMenu::applyStyle(const StyleValues& values) {
        backgroundColor_ = values.getColor("background-color", backgroundColor_);
        borderColor_ = values.getColor("border-color", borderColor_);
        textColor_ = values.getColor("text-color", textColor_);
        itemTextColor_ = values.getColor("item-text-color", itemTextColor_);
        itemHoverColor_ = values.getColor("item-hover-color", itemHoverColor_);
        itemHoverTextColor_ = values.getColor(
            "item-hover-text-color",
            itemHoverTextColor_
        );
        itemSelectedColor_ = values.getColor("item-selected-color", itemSelectedColor_);
        itemSelectedTextColor_ = values.getColor(
            "item-selected-text-color",
            itemSelectedTextColor_
        );
        itemDisabledColor_ = values.getColor("item-disabled-color", itemDisabledColor_);
        separatorColor_ = values.getColor("separator-color", separatorColor_);
        arrowColor_ = values.getColor("arrow-color", arrowColor_);
        checkColor_ = values.getColor("check-color", checkColor_);

        borderWidth_ = std::max(0, values.getInt("border-width", borderWidth_));
        borderRadius_ = std::max(0, values.getInt("border-radius", borderRadius_));
        itemRadius_ = std::max(0, values.getInt("item-radius", itemRadius_));
        itemHeight_ = std::max(12, values.getInt("item-height", itemHeight_));
        separatorHeight_ = std::max(1, values.getInt("separator-height", separatorHeight_));
        itemPadding_ = std::max(0, values.getInt("item-padding", itemPadding_));

        if (lowerCase(values.getString("menu-width", "")) == "auto") {
            menuWidth_ = 0;
        } else {
            menuWidth_ = std::max(0, values.getInt("menu-width", menuWidth_));
        }

        padding_.apply(values, 6);
        shadow_.apply(values);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);
    }

    void ContextMenu::showAt(int x, int y) {
        if (activeSubmenu_ != nullptr) {
            activeSubmenu_->closeAll();
            activeSubmenu_ = nullptr;
        }

        x_ = x;
        y_ = y;
        hoveredIndex_ = -1;
        open_ = true;
        setHovered(false);
        setVisible(true);
    }

    void ContextMenu::close() {
        open_ = false;
        hoveredIndex_ = -1;
        setHovered(false);

        if (activeSubmenu_ != nullptr) {
            activeSubmenu_->close();
            activeSubmenu_ = nullptr;
        }
    }

    void ContextMenu::closeAllImpl(std::vector<ContextMenu*>& visited) {
        if (std::find(visited.begin(), visited.end(), this) != visited.end()) {
            return;
        }

        visited.push_back(this);
        close();

        if (activeSubmenu_ != nullptr) {
            activeSubmenu_->closeAllImpl(visited);
        }

        for (const MenuItem& item : items_) {
            if (item.submenu != nullptr && item.submenu != this) {
                item.submenu->closeAllImpl(visited);
            }
        }
    }

    void ContextMenu::closeAll() {
        std::vector<ContextMenu*> visited;
        closeAllImpl(visited);
    }

    void ContextMenu::runAction(const Action& action) {
        if (!action) {
            return;
        }

        try {
            action(*this);
        } catch (const std::exception& error) {
            spdlog::error("ContextMenu action failed: {}", error.what());
        }
    }

    bool ContextMenu::containsPoint(const SDL_Rect& rect, int x, int y) {
        return x >= rect.x && x < rect.x + rect.w &&
               y >= rect.y && y < rect.y + rect.h;
    }

    int ContextMenu::itemHeightAt(std::size_t index) const {
        if (index >= items_.size()) {
            return 0;
        }

        return items_[index].separator ? separatorHeight_ : itemHeight_;
    }

    SDL_Rect ContextMenu::getPopupRect(RendererOpts& renderer) const {
        int textWidth = 0;

        for (const MenuItem& item : items_) {
            if (!item.separator && !item.text.empty()) {
                textWidth = std::max(
                    textWidth,
                    renderer.measureText(item.text, fontSpec_).x
                );
            }
        }

        // Reserve room for a check mark on the left and a submenu arrow on the right.
        const int reserved = fontSpec_.size + 8;
        int width = menuWidth_;
        if (width <= 0) {
            width = textWidth + padding_.horizontal() + 2 * itemPadding_ + reserved;
        }

        int height = padding_.vertical();
        for (std::size_t index = 0; index < items_.size(); ++index) {
            height += itemHeightAt(index);
        }

        SDL_Rect rect{
            x_,
            y_,
            std::max(1, width),
            std::max(1, height)
        };

        return clampToViewport(rect, renderer.getRenderer(), window_);
    }

    int ContextMenu::getItemIndexAt(int x, int y) const {
        if (!open_ || window_ == nullptr) {
            return -1;
        }

        const SDL_Rect popup = getPopupRect(window_->getRendererOpts());
        if (!containsPoint(popup, x, y)) {
            return -1;
        }

        int offsetY = padding_.top;
        for (std::size_t index = 0; index < items_.size(); ++index) {
            const int height = itemHeightAt(index);

            if (y >= popup.y + offsetY && y < popup.y + offsetY + height) {
                return static_cast<int>(index);
            }

            offsetY += height;
        }

        return -1;
    }

    bool ContextMenu::isPointInSubmenuTree(int x, int y) const {
        if (window_ == nullptr) {
            return false;
        }

        RendererOpts& renderer = window_->getRendererOpts();

        for (ContextMenu* submenu = activeSubmenu_;
             submenu != nullptr;
             submenu = submenu->activeSubmenu_) {
            if (submenu->isOpen() &&
                containsPoint(submenu->getPopupRect(renderer), x, y)) {
                return true;
            }
        }

        return false;
    }

    void ContextMenu::render(RendererOpts& renderer) {
        (void)renderer;
    }

    void ContextMenu::renderOverlay(RendererOpts& renderer) {
        if (!open_) {
            return;
        }

        const SDL_Rect popup = getPopupRect(renderer);

        shadow_.draw(renderer, popup, borderRadius_);
        renderer.fillRoundedRect(popup, borderRadius_, backgroundColor_);

        for (int inset = 0; inset < borderWidth_; ++inset) {
            const SDL_Rect border{
                popup.x + inset,
                popup.y + inset,
                popup.w - 2 * inset,
                popup.h - 2 * inset
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

        int offsetY = padding_.top;

        for (std::size_t index = 0; index < items_.size(); ++index) {
            const MenuItem& item = items_[index];
            const int height = itemHeightAt(index);
            const SDL_Rect itemRect{
                popup.x + padding_.left,
                popup.y + offsetY,
                popup.w - padding_.horizontal(),
                height
            };

            if (itemRect.w <= 0 || itemRect.h <= 0) {
                offsetY += height;
                continue;
            }

            if (item.separator) {
                renderer.fillRect(
                    {
                        itemRect.x,
                        itemRect.y + itemRect.h / 2,
                        itemRect.w,
                        1
                    },
                    separatorColor_
                );
                offsetY += height;
                continue;
            }

            const bool hovered = static_cast<int>(index) == hoveredIndex_ && item.enabled;
            const bool selected = item.checked;

            if (hovered) {
                renderer.fillRoundedRect(itemRect, itemRadius_, itemHoverColor_);
            } else if (selected) {
                renderer.fillRoundedRect(itemRect, itemRadius_, itemSelectedColor_);
            }

            SDL_Color foreground = itemTextColor_;
            if (!item.enabled) {
                foreground = itemDisabledColor_;
            } else if (hovered) {
                foreground = itemHoverTextColor_;
            } else if (selected) {
                foreground = itemSelectedTextColor_;
            }

            int textX = itemRect.x + itemPadding_;
            const int markSize = std::max(8, fontSpec_.size / 2);

            if (item.checked) {
                const int markX = itemRect.x + itemPadding_;
                const int markY = itemRect.y + (itemRect.h - markSize) / 2;
                const SDL_Color markColor = hovered ? itemHoverTextColor_ : checkColor_;

                renderer.drawLine(
                    {markX, markY + markSize / 2},
                    {markX + markSize / 3, markY + markSize},
                    markColor
                );
                renderer.drawLine(
                    {markX + markSize / 3, markY + markSize},
                    {markX + markSize, markY},
                    markColor
                );
                textX += markSize + 4;
            }

            const SDL_Point textSize = renderer.measureText(item.text, fontSpec_);

            if (!item.text.empty()) {
                renderer.drawText(
                    item.text,
                    {textX, itemRect.y + (itemRect.h - textSize.y) / 2},
                    foreground,
                    fontSpec_
                );
            }

            if (item.submenu != nullptr) {
                const int arrowSize = std::max(4, fontSpec_.size / 3);
                const int arrowX = itemRect.x + itemRect.w - itemPadding_ - arrowSize;
                const int arrowY = itemRect.y + itemRect.h / 2;

                renderer.drawLine(
                    {arrowX, arrowY - arrowSize},
                    {arrowX + arrowSize, arrowY},
                    arrowColor_
                );
                renderer.drawLine(
                    {arrowX + arrowSize, arrowY},
                    {arrowX, arrowY + arrowSize},
                    arrowColor_
                );
            }

            offsetY += height;
        }
    }

    void ContextMenu::onMouseEvent(EventMouse& event) {
        if (!open_ || window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        const int mouseX = event.getX();
        const int mouseY = event.getY();

        if (event.isMoved()) {
            // Moving into an open submenu must not close it.
            if (isPointInSubmenuTree(mouseX, mouseY)) {
                return;
            }

            const int index = getItemIndexAt(mouseX, mouseY);
            if (index == hoveredIndex_) {
                return;
            }

            if (activeSubmenu_ != nullptr &&
                (index < 0 || index >= static_cast<int>(items_.size()) ||
                 items_[static_cast<std::size_t>(index)].submenu != activeSubmenu_)) {
                activeSubmenu_->closeAll();
                activeSubmenu_ = nullptr;
            }

            hoveredIndex_ = index;
            setHovered(index >= 0);
            return;
        }

        if (event.getButton() != SDL_BUTTON_LEFT || !event.isPressed()) {
            return;
        }

        RendererOpts& renderer = window_->getRendererOpts();
        const SDL_Rect popup = getPopupRect(renderer);
        const bool inPopup = containsPoint(popup, mouseX, mouseY);
        const bool inSubmenu = isPointInSubmenuTree(mouseX, mouseY);

        if (!inPopup && !inSubmenu) {
            closeAll();
            return;
        }

        const int index = getItemIndexAt(mouseX, mouseY);
        if (index < 0 || index >= static_cast<int>(items_.size())) {
            return;
        }

        const MenuItem& item = items_[static_cast<std::size_t>(index)];
        if (item.separator || !item.enabled) {
            return;
        }

        if (item.submenu != nullptr) {
            if (activeSubmenu_ == item.submenu) {
                activeSubmenu_->closeAll();
                activeSubmenu_ = nullptr;
                return;
            }

            if (activeSubmenu_ != nullptr) {
                activeSubmenu_->closeAll();
                activeSubmenu_ = nullptr;
            }

            int offsetY = padding_.top;
            for (int previous = 0; previous < index; ++previous) {
                offsetY += itemHeightAt(static_cast<std::size_t>(previous));
            }

            item.submenu->setParent(this);
            item.submenu->showAt(popup.x + popup.w, popup.y + offsetY);
            activeSubmenu_ = item.submenu;
            return;
        }

        runAction(item.action);
        closeAll();
    }

    void ContextMenu::onKeyboardEvent(EventKeyboard& event) {
        if (event.isPressed() && event.getKey() == SDLK_ESCAPE && open_) {
            closeAll();
        }
    }

} // emgui

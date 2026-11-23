// Created by epax on 23.11.26.

#include "emrldgui/ui_component/components/Tabs.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <exception>
#include <string>
#include <utility>

namespace emgui {

    Tabs::Tabs(int x, int y, int width, int height)
        : UIComponent(x, y), width_(width), height_(height) {
        surface_.color = {16, 24, 46, 225};
        surface_.borderColor = {60, 72, 104, 255};
        surface_.hasBorderColor = true;
        surface_.borderWidth = 1;
        surface_.radii = {14, 14, 14, 14};
        padding_.top = 10;
        padding_.right = 10;
        padding_.bottom = 10;
        padding_.left = 10;
        geometry_.width = width_;
        geometry_.height = height_;
    }

    Tabs::~Tabs() {
        if (listenerId_ == EventManager::kInvalidListener || window_ == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            manager->removeListener(listenerId_);
        }
    }

    void Tabs::onWindowAttached(Window& window) {
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

    void Tabs::applyStyle(const StyleValues& values) {
        surface_.apply(values);
        labelStyle_.apply(values);
        geometry_.apply(values, width_, height_);

        width_ = geometry_.width;
        height_ = geometry_.height;
        padding_.apply(values, 10);

        tabBarColor_ = values.getColor("tab-bar-color", tabBarColor_);
        tabBackground_ = values.getColor("tab-background", tabBackground_);
        tabHoverBackground_ = values.getColor("tab-hover-background", tabHoverBackground_);
        tabActiveBackground_ = values.getColor("tab-active-background", tabActiveBackground_);
        tabBorderColor_ = values.getColor("tab-border-color", tabBorderColor_);
        tabTextColor_ = values.getColor("tab-text-color", tabTextColor_);
        tabActiveTextColor_ = values.getColor("tab-active-text-color", tabActiveTextColor_);
        indicatorColor_ = values.getColor("indicator-color", indicatorColor_);

        tabHeight_ = std::max(24, values.getInt("tab-height", tabHeight_));
        tabGap_ = std::max(0, values.getInt("tab-gap", tabGap_));
        tabPadding_ = std::max(4, values.getInt("tab-padding", tabPadding_));
        tabRadius_ = std::max(0, values.getInt("tab-radius", tabRadius_));
        indicatorHeight_ = std::max(0, values.getInt("indicator-height", indicatorHeight_));

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);
    }

    void Tabs::setLayoutRect(const SDL_Rect& rect) {
        x_ = rect.x;
        y_ = rect.y;
        width_ = rect.w;
        height_ = rect.h;
        geometry_.lockToLayout(rect.w, rect.h);
    }

    void Tabs::setSubtreeVisible(bool visible) {
        setVisible(visible);

        for (const Page& page : pages_) {
            if (page.component != nullptr) {
                page.component->setSubtreeVisible(visible);
            }
        }
    }

    int Tabs::addPage(std::string title, UIComponent* page) {
        if (page == nullptr) {
            return -1;
        }

        pages_.push_back({std::move(title), page});
        const int index = static_cast<int>(pages_.size()) - 1;

        if (activePage_ < 0) {
            activePage_ = index;
        }
        refreshPageVisibility();
        syncCssState();
        return index;
    }

    void Tabs::setActivePage(int index) {
        if (index < 0 || index >= static_cast<int>(pages_.size()) ||
            index == activePage_) {
            return;
        }

        activePage_ = index;
        refreshPageVisibility();
        syncCssState();

        const auto callbacks = selectionCallbacks_;

        for (const SelectionCallback& callback : callbacks) {
            try {
                callback(*this, activePage_);
            } catch (const std::exception& error) {
                spdlog::error("Tabs selection callback failed: {}", error.what());
            }
        }
    }

    void Tabs::refreshPageVisibility() {
        for (std::size_t index = 0; index < pages_.size(); ++index) {
            if (pages_[index].component != nullptr) {
                pages_[index].component->setSubtreeVisible(
                    static_cast<int>(index) == activePage_
                );
            }
        }
    }

    void Tabs::syncCssState() {
        setHovered(hoveredTab_ >= 0);
        setFocused(activePage_ >= 0);
        setSelected(activePage_ >= 0);
    }

    int Tabs::tabAt(int x, int y) const {
        for (std::size_t index = 0; index < tabRects_.size(); ++index) {
            const SDL_Rect& rect = tabRects_[index];

            if (x >= rect.x && x < rect.x + rect.w &&
                y >= rect.y && y < rect.y + rect.h) {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    void Tabs::onMouseEvent(EventMouse& event) {
        if (window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        const int index = tabAt(event.getX(), event.getY());

        if (index != hoveredTab_) {
            hoveredTab_ = index;
            syncCssState();
        }

        if (event.getButton() != SDL_BUTTON_LEFT) {
            return;
        }

        if (event.isPressed() && index >= 0) {
            setActive(true);
            setActivePage(index);
            return;
        }

        if (event.isReleased()) {
            setActive(false);
        }
    }

    void Tabs::updateTabRects(RendererOpts* renderer) {
        tabRects_.clear();

        if (pages_.empty()) {
            return;
        }

        const int available = std::max(0, width_ - padding_.horizontal());
        const int startX = x_ + padding_.left;
        const int barY = y_ + padding_.top;
        int cursor = startX;

        for (const Page& page : pages_) {
            const std::string label = labelStyle_.prepare(page.title);
            int textWidth = static_cast<int>(label.size()) * fontSpec_.size * 3 / 5;

            if (renderer != nullptr) {
                textWidth = renderer->measureText(label, fontSpec_).x;
            }

            textWidth += std::max(0, labelStyle_.letterSpacing) *
                         std::max(0, static_cast<int>(label.size()) - 1);

            int tabWidth = textWidth + 2 * tabPadding_;
            tabWidth = std::min(tabWidth, std::max(1, startX + available - cursor));

            tabRects_.push_back({cursor, barY, tabWidth, tabHeight_});
            cursor += tabWidth + tabGap_;

            if (cursor >= startX + available) {
                break;
            }
        }
    }

    void Tabs::layout() {
        updateTabRects(nullptr);

        const int contentTop = y_ + padding_.top + tabHeight_ + 4;
        const SDL_Rect content{
            x_ + padding_.left,
            contentTop,
            std::max(0, width_ - padding_.horizontal()),
            std::max(0, y_ + height_ - padding_.bottom - contentTop)
        };

        for (const Page& page : pages_) {
            if (page.component != nullptr && page.component->isVisible()) {
                page.component->setLayoutRect(content);
                page.component->layout();
            }
        }
    }

    void Tabs::render(RendererOpts& renderer) {
        updateTabRects(&renderer);

        const SDL_Rect container{x_, y_, width_, height_};
        surface_.drawShadow(renderer, container);
        surface_.drawBackground(renderer, container, surface_.color);
        surface_.drawBorder(renderer, container, surface_.borderColor);

        const SDL_Rect bar{
            x_ + padding_.left,
            y_ + padding_.top,
            std::max(0, width_ - padding_.horizontal()),
            tabHeight_
        };
        renderer.fillRoundedRect(bar, tabRadius_, tabBarColor_);

        for (std::size_t index = 0; index < pages_.size() &&
                                    index < tabRects_.size(); ++index) {
            const SDL_Rect& tab = tabRects_[index];
            const bool active = static_cast<int>(index) == activePage_;
            const bool hovered = static_cast<int>(index) == hoveredTab_;

            const SDL_Color background = active
                ? tabActiveBackground_
                : (hovered ? tabHoverBackground_ : tabBackground_);
            const SDL_Color foreground = active
                ? tabActiveTextColor_
                : tabTextColor_;

            renderer.fillRoundedRect(tab, tabRadius_, background);
            renderer.drawRoundedRect(tab, tabRadius_, tabBorderColor_);

            const std::string label = labelStyle_.prepare(pages_[index].title);
            const SDL_Point measured = renderer.measureText(label, fontSpec_);
            const int spacing = std::max(0, labelStyle_.letterSpacing) *
                                std::max(0, static_cast<int>(label.size()) - 1);
            const int textWidth = measured.x + spacing;
            const int textX = tab.x + std::max(0, (tab.w - textWidth) / 2);
            const int textY = tab.y + std::max(0, (tab.h - measured.y) / 2);

            labelStyle_.draw(renderer, label, {textX, textY}, foreground, fontSpec_);

            if (active && indicatorHeight_ > 0) {
                renderer.fillRect(
                    {
                        tab.x + tabRadius_,
                        tab.y + tab.h + 2,
                        std::max(0, tab.w - 2 * tabRadius_),
                        indicatorHeight_
                    },
                    indicatorColor_
                );
            }
        }
    }

} // emgui

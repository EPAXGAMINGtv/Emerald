// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/ScrollArea.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <exception>
#include <string>
#include <utility>

namespace emgui {

    ScrollArea::ScrollArea(int x, int y, int width, int height)
        : UIComponent(x, y),
          width_(std::max(0, width)),
          height_(std::max(0, height)) {
        padding_.top = 4;
        padding_.right = 4;
        padding_.bottom = 4;
        padding_.left = 4;

        // The area handles wheel scrolling once and forwards it to this rect
        verticalBar_.setWheelEnabled(false);
        horizontalBar_.setWheelEnabled(false);
        verticalBar_.setRenderedByParent(true);
        horizontalBar_.setRenderedByParent(true);

        verticalBar_.onValueChanged(
            [this](int value) {
                setScroll(value, scrollY_);
            }
        );
        horizontalBar_.onValueChanged(
            [this](int value) {
                setScroll(scrollX_, value);
            }
        );
    }

    ScrollArea::~ScrollArea() {
        if (window_ == nullptr) {
            return;
        }

        UIComponentManager& components = window_->getUIComponentManager();

        if (!components.isShuttingDown()) {
            components.removeComponent(&verticalBar_);
            components.removeComponent(&horizontalBar_);
        }

        if (listenerId_ == EventManager::kInvalidListener) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            manager->removeListener(listenerId_);
        }
    }

    void ScrollArea::onWindowAttached(Window& window) {
        auto& components = window.getUIComponentManager();

        verticalBar_.setRenderedByParent(true);
        horizontalBar_.setRenderedByParent(true);
        components.addComponent(&verticalBar_);
        components.addComponent(&horizontalBar_);

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

    void ScrollArea::applyStyle(const StyleValues& values) {
        backgroundColor_ = values.getColor("background-color", backgroundColor_);
        borderColor_ = values.getColor("border-color", borderColor_);
        borderWidth_ = std::max(0, values.getInt("border-width", borderWidth_));
        radius_ = std::max(0, values.getInt("radius", radius_));

        padding_.apply(values, 4);
        gap_ = std::max(0, values.getInt("gap", gap_));
        rowDirection_ = lowerCase(
            values.getString("direction", rowDirection_ ? "row" : "column")
        ) == "row";
        wheelStep_ = std::max(1, values.getInt("wheel-step", wheelStep_));
        scrollbarThickness_ = std::max(
            1,
            values.getInt("scrollbar-thickness", scrollbarThickness_)
        );
    }

    void ScrollArea::setLayoutRect(const SDL_Rect& rect) {
        x_ = rect.x;
        y_ = rect.y;
        width_ = std::max(0, rect.w);
        height_ = std::max(0, rect.h);
        updateScrollBars();
    }

    void ScrollArea::setSubtreeVisible(bool visible) {
        setVisible(visible);

        for (UIComponent* child : children_) {
            if (child != nullptr) {
                child->setSubtreeVisible(visible);
            }
        }
    }

    void ScrollArea::collectFocusable(std::vector<UIComponent*>& out) {
        UIComponent::collectFocusable(out);

        for (UIComponent* child : children_) {
            if (child != nullptr && child != &verticalBar_ &&
                child != &horizontalBar_) {
                child->collectFocusable(out);
            }
        }
    }

    void ScrollArea::addChild(UIComponent* child) {
        if (child == nullptr || child == this) {
            return;
        }

        if (std::find(children_.begin(), children_.end(), child) ==
            children_.end()) {
            children_.push_back(child);
            child->setRenderedByParent(true);
        }
    }

    void ScrollArea::removeChild(UIComponent* child) {
        children_.erase(
            std::remove(children_.begin(), children_.end(), child),
            children_.end()
        );
    }

    void ScrollArea::clearChildren() {
        children_.clear();
    }

    void ScrollArea::setContentSize(int width, int height) {
        contentWidth_ = std::max(0, width);
        contentHeight_ = std::max(0, height);
        contentSizeExplicit_ = true;
        clampScroll();
        updateScrollBars();
    }

    void ScrollArea::setContentRenderer(ContentRenderer renderer) {
        contentRenderer_ = std::move(renderer);
    }

    SDL_Rect ScrollArea::getViewportRect() const {
        return {x_, y_, width_, height_};
    }

    int ScrollArea::getBarViewportWidth() const {
        const int viewWidth = std::max(0, width_);
        const int viewHeight = std::max(0, height_);
        const int limit = std::max(1, std::min(viewWidth, viewHeight));
        const int thickness = std::clamp(scrollbarThickness_, 1, limit);
        const bool needVertical = contentHeight_ > viewHeight;
        return std::max(0, viewWidth - (needVertical ? thickness : 0));
    }

    int ScrollArea::getBarViewportHeight() const {
        const int viewWidth = std::max(0, width_);
        const int viewHeight = std::max(0, height_);
        const int limit = std::max(1, std::min(viewWidth, viewHeight));
        const int thickness = std::clamp(scrollbarThickness_, 1, limit);
        const bool needHorizontal = contentWidth_ > viewWidth;
        return std::max(0, viewHeight - (needHorizontal ? thickness : 0));
    }

    int ScrollArea::getMaxScrollX() const {
        return std::max(0, contentWidth_ - getBarViewportWidth());
    }

    int ScrollArea::getMaxScrollY() const {
        return std::max(0, contentHeight_ - getBarViewportHeight());
    }

    void ScrollArea::clampScroll() {
        scrollX_ = std::clamp(scrollX_, 0, getMaxScrollX());
        scrollY_ = std::clamp(scrollY_, 0, getMaxScrollY());
    }

    void ScrollArea::setScroll(int x, int y) {
        scrollX_ = std::clamp(x, 0, getMaxScrollX());
        scrollY_ = std::clamp(y, 0, getMaxScrollY());
        syncBarsFromScroll();
    }

    void ScrollArea::scrollBy(int dx, int dy) {
        setScroll(scrollX_ + dx, scrollY_ + dy);
    }

    void ScrollArea::syncBarsFromScroll() {
        // ScrollBar::setValue does not fire callbacks, so this cannot recurse
        verticalBar_.setValue(scrollY_);
        horizontalBar_.setValue(scrollX_);
    }

    void ScrollArea::updateScrollBars() {
        const int viewWidth = std::max(0, width_);
        const int viewHeight = std::max(0, height_);
        const int limit = std::max(1, std::min(viewWidth, viewHeight));
        const int thickness = std::clamp(scrollbarThickness_, 1, limit);
        const bool needVertical = contentHeight_ > viewHeight;
        const bool needHorizontal = contentWidth_ > viewWidth;
        const int barWidth = getBarViewportWidth();
        const int barHeight = getBarViewportHeight();

        verticalBar_.setVisible(needVertical);
        horizontalBar_.setVisible(needHorizontal);

        verticalBar_.setLayoutRect(
            {x_ + barWidth, y_, thickness, barHeight}
        );
        horizontalBar_.setLayoutRect(
            {x_, y_ + barHeight, barWidth, thickness}
        );

        verticalBar_.setRange(contentHeight_, barHeight);
        horizontalBar_.setRange(contentWidth_, barWidth);
        syncBarsFromScroll();
    }

    void ScrollArea::layout() {
        if (!contentSizeExplicit_) {
            int mainTotal = 0;
            int crossMax = 0;
            int count = 0;

            for (UIComponent* child : children_) {
                if (child == nullptr || !child->isVisible()) {
                    continue;
                }

                const SDL_Point natural = child->getLayoutSize();
                const int mainSize = std::max(
                    0,
                    rowDirection_ ? natural.x : natural.y
                );
                const int crossSize = std::max(
                    0,
                    rowDirection_ ? natural.y : natural.x
                );

                mainTotal += mainSize;
                crossMax = std::max(crossMax, crossSize);
                ++count;
            }

            if (count > 1) {
                mainTotal += gap_ * (count - 1);
            }

            if (rowDirection_) {
                contentWidth_ = mainTotal + padding_.horizontal();
                contentHeight_ = crossMax + padding_.vertical();
            } else {
                contentWidth_ = crossMax + padding_.horizontal();
                contentHeight_ = mainTotal + padding_.vertical();
            }
        }

        clampScroll();
        updateScrollBars();

        const int innerX = x_ + padding_.left;
        const int innerY = y_ + padding_.top;
        int cursor = 0;

        for (UIComponent* child : children_) {
            if (child == nullptr || !child->isVisible()) {
                continue;
            }

            const SDL_Point natural = child->getLayoutSize();
            const int childWidth = std::max(0, natural.x);
            const int childHeight = std::max(0, natural.y);
            SDL_Rect rect{};

            if (rowDirection_) {
                rect = {
                    innerX + cursor - scrollX_,
                    innerY - scrollY_,
                    childWidth,
                    childHeight
                };
                cursor += childWidth + gap_;
            } else {
                rect = {
                    innerX - scrollX_,
                    innerY + cursor - scrollY_,
                    childWidth,
                    childHeight
                };
                cursor += childHeight + gap_;
            }

            child->setLayoutRect(rect);
            child->layout();
        }
    }

    void ScrollArea::render(RendererOpts& renderer) {
        const SDL_Rect viewport = getViewportRect();

        if (viewport.w <= 0 || viewport.h <= 0) {
            return;
        }

        renderer.pushClipRect(viewport);

        renderer.fillRoundedRect(viewport, radius_, backgroundColor_);

        for (int inset = 0; inset < borderWidth_; ++inset) {
            const SDL_Rect borderRect{
                viewport.x + inset,
                viewport.y + inset,
                viewport.w - 2 * inset,
                viewport.h - 2 * inset
            };

            if (borderRect.w <= 0 || borderRect.h <= 0) {
                break;
            }

            renderer.drawRoundedRect(
                borderRect,
                std::max(0, radius_ - inset),
                borderColor_
            );
        }

        for (UIComponent* child : children_) {
            if (child != nullptr && child->isVisible()) {
                child->render(renderer);
            }
        }

        if (contentRenderer_) {
            try {
                contentRenderer_(renderer, viewport, scrollX_, scrollY_);
            } catch (const std::exception& error) {
                spdlog::error("ScrollArea content renderer failed: {}", error.what());
            }
        }

        renderer.popClipRect();

        // Bars sit outside the clipped content and are drawn on top of it
        if (verticalBar_.isVisible()) {
            verticalBar_.render(renderer);
        }
        if (horizontalBar_.isVisible()) {
            horizontalBar_.render(renderer);
        }
    }

    void ScrollArea::onMouseEvent(EventMouse& event) {
        if (!event.isWheel()) {
            return;
        }

        // EventMouse wheel events carry no cursor coordinates, so a wheel
        // event is treated as targeting the area when it can scroll.
        const int deltaX = event.getScrollX();
        const int deltaY = event.getScrollY();

        if (deltaX == 0 && deltaY == 0) {
            return;
        }

        // SDL reports positive Y for wheel-up, which decreases scrollY
        scrollBy(deltaX * wheelStep_, -deltaY * wheelStep_);
    }

} // emgui

// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/Splitter.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <utility>

namespace emgui {

    namespace {

        bool inside(const SDL_Rect& rect, int x, int y) {
            return rect.w > 0 && rect.h > 0 &&
                   x >= rect.x && x < rect.x + rect.w &&
                   y >= rect.y && y < rect.y + rect.h;
        }

    } // namespace

    Splitter::Splitter(
        int x,
        int y,
        int width,
        int height,
        Orientation orientation
    )
        : UIComponent(x, y),
          width_(std::max(0, width)),
          height_(std::max(0, height)),
          orientation_(orientation) {
        setFocusable(true);
    }

    Splitter::~Splitter() {
        if (window_ != nullptr) {
            SDL_Window* sdlWindow = window_->getWindow();

            if (sdlWindow != nullptr) {
                if (auto* manager = EventManager::getForWindow(sdlWindow)) {
                    manager->removeListener(mouseListener_);
                    manager->removeListener(keyListener_);
                }
            }
        }

        if (resizeCursor_ != nullptr) {
            SDL_FreeCursor(resizeCursor_);
        }
    }

    void Splitter::onWindowAttached(Window& window) {
        if (mouseListener_ != EventManager::kInvalidListener) {
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

    void Splitter::applyStyle(const StyleValues& values) {
        surface_.apply(values);
        padding_.apply(values, 0);
        focusRing_.apply(values);

        handleColor_ = values.getColor("handle-color", handleColor_);
        handleHoverColor_ = values.getColor(
            "handle-hover-color",
            handleHoverColor_
        );
        handleActiveColor_ = values.getColor(
            "handle-active-color",
            handleActiveColor_
        );
        gripColor_ = values.getColor("grip-color", gripColor_);

        handleWidth_ = std::max(0, values.getInt("handle-width", handleWidth_));
        gripSize_ = std::max(1, values.getInt("grip-size", gripSize_));
        gripGap_ = std::max(0, values.getInt("grip-gap", gripGap_));
        cursorHint_ = lowerCase(
            values.getString("cursor-hint", cursorHint_)
        );

        layout();
    }

    void Splitter::setLayoutRect(const SDL_Rect& rect) {
        x_ = rect.x;
        y_ = rect.y;
        width_ = std::max(0, rect.w);
        height_ = std::max(0, rect.h);
        layout();
    }

    void Splitter::setSubtreeVisible(bool visible) {
        setVisible(visible);

        if (first_ != nullptr) {
            first_->setSubtreeVisible(visible);
        }
        if (second_ != nullptr) {
            second_->setSubtreeVisible(visible);
        }
    }

    void Splitter::collectFocusable(std::vector<UIComponent*>& out) {
        UIComponent::collectFocusable(out);

        if (first_ != nullptr) {
            first_->collectFocusable(out);
        }
        if (second_ != nullptr) {
            second_->collectFocusable(out);
        }
    }

    void Splitter::setFirst(UIComponent* component) {
        first_ = component;

        if (first_ != nullptr) {
            first_->setRenderedByParent(true);
        }

        layout();
    }

    void Splitter::setSecond(UIComponent* component) {
        second_ = component;

        if (second_ != nullptr) {
            second_->setRenderedByParent(true);
        }

        layout();
    }

    void Splitter::setSplit(float fraction) {
        const float clamped = clampFraction(fraction);

        if (std::fabs(clamped - split_) < 0.0001f) {
            return;
        }

        split_ = clamped;
        notifySplitChanged();
        layout();
    }

    void Splitter::setMinPaneSize(int pixels) {
        minPaneSize_ = std::max(0, pixels);
        setSplit(split_);
        layout();
    }

    void Splitter::onSplitChanged(SplitCallback callback) {
        if (callback) {
            splitCallbacks_.push_back(std::move(callback));
        }
    }

    void Splitter::notifySplitChanged() {
        const auto callbacks = splitCallbacks_;

        for (const SplitCallback& callback : callbacks) {
            try {
                callback(*this);
            } catch (const std::exception& error) {
                spdlog::error("Splitter split callback failed: {}", error.what());
            }
        }
    }

    SDL_Rect Splitter::getInnerRect() const {
        return {
            x_ + padding_.left,
            y_ + padding_.top,
            std::max(0, width_ - padding_.horizontal()),
            std::max(0, height_ - padding_.vertical())
        };
    }

    float Splitter::clampFraction(float value) const {
        const SDL_Rect inner = getInnerRect();
        const int mainLength = orientation_ == Orientation::Vertical
            ? inner.w
            : inner.h;
        const int thickness = std::min(handleWidth_, mainLength);
        const int usable = std::max(0, mainLength - thickness);

        if (usable <= 0) {
            return 0.0f;
        }

        const int half = usable / 2;
        const int minSize = std::min(minPaneSize_, half);
        const float minFraction =
            static_cast<float>(minSize) / static_cast<float>(usable);

        return std::clamp(value, minFraction, 1.0f - minFraction);
    }

    Splitter::Geometry Splitter::computeGeometry() const {
        Geometry result{};

        const SDL_Rect inner = getInnerRect();

        if (first_ == nullptr || second_ == nullptr) {
            if (first_ != nullptr) {
                result.first = inner;
            }
            if (second_ != nullptr) {
                result.second = inner;
            }
            return result;
        }

        const bool vertical = orientation_ == Orientation::Vertical;
        const int mainLength = vertical ? inner.w : inner.h;
        const int thickness = std::min(handleWidth_, mainLength);
        const int usable = std::max(0, mainLength - thickness);

        const int half = usable / 2;
        const int minSize = std::min(minPaneSize_, half);
        const int desired = static_cast<int>(
            std::lround(split_ * static_cast<float>(usable))
        );
        const int firstLength = std::clamp(desired, minSize, usable - minSize);
        const int secondLength = usable - firstLength;

        if (vertical) {
            result.first = {inner.x, inner.y, firstLength, inner.h};
            result.second = {
                inner.x + firstLength + thickness,
                inner.y,
                secondLength,
                inner.h
            };
            result.handle = {inner.x + firstLength, inner.y, thickness, inner.h};
        } else {
            result.first = {inner.x, inner.y, inner.w, firstLength};
            result.second = {
                inner.x,
                inner.y + firstLength + thickness,
                inner.w,
                secondLength
            };
            result.handle = {inner.x, inner.y + firstLength, inner.w, thickness};
        }

        return result;
    }

    void Splitter::layout() {
        const Geometry geometry = computeGeometry();

        if (first_ != nullptr) {
            first_->setLayoutRect(geometry.first);

            if (first_->isVisible()) {
                first_->layout();
            }
        }

        if (second_ != nullptr) {
            second_->setLayoutRect(geometry.second);

            if (second_->isVisible()) {
                second_->layout();
            }
        }
    }

    void Splitter::render(RendererOpts& renderer) {
        const SDL_Rect rect{x_, y_, width_, height_};

        if (rect.w <= 0 || rect.h <= 0) {
            return;
        }

        surface_.drawShadow(renderer, rect);
        surface_.drawBackground(renderer, rect, surface_.color);
        surface_.drawBorder(renderer, rect, surface_.borderColor);

        // Registered panes are flagged as parent-rendered, so the manager skips
        // them and this splitter is the only place they get drawn.
        if (first_ != nullptr && first_->isVisible() &&
            first_->isRenderedByParent()) {
            first_->render(renderer);
        }
        if (second_ != nullptr && second_->isVisible() &&
            second_->isRenderedByParent()) {
            second_->render(renderer);
        }

        const Geometry geometry = computeGeometry();

        if (geometry.handle.w > 0 && geometry.handle.h > 0) {
            drawHandle(renderer, geometry.handle);
            focusRing_.draw(renderer, geometry.handle, 4, isFocused());
        }
    }

    void Splitter::drawHandle(RendererOpts& renderer, const SDL_Rect& rect) const {
        SDL_Color color = handleColor_;

        if (dragging_) {
            color = handleActiveColor_;
        } else if (isHovered()) {
            color = handleHoverColor_;
        }

        const int radius = std::min(handleWidth_ / 2, 8);
        renderer.fillRoundedRect(rect, radius, color);

        const bool vertical = orientation_ == Orientation::Vertical;
        const int crossLength = vertical ? rect.h : rect.w;
        const int crossStart = vertical ? rect.y : rect.x;
        const int dotCount = 3;
        const int span = dotCount * gripSize_ +
                         (dotCount - 1) * gripGap_;

        if (span > crossLength) {
            return;
        }

        int center = crossStart + (crossLength - span) / 2 + gripSize_ / 2;
        const int dotRadius = std::max(1, gripSize_ / 2);

        for (int index = 0; index < dotCount; ++index) {
            const SDL_Point point = vertical
                ? SDL_Point{rect.x + rect.w / 2, center}
                : SDL_Point{center, rect.y + rect.h / 2};

            renderer.fillCircle(point, dotRadius, gripColor_);
            center += gripSize_ + gripGap_;
        }
    }

    bool Splitter::containsHandle(int x, int y) const {
        const Geometry geometry = computeGeometry();
        return inside(geometry.handle, x, y);
    }

    void Splitter::updateSplitFromPointer(int coordinate) {
        const SDL_Rect inner = getInnerRect();
        const bool vertical = orientation_ == Orientation::Vertical;
        const int mainLength = vertical ? inner.w : inner.h;
        const int thickness = std::min(handleWidth_, mainLength);
        const int usable = std::max(0, mainLength - thickness);

        if (usable <= 0) {
            return;
        }

        const int start = vertical ? inner.x : inner.y;
        const float fraction =
            static_cast<float>(coordinate - dragOffset_ - start) /
            static_cast<float>(usable);

        setSplit(fraction);
    }

    SDL_SystemCursor Splitter::getResizeCursor() const {
        if (cursorHint_ == "sizewe" || cursorHint_ == "ew-resize" ||
            cursorHint_ == "col-resize") {
            return SDL_SYSTEM_CURSOR_SIZEWE;
        }
        if (cursorHint_ == "sizens" || cursorHint_ == "ns-resize" ||
            cursorHint_ == "row-resize") {
            return SDL_SYSTEM_CURSOR_SIZENS;
        }

        return orientation_ == Orientation::Vertical
            ? SDL_SYSTEM_CURSOR_SIZEWE
            : SDL_SYSTEM_CURSOR_SIZENS;
    }

    void Splitter::updateCursor(bool overHandle) {
        if (!overHandle || cursorHint_ == "none" ||
            cursorHint_ == "default") {
            if (cursorActive_ && SDL_GetCursor() == resizeCursor_) {
                SDL_Cursor* fallback = SDL_GetDefaultCursor();

                if (fallback != nullptr) {
                    SDL_SetCursor(fallback);
                }
            }
            cursorActive_ = false;
            return;
        }

        if (resizeCursor_ == nullptr) {
            resizeCursor_ = SDL_CreateSystemCursor(getResizeCursor());
        }

        if (resizeCursor_ != nullptr) {
            SDL_SetCursor(resizeCursor_);
            cursorActive_ = true;
        }
    }

    void Splitter::onMouseEvent(EventMouse& event) {
        if (!isVisible() || isDisabled()) {
            updateCursor(false);
            return;
        }

        const bool overHandle = containsHandle(event.getX(), event.getY());
        setHovered(overHandle);
        updateCursor(overHandle || dragging_);

        if (event.isMoved()) {
            if (dragging_) {
                updateSplitFromPointer(
                    orientation_ == Orientation::Vertical
                        ? event.getX()
                        : event.getY()
                );
                event.handled = true;
            }
            return;
        }

        if (event.getButton() != SDL_BUTTON_LEFT) {
            return;
        }

        if (event.isPressed() && overHandle) {
            const Geometry geometry = computeGeometry();

            dragging_ = true;
            dragOffset_ = orientation_ == Orientation::Vertical
                ? event.getX() - geometry.handle.x
                : event.getY() - geometry.handle.y;
            setActive(true);

            if (window_ != nullptr) {
                window_->getUIComponentManager().setFocusedComponent(this);
            }

            event.handled = true;
            return;
        }

        if (event.isReleased() && dragging_) {
            dragging_ = false;
            setActive(false);
            updateCursor(overHandle);
            event.handled = true;
        }
    }

    void Splitter::onKeyEvent(EventKeyboard& event) {
        if (!isFocused() || isDisabled() || !event.isPressed()) {
            return;
        }

        const float step =
            (event.getModifiers() & KMOD_SHIFT) != KMOD_NONE ? 0.1f : 0.02f;

        switch (event.getKey()) {
            case SDLK_LEFT:
            case SDLK_UP:
                setSplit(split_ - step);
                break;
            case SDLK_RIGHT:
            case SDLK_DOWN:
                setSplit(split_ + step);
                break;
            default:
                return;
        }

        event.handled = true;
    }

} // emgui

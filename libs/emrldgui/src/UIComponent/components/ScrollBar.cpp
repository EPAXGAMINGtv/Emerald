// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/ScrollBar.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace emgui {

    ScrollBar::ScrollBar(
        Orientation orientation,
        int x,
        int y,
        int width,
        int height
    )
        : UIComponent(x, y),
          orientation_(orientation),
          width_(std::max(0, width)),
          height_(std::max(0, height)) {
        setStyleClass(
            orientation_ == Orientation::Vertical ? "vertical" : "horizontal"
        );
    }

    ScrollBar::~ScrollBar() {
        if (listenerId_ == EventManager::kInvalidListener ||
            window_ == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            manager->removeListener(listenerId_);
        }
    }

    void ScrollBar::onWindowAttached(Window& window) {
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

    void ScrollBar::applyStyle(const StyleValues& values) {
        trackColor_ = values.getColor("track-color", trackColor_);
        thumbColor_ = values.getColor("thumb-color", thumbColor_);
        thumbHoverColor_ = values.getColor("thumb-hover-color", thumbHoverColor_);
        thumbPressedColor_ = values.getColor(
            "thumb-pressed-color",
            thumbPressedColor_
        );
        borderColor_ = values.getColor("border-color", borderColor_);

        borderWidth_ = std::max(0, values.getInt("border-width", borderWidth_));
        radius_ = std::max(0, values.getInt("radius", radius_));
        thickness_ = std::max(1, values.getInt("thickness", thickness_));
        wheelStep_ = std::max(1, values.getInt("wheel-step", wheelStep_));
    }

    void ScrollBar::setRange(int contentSize, int viewportSize) {
        contentSize_ = std::max(0, contentSize);
        viewportSize_ = std::max(0, viewportSize);
        maxValue_ = std::max(0, contentSize_ - viewportSize_);
        setValue(value_);
    }

    void ScrollBar::setValue(int value) {
        const int clamped = std::clamp(value, 0, maxValue_);

        if (clamped == value_) {
            return;
        }

        value_ = clamped;
        notifyValueChanged();
    }

    void ScrollBar::setPageStep(int pageStep) {
        pageStep_ = std::max(1, pageStep);
    }

    void ScrollBar::onValueChanged(ValueChangedAction action) {
        if (action) {
            valueChangedActions_.push_back(std::move(action));
        }
    }

    void ScrollBar::notifyValueChanged() {
        // Copy so a callback may register another callback without invalidating the loop
        const auto snapshot = valueChangedActions_;

        for (const ValueChangedAction& action : snapshot) {
            try {
                action(value_);
            } catch (const std::exception& error) {
                spdlog::error("ScrollBar value callback failed: {}", error.what());
            }
        }
    }

    void ScrollBar::setHoveredState(bool hovered) {
        if (hovered_ == hovered) {
            return;
        }

        hovered_ = hovered;
        setHovered(hovered_);
    }

    void ScrollBar::setPressedState(bool pressed) {
        if (pressed_ == pressed) {
            return;
        }

        pressed_ = pressed;
        setActive(pressed_);
    }

    SDL_Point ScrollBar::getLayoutSize() const {
        return {width_, height_};
    }

    void ScrollBar::setLayoutRect(const SDL_Rect& rect) {
        x_ = rect.x;
        y_ = rect.y;
        width_ = std::max(0, rect.w);
        height_ = std::max(0, rect.h);
    }

    bool ScrollBar::containsPoint(int x, int y) const {
        return x >= x_ && x < x_ + width_ &&
               y >= y_ && y < y_ + height_;
    }

    SDL_Rect ScrollBar::getTrackRect() const {
        return {x_, y_, width_, height_};
    }

    SDL_Rect ScrollBar::getThumbRect() const {
        const SDL_Rect track = getTrackRect();

        if (track.w <= 0 || track.h <= 0) {
            return {track.x, track.y, 0, 0};
        }

        const bool vertical = orientation_ == Orientation::Vertical;
        const int trackLength = vertical ? track.h : track.w;
        const int crossLength = vertical ? track.w : track.h;
        const int thumbThickness = std::min(
            crossLength,
            std::max(1, thickness_)
        );

        int thumbLength = trackLength;

        if (contentSize_ > 0 && viewportSize_ > 0 &&
            contentSize_ > viewportSize_) {
            const auto scaled = static_cast<long long>(trackLength) *
                static_cast<long long>(viewportSize_) /
                static_cast<long long>(contentSize_);
            const int minLength = std::min(
                trackLength,
                std::max(24, 2 * thumbThickness)
            );
            thumbLength = std::clamp(
                static_cast<int>(scaled),
                minLength,
                trackLength
            );
        }

        int offset = 0;

        if (maxValue_ > 0 && thumbLength < trackLength) {
            const auto scaled =
                static_cast<long long>(trackLength - thumbLength) *
                static_cast<long long>(value_) /
                static_cast<long long>(maxValue_);
            offset = static_cast<int>(scaled);
        }

        const int crossOffset = (crossLength - thumbThickness) / 2;

        if (vertical) {
            return {
                track.x + crossOffset,
                track.y + offset,
                thumbThickness,
                thumbLength
            };
        }

        return {
            track.x + offset,
            track.y + crossOffset,
            thumbLength,
            thumbThickness
        };
    }

    void ScrollBar::render(RendererOpts& renderer) {
        const SDL_Rect track = getTrackRect();

        if (track.w <= 0 || track.h <= 0) {
            return;
        }

        renderer.fillRoundedRect(track, radius_, trackColor_);

        for (int inset = 0; inset < borderWidth_; ++inset) {
            const SDL_Rect borderRect{
                track.x + inset,
                track.y + inset,
                track.w - 2 * inset,
                track.h - 2 * inset
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

        const SDL_Rect thumb = getThumbRect();

        if (thumb.w <= 0 || thumb.h <= 0) {
            return;
        }

        const int thumbRadius = std::min(
            radius_,
            std::min(thumb.w, thumb.h) / 2
        );
        const SDL_Color thumbColor = pressed_
            ? thumbPressedColor_
            : hovered_ ? thumbHoverColor_ : thumbColor_;

        renderer.fillRoundedRect(thumb, thumbRadius, thumbColor);
        renderer.drawRoundedRect(thumb, thumbRadius, borderColor_);
    }

    void ScrollBar::onMouseEvent(EventMouse& event) {
        const bool vertical = orientation_ == Orientation::Vertical;
        const bool hovering = containsPoint(event.getX(), event.getY());

        if (event.isMoved()) {
            setHoveredState(hovering);

            if (!dragging_) {
                return;
            }

            const SDL_Rect track = getTrackRect();
            const SDL_Rect thumb = getThumbRect();
            const int trackLength = vertical ? track.h : track.w;
            const int thumbLength = vertical ? thumb.h : thumb.w;
            const int travel = trackLength - thumbLength;

            if (travel <= 0 || maxValue_ <= 0) {
                return;
            }

            const int trackStart = vertical ? track.y : track.x;
            const int pointer = vertical ? event.getY() : event.getX();
            const int thumbStart = pointer - dragOffset_ - trackStart;
            const auto scaled = static_cast<long long>(thumbStart) *
                static_cast<long long>(maxValue_) /
                static_cast<long long>(travel);

            setValue(static_cast<int>(scaled));
            return;
        }

        if (event.isWheel()) {
            if (!wheelEnabled_) {
                return;
            }

            // SDL reports positive Y for wheel-up, so invert the vertical delta
            const int delta = vertical ? event.getScrollY() : event.getScrollX();

            if (delta == 0) {
                return;
            }

            const int step = delta * std::max(1, wheelStep_);
            setValue(vertical ? value_ - step : value_ + step);
            return;
        }

        if (event.getButton() != SDL_BUTTON_LEFT) {
            return;
        }

        if (event.isPressed()) {
            if (!hovering) {
                return;
            }

            const SDL_Rect thumb = getThumbRect();
            const int thumbStart = vertical ? thumb.y : thumb.x;
            const int pointer = vertical ? event.getY() : event.getX();

            if (event.getX() >= thumb.x && event.getX() < thumb.x + thumb.w &&
                event.getY() >= thumb.y && event.getY() < thumb.y + thumb.h) {
                dragging_ = true;
                dragOffset_ = pointer - thumbStart;
            } else {
                setValue(value_ + (pointer < thumbStart ? -pageStep_ : pageStep_));
            }

            setPressedState(true);
            return;
        }

        if (event.isReleased() && (dragging_ || pressed_)) {
            dragging_ = false;
            setPressedState(false);
        }
    }

} // emgui

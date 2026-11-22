// Created by epax on 22.11.26.

#include "emrldgui/ui_component/components/Slider.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <iomanip>
#include <sstream>
#include <utility>

namespace emgui {

    namespace {

        std::string lower(std::string value) {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
            );
            return value;
        }

        bool parseBool(const std::string& value, bool fallback) {
            const std::string normalized = lower(value);

            if (normalized == "true" || normalized == "yes" || normalized == "1") {
                return true;
            }
            if (normalized == "false" || normalized == "no" || normalized == "0") {
                return false;
            }

            return fallback;
        }

    } // namespace

    Slider::Slider(
        float minimum,
        float maximum,
        float value,
        int x,
        int y,
        int width,
        int height
    )
        : UIComponent(x, y),
          minimum_(minimum),
          maximum_(maximum > minimum ? maximum : minimum + 1.0f),
          value_(value),
          width_(width),
          height_(height) {
        setValue(value_);
    }

    Slider::~Slider() {
        if (listenerId_ == EventManager::kInvalidListener ||
            window_ == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            manager->removeListener(listenerId_);
        }
    }

    void Slider::onWindowAttached(Window& window) {
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

    void Slider::onValueChanged(ValueAction action) {
        if (action) {
            valueActions_.push_back(std::move(action));
        }
    }

    void Slider::setRange(float minimum, float maximum) {
        minimum_ = minimum;
        maximum_ = maximum > minimum ? maximum : minimum + 1.0f;
        setValue(value_);
    }

    void Slider::setValue(float value) {
        float snapped = std::clamp(value, minimum_, maximum_);

        if (step_ > 0.0f) {
            const float steps = std::round((snapped - minimum_) / step_);
            snapped = minimum_ + steps * step_;
        }

        snapped = std::clamp(snapped, minimum_, maximum_);

        if (std::fabs(snapped - value_) < 0.0001f) {
            return;
        }

        value_ = snapped;
        notifyValueChanged();
    }

    float Slider::getNormalizedValue() const {
        const float range = maximum_ - minimum_;

        if (range <= 0.0f) {
            return 0.0f;
        }

        return std::clamp((value_ - minimum_) / range, 0.0f, 1.0f);
    }

    void Slider::applyStyle(const StyleValues& values) {
        backgroundColor_ = values.getColor("background-color", backgroundColor_);
        trackColor_ = values.getColor("track-color", trackColor_);
        trackHoverColor_ = values.getColor("track-hover-color", trackHoverColor_);
        trackFilledColor_ = values.getColor("track-filled-color", trackFilledColor_);
        trackFilledHoverColor_ = values.getColor(
            "track-filled-hover-color",
            trackFilledHoverColor_
        );
        handleColor_ = values.getColor("handle-color", handleColor_);
        handleHoverColor_ = values.getColor("handle-hover-color", handleHoverColor_);
        handleBorderColor_ = values.getColor(
            "handle-border-color",
            handleBorderColor_
        );
        disabledColor_ = values.getColor("disabled-color", disabledColor_);
        valueColor_ = values.getColor("value-color", valueColor_);

        backgroundRadius_ = std::max(
            0,
            values.getInt("background-radius", backgroundRadius_)
        );
        trackHeight_ = std::max(4, values.getInt("track-height", trackHeight_));
        handleSize_ = std::max(8, values.getInt("handle-size", handleSize_));
        handleBorderWidth_ = std::max(
            0,
            values.getInt("handle-border-width", handleBorderWidth_)
        );
        handleRadius_ = std::max(0, values.getInt("handle-radius", handleRadius_));
        padding_ = std::max(0, values.getInt("padding", padding_));
        valueOffset_ = std::max(0, values.getInt("value-offset", valueOffset_));

        showValue_ = parseBool(
            values.getString("show-value", showValue_ ? "true" : "false"),
            showValue_
        );

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        shadow_.apply(values);
        focusRing_.apply(values);

        spdlog::debug(
            "Slider style: track={} handle={} showValue={}",
            trackHeight_,
            handleSize_,
            showValue_
        );
    }

    void Slider::render(RendererOpts& renderer) {
        const SDL_Rect rect{x_, y_, width_, height_};

        if (backgroundColor_.a > 0) {
            shadow_.draw(renderer, rect, backgroundRadius_);
            renderer.fillRoundedRect(rect, backgroundRadius_, backgroundColor_);
        }

        const int trackX = x_ + padding_;
        const int trackWidth = std::max(1, width_ - 2 * padding_);
        const int centerY = y_ + height_ / 2;
        const SDL_Rect track{
            trackX,
            centerY - trackHeight_ / 2,
            trackWidth,
            trackHeight_
        };

        SDL_Color trackColor = trackColor_;
        SDL_Color filledColor = trackFilledColor_;
        SDL_Color handleColor = handleColor_;
        SDL_Color handleBorder = handleBorderColor_;

        if (hovered_ && enabled_ && !dragging_) {
            trackColor = trackHoverColor_;
            filledColor = trackFilledHoverColor_;
            handleColor = handleHoverColor_;
        }

        if (!enabled_) {
            trackColor = disabledColor_;
            filledColor = disabledColor_;
            handleColor = disabledColor_;
            handleBorder = disabledColor_;
        }

        renderer.fillRoundedRect(track, trackHeight_ / 2, trackColor);

        const float normalized = getNormalizedValue();
        const int filledWidth = static_cast<int>(
            std::lround(normalized * static_cast<float>(trackWidth))
        );

        if (filledWidth > 0) {
            renderer.fillRoundedRect(
                {track.x, track.y, filledWidth, track.h},
                trackHeight_ / 2,
                filledColor
            );
        }

        const int handleCenterX = trackX + filledWidth;
        const SDL_Rect handle{
            handleCenterX - handleSize_ / 2,
            centerY - handleSize_ / 2,
            handleSize_,
            handleSize_
        };

        renderer.fillRoundedRect(handle, handleRadius_, handleColor);

        for (int inset = 0; inset < handleBorderWidth_; ++inset) {
            const SDL_Rect borderRect{
                handle.x + inset,
                handle.y + inset,
                handle.w - 2 * inset,
                handle.h - 2 * inset
            };

            if (borderRect.w <= 0 || borderRect.h <= 0) {
                break;
            }

            renderer.drawRoundedRect(borderRect, handleRadius_, handleBorder);
        }

        if (showValue_) {
            const std::string valueText = formatValue();
            const SDL_Point textSize = renderer.measureText(valueText, fontSpec_);
            const int labelX = std::clamp(
                handleCenterX - textSize.x / 2,
                rect.x,
                rect.x + std::max(0, rect.w - textSize.x)
            );
            const int labelY = std::max(
                rect.y,
                handle.y - valueOffset_ - textSize.y
            );

            renderer.drawText(valueText, {labelX, labelY}, valueColor_, fontSpec_);
        }
    }

    bool Slider::containsPoint(int x, int y) const {
        return x >= x_ && x < x_ + width_ && y >= y_ && y < y_ + height_;
    }

    void Slider::updateValueFromMouseX(int mouseX) {
        const int trackX = x_ + padding_;
        const int trackWidth = std::max(1, width_ - 2 * padding_);
        const float normalized = std::clamp(
            static_cast<float>(mouseX - trackX) / static_cast<float>(trackWidth),
            0.0f,
            1.0f
        );
        setValue(minimum_ + normalized * (maximum_ - minimum_));
    }

    void Slider::onMouseEvent(EventMouse& event) {
        if (!enabled_) {
            return;
        }

        hovered_ = containsPoint(event.getX(), event.getY());

        if (event.isMoved() && dragging_) {
            updateValueFromMouseX(event.getX());
            return;
        }

        if (event.getButton() != SDL_BUTTON_LEFT) {
            return;
        }

        if (event.isPressed() && hovered_) {
            dragging_ = true;
            updateValueFromMouseX(event.getX());
        } else if (event.isReleased()) {
            dragging_ = false;
        }
    }

    void Slider::notifyValueChanged() {
        runActions(valueActions_);
    }

    void Slider::runActions(const std::vector<ValueAction>& actions) {
        const auto snapshot = actions;

        for (const ValueAction& action : snapshot) {
            try {
                action(*this);
            } catch (const std::exception& error) {
                spdlog::error("Slider action failed: {}", error.what());
            }
        }
    }

    std::string Slider::formatValue() const {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(step_ < 1.0f ? 1 : 0) << value_;
        return stream.str();
    }

} // emgui

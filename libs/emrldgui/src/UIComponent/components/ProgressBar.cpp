// Created by epax on 22.11.26.

#include "emrldgui/ui_component/components/ProgressBar.h"

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

    } // namespace

    ProgressBar::ProgressBar(
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
          value_(std::clamp(value, minimum_, maximum_)),
          displayedValue_(value_),
          width_(width),
          height_(height) {
        lastUpdateTicks_ = SDL_GetTicks();
    }

    void ProgressBar::onValueChanged(ValueAction action) {
        if (action) {
            valueActions_.push_back(std::move(action));
        }
    }

    void ProgressBar::setRange(float minimum, float maximum) {
        minimum_ = minimum;
        maximum_ = maximum > minimum ? maximum : minimum + 1.0f;
        setValue(value_);
    }

    void ProgressBar::setValue(float value) {
        const float next = std::clamp(value, minimum_, maximum_);

        if (std::fabs(next - value_) < 0.0001f) {
            return;
        }

        value_ = next;
        notifyValueChanged();
    }

    float ProgressBar::getNormalizedValue() const {
        const float range = maximum_ - minimum_;

        if (range <= 0.0f) {
            return 0.0f;
        }

        return std::clamp((value_ - minimum_) / range, 0.0f, 1.0f);
    }

    void ProgressBar::applyStyle(const StyleValues& values) {
        trackColor_ = values.getColor("track-color", trackColor_);
        fillColor_ = values.getColor("fill-color", fillColor_);
        fillColor2_ = values.getColor("fill-color-2", fillColor2_);
        shimmerColor_ = values.getColor("shimmer-color", shimmerColor_);
        textColor_ = values.getColor("text-color", textColor_);

        const auto gradient = values.getColors("fill-gradient");

        if (gradient.size() >= 2) {
            fillColor_ = gradient[0];
            fillColor2_ = gradient[1];
        }

        trackHeight_ = std::max(4, values.getInt("track-height", trackHeight_));
        radius_ = std::max(0, values.getInt("radius", radius_));
        padding_ = std::max(0, values.getInt("padding", padding_));
        showValue_ = values.getBool("show-value", showValue_);

        const std::string format = lower(
            values.getString("value-format", valueFormat_)
        );
        valueFormat_ = format == "value" || format == "percent" || format == "none"
            ? format
            : "percent";

        animationSpeed_ = std::max(
            0.5f,
            values.getFloat("animation-speed", animationSpeed_)
        );

        shadow_.apply(values);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);
    }

    void ProgressBar::update() {
        const Uint32 now = SDL_GetTicks();
        const Uint32 elapsed = now - lastUpdateTicks_;
        lastUpdateTicks_ = now;

        if (elapsed > 100) {
            return;
        }

        const float seconds = static_cast<float>(elapsed) / 1000.0f;

        if (!indeterminate_) {
            const float target = value_;
            const float difference = target - displayedValue_;

            if (std::fabs(difference) < 0.01f) {
                displayedValue_ = target;
            } else {
                const float step = 1.0f - std::exp(-animationSpeed_ * seconds);
                displayedValue_ += difference * std::clamp(step, 0.0f, 1.0f);
            }
        } else {
            phase_ += seconds * animationSpeed_;

            if (phase_ > 2.0f) {
                phase_ -= 2.0f;
            }
        }
    }

    void ProgressBar::render(RendererOpts& renderer) {
        const SDL_Rect area{x_, y_, width_, height_};
        const int trackHeight = std::min(trackHeight_, height_);
        const SDL_Rect track{
            x_ + padding_,
            y_ + (height_ - trackHeight) / 2,
            std::max(1, width_ - 2 * padding_),
            trackHeight
        };

        shadow_.draw(renderer, track, radius_);
        renderer.fillRoundedRect(track, radius_, trackColor_);

        if (indeterminate_) {
            const int shimmerWidth = std::max(
                track.w / 3,
                16
            );
            const float position = phase_ <= 1.0f
                ? phase_
                : 2.0f - phase_;
            const int shimmerX = track.x + static_cast<int>(
                std::lround(position * static_cast<float>(track.w - shimmerWidth))
            );

            renderer.fillRoundedRect(
                {shimmerX, track.y, shimmerWidth, track.h},
                radius_,
                blendColor(fillColor_, fillColor2_, 0.5f)
            );
            renderer.fillRoundedRect(
                {shimmerX, track.y, std::max(4, shimmerWidth / 4), track.h},
                radius_,
                shimmerColor_
            );
        } else {
            const float normalized = maximum_ > minimum_
                ? std::clamp((displayedValue_ - minimum_) / (maximum_ - minimum_), 0.0f, 1.0f)
                : 0.0f;
            const int filled = static_cast<int>(
                std::lround(normalized * static_cast<float>(track.w))
            );

            if (filled > 0) {
                renderer.fillRoundedRectGradient(
                    {track.x, track.y, filled, track.h},
                    radius_,
                    fillColor_,
                    fillColor2_
                );
            }
        }

        if (!showValue_) {
            return;
        }

        const std::string label = formatValue();

        if (label.empty()) {
            return;
        }

        const SDL_Point textSize = renderer.measureText(label, fontSpec_);
        renderer.drawText(
            label,
            {
                area.x + (area.w - textSize.x) / 2,
                area.y + (area.h - textSize.y) / 2
            },
            textColor_,
            fontSpec_
        );
    }

    void ProgressBar::notifyValueChanged() {
        runActions(valueActions_);
    }

    void ProgressBar::runActions(const std::vector<ValueAction>& actions) {
        const auto snapshot = actions;

        for (const ValueAction& action : snapshot) {
            try {
                action(*this);
            } catch (const std::exception& error) {
                spdlog::error("ProgressBar action failed: {}", error.what());
            }
        }
    }

    std::string ProgressBar::formatValue() const {
        if (!valueLabel_.empty()) {
            return valueLabel_;
        }

        if (valueFormat_ == "none") {
            return {};
        }

        std::ostringstream stream;

        if (valueFormat_ == "value") {
            stream << std::fixed << std::setprecision(value_ < 10.0f ? 1 : 0) << value_;
        } else {
            const int percent = static_cast<int>(std::lround(getNormalizedValue() * 100.0f));
            stream << percent << '%';
        }

        return stream.str();
    }

} // emgui

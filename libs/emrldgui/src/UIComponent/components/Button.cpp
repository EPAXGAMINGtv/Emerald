//
// Created by epax on 22.11.26.
//

#include "emrldgui/ui_component/components/Button.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>

namespace emgui {

    namespace {

        SDL_Color mixColor(const SDL_Color& from, const SDL_Color& to, float amount) {
            const float t = std::clamp(amount, 0.0f, 1.0f);

            const auto mixChannel = [t](Uint8 a, Uint8 b) {
                const float value =
                    static_cast<float>(a) * (1.0f - t) +
                    static_cast<float>(b) * t;
                return static_cast<Uint8>(std::lround(value));
            };

            return {
                mixChannel(from.r, to.r),
                mixChannel(from.g, to.g),
                mixChannel(from.b, to.b),
                mixChannel(from.a, to.a)
            };
        }

        void animate(float& value, float target, Uint32 deltaMs, int durationMs) {
            if (durationMs <= 0) {
                value = target;
                return;
            }

            const float step =
                static_cast<float>(deltaMs) / static_cast<float>(durationMs);

            if (value < target) {
                value = std::min(target, value + step);
            } else {
                value = std::max(target, value - step);
            }
        }

        std::string lower(std::string value) {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
            );
            return value;
        }

        const char* buttonTypeClass(Button::ButtonType type) {
            switch (type) {
                case Button::ButtonType::Primary: return "primary";
                case Button::ButtonType::Secondary: return "secondary";
                case Button::ButtonType::Success: return "success";
                case Button::ButtonType::Warning: return "warning";
                case Button::ButtonType::Danger: return "danger";
                case Button::ButtonType::Ghost: return "ghost";
                case Button::ButtonType::Normal:
                default: return "";
            }
        }

    } // namespace

    Button::Button(
        std::string text,
        int x,
        int y,
        int width,
        int height
    )
        : UIComponent(x, y),
          text_(std::move(text)),
          width_(width),
          height_(height) {
        lastUpdateTicks_ = SDL_GetTicks();
    }

    Button::~Button() {
        if (listenerId_ == EventManager::kInvalidListener ||
            window_ == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            manager->removeListener(listenerId_);
        }
    }

    void Button::onWindowAttached(Window& window) {
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

    void Button::onClick(Action action) {
        if (action) {
            clickActions_.push_back(std::move(action));
        }
    }

    void Button::onPressed(Action action) {
        if (action) {
            pressActions_.push_back(std::move(action));
        }
    }

    void Button::onReleased(Action action) {
        if (action) {
            releaseActions_.push_back(std::move(action));
        }
    }

    void Button::setButtonType(ButtonType type) {
        type_ = type;
        setStyleClass(buttonTypeClass(type));
    }

    void Button::applyStyle(const StyleValues& values) {
        backgroundColor_ = values.getColor("background-color", backgroundColor_);
        backgroundHoverColor_ = values.getColor(
            "background-hover-color",
            backgroundHoverColor_
        );
        backgroundPressedColor_ = values.getColor(
            "background-pressed-color",
            backgroundPressedColor_
        );
        disabledBackgroundColor_ = values.getColor(
            "disabled-background-color",
            disabledBackgroundColor_
        );

        const auto gradient = values.getColors("background-gradient");

        if (gradient.size() >= 2) {
            useGradient_ = true;
            gradientTopColor_ = gradient[0];
            gradientBottomColor_ = gradient[1];
        } else {
            useGradient_ = false;
        }

        borderColor_ = values.getColor("border-color", borderColor_);
        borderHoverColor_ = values.getColor("border-hover-color", borderHoverColor_);
        borderPressedColor_ = values.getColor(
            "border-pressed-color",
            borderPressedColor_
        );

        borderWidth_ = std::max(0, values.getInt("border-width", borderWidth_));
        borderRadius_ = std::max(0, values.getInt("border-radius", borderRadius_));

        shadow_.apply(values);
        focusRing_.apply(values);

        textColor_ = values.getColor("text-color", textColor_);
        textHoverColor_ = values.getColor("text-hover-color", textHoverColor_);
        textPressedColor_ = values.getColor("text-pressed-color", textPressedColor_);
        textDisabledColor_ = values.getColor("text-disabled-color", textDisabledColor_);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        padding_ = std::max(0, values.getInt("padding", padding_));

        const std::string align = lower(values.getString("text-align", textAlign_));
        textAlign_ = align == "left" || align == "right" ? align : "center";

        pressOffset_ = std::max(0, values.getInt("press-offset", pressOffset_));
        pressDurationMs_ = std::max(1, values.getInt("press-duration-ms", pressDurationMs_));
        releaseDurationMs_ = std::max(
            1,
            values.getInt("release-duration-ms", releaseDurationMs_)
        );

        spdlog::debug(
            "Button style: font={} size={} radius={} border={} align={}",
            fontSpec_.family,
            fontSpec_.size,
            borderRadius_,
            borderWidth_,
            textAlign_
        );
    }

    void Button::update() {
        const Uint32 now = SDL_GetTicks();
        const Uint32 deltaMs = lastUpdateTicks_ == 0
            ? 16
            : std::min(now - lastUpdateTicks_, static_cast<Uint32>(100));
        lastUpdateTicks_ = now;

        const float pressTarget = pressed_ && enabled_ ? 1.0f : 0.0f;
        const float hoverTarget = hovered_ && enabled_ ? 1.0f : 0.0f;

        animate(
            pressProgress_,
            pressTarget,
            deltaMs,
            pressed_ ? pressDurationMs_ : releaseDurationMs_
        );
        animate(
            hoverProgress_,
            hoverTarget,
            deltaMs,
            hovered_ ? pressDurationMs_ : releaseDurationMs_
        );
    }

    void Button::render(RendererOpts& renderer) {
        const float easedPress = easeOutCubic(pressProgress_);
        const float easedHover = easeOutCubic(hoverProgress_);
        const int offset = static_cast<int>(
            std::lround(easedPress * static_cast<float>(pressOffset_))
        );
        const SDL_Rect rect{x_, y_ + offset, width_, height_};

        SDL_Color background = mixColor(
            backgroundColor_,
            backgroundHoverColor_,
            easedHover
        );
        background = mixColor(background, backgroundPressedColor_, easedPress);

        SDL_Color border = mixColor(borderColor_, borderHoverColor_, easedHover);
        border = mixColor(border, borderPressedColor_, easedPress);

        SDL_Color foreground = mixColor(textColor_, textHoverColor_, easedHover);
        foreground = mixColor(foreground, textPressedColor_, easedPress);

        if (!enabled_) {
            background = disabledBackgroundColor_;
            border = borderColor_;
            foreground = textDisabledColor_;
        }

        shadow_.draw(renderer, rect, borderRadius_);

        if (useGradient_ && enabled_) {
            SDL_Color gradientTop = mixColor(
                gradientTopColor_,
                backgroundHoverColor_,
                easedHover
            );
            gradientTop = mixColor(gradientTop, backgroundPressedColor_, easedPress);

            SDL_Color gradientBottom = mixColor(
                gradientBottomColor_,
                backgroundHoverColor_,
                easedHover
            );
            gradientBottom = mixColor(
                gradientBottom,
                backgroundPressedColor_,
                easedPress
            );

            renderer.fillRoundedRectGradient(
                rect,
                borderRadius_,
                gradientTop,
                gradientBottom
            );
        } else {
            renderer.fillRoundedRect(rect, borderRadius_, background);
        }

        for (int inset = 0; inset < borderWidth_; ++inset) {
            const SDL_Rect borderRect{
                rect.x + inset,
                rect.y + inset,
                rect.w - 2 * inset,
                rect.h - 2 * inset
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

        focusRing_.draw(
            renderer,
            rect,
            borderRadius_,
            hovered_ && enabled_
        );

        const SDL_Point textSize = renderer.measureText(text_, fontSpec_);

        int textX = rect.x + padding_;

        if (textAlign_ == "right") {
            textX = rect.x + rect.w - padding_ - textSize.x;
        } else if (textAlign_ == "center") {
            textX = rect.x + (rect.w - textSize.x) / 2;
        }

        textX = std::max(rect.x, textX);

        const int textY = std::max(
            rect.y,
            rect.y + (rect.h - textSize.y) / 2
        );

        renderer.drawText(text_, {textX, textY}, foreground, fontSpec_);
    }

    bool Button::containsPoint(int x, int y) const {
        return x >= x_ && x < x_ + width_ && y >= y_ && y < y_ + height_;
    }

    void Button::onMouseEvent(EventMouse& event) {
        if (!enabled_ || window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        hovered_ = containsPoint(event.getX(), event.getY());

        if (event.getButton() != SDL_BUTTON_LEFT) {
            return;
        }

        if (event.isPressed()) {
            if (hovered_) {
                pressed_ = true;
                runActions(pressActions_);
            }
            return;
        }

        if (event.isReleased() && pressed_) {
            pressed_ = false;
            runActions(releaseActions_);

            if (hovered_) {
                runActions(clickActions_);
            }
        }
    }

    void Button::runActions(const std::vector<Action>& actions) {
        //copy so an action may register another action without invalidating the loop
        const auto snapshot = actions;

        for (const Action& action : snapshot) {
            try {
                action(*this);
            } catch (const std::exception& error) {
                spdlog::error("Button action failed: {}", error.what());
            }
        }
    }

} // emgui

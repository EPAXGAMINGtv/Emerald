// Created by epax on 22.11.26.

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
        surface_.color = {45, 116, 218, 255};
        surface_.borderColor = {27, 79, 156, 255};
        surface_.hasBorderColor = true;
        surface_.borderWidth = 2;
        surface_.radii = {10, 10, 10, 10};
        padding_.top = 10;
        padding_.right = 18;
        padding_.bottom = 10;
        padding_.left = 18;
        geometry_.width = width_;
        geometry_.height = height_;
        computedWidth_ = width_;
        computedHeight_ = height_;
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
        surface_.apply(values);
        textStyle_.apply(values);
        geometry_.apply(values, width_, height_);

        const auto stateGradient = [&values](const char* property) {
            const auto stops = values.getColors(property);
            return stops.size() >= 2 ? stops : std::vector<SDL_Color>{};
        };

        hoverGradient_ = stateGradient("background-hover-gradient");
        pressedGradient_ = stateGradient("background-pressed-gradient");
        disabledGradient_ = stateGradient("background-disabled-gradient");

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

        borderHoverColor_ = values.getColor("border-hover-color", borderHoverColor_);
        borderPressedColor_ = values.getColor(
            "border-pressed-color",
            borderPressedColor_
        );

        textColor_ = values.getColor("text-color", textColor_);
        textHoverColor_ = values.getColor("text-hover-color", textHoverColor_);
        textPressedColor_ = values.getColor("text-pressed-color", textPressedColor_);
        textDisabledColor_ = values.getColor("text-disabled-color", textDisabledColor_);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        padding_.apply(values, 10);

        const std::string align = lower(values.getString("text-align", textAlign_));
        textAlign_ = align == "left" || align == "right" ? align : "center";

        pressOffset_ = std::max(0, values.getInt("press-offset", pressOffset_));
        pressDurationMs_ = std::max(1, values.getInt("press-duration-ms", pressDurationMs_));
        releaseDurationMs_ = std::max(
            1,
            values.getInt("release-duration-ms", releaseDurationMs_)
        );

        computedWidth_ = geometry_.width;
        computedHeight_ = geometry_.height;

        spdlog::debug(
            "Button style: font={} size={} radius={} border={} align={} autoWidth={}",
            fontSpec_.family,
            fontSpec_.size,
            surface_.maxRadius(),
            surface_.borderWidth,
            textAlign_,
            geometry_.autoWidth
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

        const std::string preparedText = textStyle_.prepare(text_);
        const SDL_Point measured = renderer.measureText(preparedText, fontSpec_);
        const int spacing = textStyle_.letterSpacing *
                            std::max(0, static_cast<int>(preparedText.size()) - 1);
        const int contentWidth = measured.x + spacing + padding_.horizontal();
        const int contentHeight = measured.y + padding_.vertical();

        SDL_Rect rect = geometry_.resolve(x_, y_, contentWidth, contentHeight);
        computedWidth_ = rect.w;
        computedHeight_ = rect.h;
        rect.y += offset;

        SDL_Color background = surface_.color;
        SDL_Color border = surface_.borderColor;
        SDL_Color foreground = textColor_;
        std::vector<SDL_Color> stops = surface_.gradient;

        const auto blendState = [&stops](
            const std::vector<SDL_Color>& stateStops,
            const SDL_Color& stateColor,
            const SDL_Color& baseColor,
            float amount
        ) {
            if (!stateStops.empty()) {
                if (stops.size() != stateStops.size()) {
                    stops.assign(stateStops.size(), baseColor);
                }

                for (std::size_t index = 0; index < stops.size(); ++index) {
                    stops[index] = mixColor(stops[index], stateStops[index], amount);
                }
            } else if (!stops.empty()) {
                for (SDL_Color& stop : stops) {
                    stop = mixColor(stop, stateColor, amount);
                }
            }
        };

        blendState(hoverGradient_, backgroundHoverColor_, background, easedHover);
        background = mixColor(background, backgroundHoverColor_, easedHover);
        blendState(pressedGradient_, backgroundPressedColor_, background, easedPress);
        background = mixColor(background, backgroundPressedColor_, easedPress);

        border = mixColor(border, borderHoverColor_, easedHover);
        border = mixColor(border, borderPressedColor_, easedPress);
        foreground = mixColor(foreground, textHoverColor_, easedHover);
        foreground = mixColor(foreground, textPressedColor_, easedPress);

        if (!enabled_) {
            background = disabledBackgroundColor_;
            border = surface_.borderColor;
            foreground = textDisabledColor_;

            if (disabledGradient_.size() >= 2) {
                stops = disabledGradient_;
            } else if (!stops.empty()) {
                for (SDL_Color& stop : stops) {
                    stop = disabledBackgroundColor_;
                }
            }
        }

        surface_.drawShadow(renderer, rect);
        surface_.drawBackgroundStops(renderer, rect, background, stops);
        surface_.drawBorderColor(renderer, rect, border);
        const int textWidth = measured.x + spacing;
        int textX = rect.x + padding_.left;

        if (textAlign_ == "right") {
            textX = rect.x + rect.w - padding_.right - textWidth;
        } else if (textAlign_ == "center") {
            textX = rect.x + (rect.w - textWidth) / 2;
        }

        textX = std::max(rect.x + padding_.left, textX);

        const int textY = std::max(
            rect.y + padding_.top,
            rect.y + padding_.top +
                std::max(0, (rect.h - padding_.vertical() - measured.y) / 2)
        );

        textStyle_.draw(renderer, preparedText, {textX, textY}, foreground, fontSpec_);
    }

    bool Button::containsPoint(int x, int y) const {
        return x >= x_ && x < x_ + computedWidth_ &&
               y >= y_ && y < y_ + computedHeight_;
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
        // Copy so an action may register another action without invalidating the loop
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

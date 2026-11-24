// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/Tooltip.h"

#include "emrldgui/Window.h"
#include "emrldgui/style/StyleValues.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace emgui {

    namespace {

        std::vector<std::string> splitLines(const std::string& text) {
            std::vector<std::string> lines;
            std::string::size_type start = 0;

            while (true) {
                const std::string::size_type end = text.find('\n', start);
                lines.push_back(text.substr(start, end - start));

                if (end == std::string::npos) {
                    break;
                }

                start = end + 1;
            }

            return lines;
        }

        SDL_Rect clampToViewport(SDL_Rect rect, SDL_Renderer* renderer, const Window* window) {
            int viewWidth = 0;
            int viewHeight = 0;

            if (renderer != nullptr) {
                SDL_RenderGetLogicalSize(renderer, &viewWidth, &viewHeight);

                if (viewWidth <= 0 || viewHeight <= 0) {
                    viewWidth = 0;
                    viewHeight = 0;
                }
            }

            if ((viewWidth <= 0 || viewHeight <= 0) && window != nullptr) {
                viewWidth = window->getWidth();
                viewHeight = window->getHeight();
            }

            if ((viewWidth <= 0 || viewHeight <= 0) && renderer != nullptr) {
                SDL_GetRendererOutputSize(renderer, &viewWidth, &viewHeight);
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

    Tooltip::Tooltip(std::string text) : text_(std::move(text)) {}

    Tooltip::~Tooltip() {
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

    void Tooltip::onWindowAttached(Window& window) {
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

    void Tooltip::applyStyle(const StyleValues& values) {
        backgroundColor_ = values.getColor("background-color", backgroundColor_);
        borderColor_ = values.getColor("border-color", borderColor_);
        textColor_ = values.getColor("text-color", textColor_);

        borderWidth_ = std::max(0, values.getInt("border-width", borderWidth_));
        borderRadius_ = std::max(0, values.getInt("border-radius", borderRadius_));
        delayMs_ = std::max(0, values.getInt("delay-ms", delayMs_));
        displayMs_ = std::max(0, values.getInt("display-ms", displayMs_));
        fadeMs_ = std::max(0, values.getInt("fade-ms", fadeMs_));
        offsetX_ = values.getInt("offset-x", offsetX_);
        offsetY_ = values.getInt("offset-y", offsetY_);
        maxWidth_ = std::max(0, values.getInt("max-width", maxWidth_));

        padding_.apply(values, 10);
        shadow_.apply(values);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);
    }

    void Tooltip::beginShow() {
        shown_ = true;
        lastShowTick_ = SDL_GetTicks();
        setVisible(true);
    }

    void Tooltip::show(std::string text, int x, int y) {
        text_ = std::move(text);
        showAt(x, y);
    }

    void Tooltip::showAt(int x, int y) {
        anchor_ = nullptr;
        anchorShown_ = false;
        anchorExpired_ = false;
        x_ = x;
        y_ = y;
        beginShow();
    }

    void Tooltip::hide() {
        shown_ = false;
        anchorShown_ = false;
        anchorExpired_ = true;
    }

    void Tooltip::update() {
        if (anchor_ != nullptr) {
            const bool hovered = anchor_->isHovered();

            if (!hovered) {
                anchorShown_ = false;
                anchorExpired_ = false;

                if (shown_) {
                    hide();
                }

                return;
            }

            if (anchorExpired_) {
                return;
            }

            const SDL_Point anchorPosition = anchor_->getPosition();
            const SDL_Point anchorSize = anchor_->getLayoutSize();

            x_ = anchorPosition.x + offsetX_;
            y_ = anchorPosition.y + anchorSize.y + offsetY_;

            if (!anchorShown_) {
                anchorShown_ = true;
                beginShow();
                return;
            }
        }

        if (!shown_ || displayMs_ <= 0) {
            return;
        }

        const Uint32 elapsed = SDL_GetTicks() - lastShowTick_;
        const Uint32 totalMs = static_cast<Uint32>(delayMs_ + displayMs_);

        if (elapsed >= totalMs) {
            hide();
        }
    }

    bool Tooltip::containsPoint(const SDL_Rect& rect, int x, int y) {
        return x >= rect.x && x < rect.x + rect.w &&
               y >= rect.y && y < rect.y + rect.h;
    }

    SDL_Rect Tooltip::getCardRect(RendererOpts& renderer) const {
        const std::vector<std::string> lines = splitLines(text_);
        const int lineHeight = std::max(1, fontSpec_.size + 4);

        int textWidth = 0;
        for (const std::string& line : lines) {
            textWidth = std::max(textWidth, renderer.measureText(line, fontSpec_).x);
        }

        int width = textWidth + padding_.horizontal() + 2 * borderWidth_;
        if (maxWidth_ > 0) {
            width = std::min(width, maxWidth_);
        }

        const int lineCount = std::max(1, static_cast<int>(lines.size()));
        const int height = lineCount * lineHeight + padding_.vertical() + 2 * borderWidth_;

        SDL_Rect rect{
            x_,
            y_,
            std::max(1, width),
            std::max(1, height)
        };

        return clampToViewport(rect, renderer.getRenderer(), window_);
    }

    void Tooltip::render(RendererOpts& renderer) {
        (void)renderer;
    }

    void Tooltip::renderOverlay(RendererOpts& renderer) {
        if (!shown_) {
            return;
        }

        const Uint32 elapsed = SDL_GetTicks() - lastShowTick_;

        if (elapsed < static_cast<Uint32>(delayMs_)) {
            return;
        }

        const Uint32 totalMs = static_cast<Uint32>(delayMs_ + displayMs_);
        if (displayMs_ > 0 && elapsed >= totalMs) {
            return;
        }

        float opacity = 1.0f;
        if (fadeMs_ > 0 && displayMs_ > 0) {
            const float fade = static_cast<float>(fadeMs_);
            const float since = static_cast<float>(elapsed - static_cast<Uint32>(delayMs_));
            const float remaining = static_cast<float>(totalMs - elapsed);
            opacity = std::clamp(std::min(since / fade, remaining / fade), 0.0f, 1.0f);
        }

        const SDL_Rect card = getCardRect(renderer);

        renderer.drawShadow(
            card,
            borderRadius_,
            withOpacity(shadow_.color, opacity),
            shadow_.offsetX,
            shadow_.offsetY,
            shadow_.spread
        );
        renderer.fillRoundedRect(card, borderRadius_, withOpacity(backgroundColor_, opacity));

        for (int inset = 0; inset < borderWidth_; ++inset) {
            const SDL_Rect border{
                card.x + inset,
                card.y + inset,
                card.w - 2 * inset,
                card.h - 2 * inset
            };

            if (border.w <= 0 || border.h <= 0) {
                break;
            }

            renderer.drawRoundedRect(
                border,
                std::max(0, borderRadius_ - inset),
                withOpacity(borderColor_, opacity)
            );
        }

        const std::vector<std::string> lines = splitLines(text_);
        const int lineHeight = std::max(1, fontSpec_.size + 4);
        const SDL_Color foreground = withOpacity(textColor_, opacity);

        int lineY = card.y + borderWidth_ + padding_.top;
        for (const std::string& line : lines) {
            renderer.drawText(
                line,
                {card.x + borderWidth_ + padding_.left, lineY},
                foreground,
                fontSpec_
            );
            lineY += lineHeight;
        }
    }

    void Tooltip::onMouseEvent(EventMouse& event) {
        if (!shown_ || window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        if (event.getButton() != SDL_BUTTON_LEFT || !event.isPressed()) {
            return;
        }

        const SDL_Rect card = getCardRect(window_->getRendererOpts());

        if (!containsPoint(card, event.getX(), event.getY())) {
            hide();
        }
    }

    void Tooltip::onKeyboardEvent(EventKeyboard& event) {
        if (event.isPressed() && event.getKey() == SDLK_ESCAPE && shown_) {
            hide();
        }
    }

} // emgui

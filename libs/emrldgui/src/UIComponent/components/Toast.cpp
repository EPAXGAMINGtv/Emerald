// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/Toast.h"

#include "emrldgui/Window.h"
#include "emrldgui/style/StyleValues.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace emgui {

    namespace {

        std::vector<std::string> wrapText(
            RendererOpts& renderer,
            const std::string& text,
            int maxWidth,
            const FontSpec& fontSpec,
            int maxLines
        ) {
            std::vector<std::string> lines;

            if (text.empty()) {
                return lines;
            }

            const int width = std::max(1, maxWidth);
            const int limit = std::max(1, maxLines);

            const auto fits = [&renderer, &fontSpec, width](const std::string& value) {
                return renderer.measureText(value, fontSpec).x <= width;
            };

            std::size_t paragraphStart = 0;
            while (true) {
                const std::string::size_type newline = text.find('\n', paragraphStart);
                const std::string paragraph = text.substr(
                    paragraphStart,
                    newline == std::string::npos ? std::string::npos : newline - paragraphStart
                );

                std::string line;
                std::size_t position = 0;

                while (position < paragraph.size()) {
                    const std::size_t space = paragraph.find_first_of(" \t", position);
                    const std::string word = paragraph.substr(
                        position,
                        space == std::string::npos ? std::string::npos : space - position
                    );
                    const std::string candidate = line.empty() ? word : line + " " + word;

                    if (fits(candidate)) {
                        line = candidate;
                    } else {
                        if (!line.empty()) {
                            lines.push_back(line);
                            line.clear();
                        }

                        if (fits(word)) {
                            line = word;
                        } else {
                            std::string chunk;
                            for (const char character : word) {
                                const std::string next = chunk + character;
                                if (!chunk.empty() && !fits(next)) {
                                    lines.push_back(chunk);
                                    chunk.clear();
                                }
                                chunk += character;
                            }
                            line = chunk;
                        }
                    }

                    if (space == std::string::npos) {
                        break;
                    }

                    position = paragraph.find_first_not_of(" \t", space);
                    if (position == std::string::npos) {
                        break;
                    }
                }

                lines.push_back(line);

                if (newline == std::string::npos) {
                    break;
                }
                paragraphStart = newline + 1;
            }

            if (static_cast<int>(lines.size()) > limit) {
                lines.resize(static_cast<std::size_t>(limit));

                std::string& last = lines.back();
                while (!last.empty() && !fits(last + "...")) {
                    last.pop_back();
                }
                last += "...";
            }

            return lines;
        }

    } // namespace

    ToastManager::ToastManager(int x, int y, int width, int height)
        : UIComponent(x, y),
          width_(width),
          height_(height) {}

    void ToastManager::applyStyle(const StyleValues& values) {
        backgroundColor_ = values.getColor("background-color", backgroundColor_);
        borderColor_ = values.getColor("border-color", borderColor_);
        titleColor_ = values.getColor("title-color", titleColor_);
        textColor_ = values.getColor("text-color", textColor_);
        progressColor_ = values.getColor("progress-color", progressColor_);

        borderWidth_ = std::max(0, values.getInt("border-width", borderWidth_));
        borderRadius_ = std::max(0, values.getInt("border-radius", borderRadius_));

        width_ = std::max(0, values.getInt("width", width_));
        height_ = std::max(0, values.getInt("height", height_));
        gap_ = std::max(0, values.getInt("gap", gap_));
        maxLines_ = std::max(1, values.getInt("max-lines", maxLines_));
        progressHeight_ = std::max(0, values.getInt("progress-height", progressHeight_));

        const int duration = std::max(0, values.getInt("duration-ms", 3000));
        defaultDurationMs_ = static_cast<Uint32>(duration);

        bottomUp_ = lowerCase(values.getString("direction", "top-down")) == "bottom-up";

        opacity_ = std::clamp(values.getFloat("opacity", opacity_), 0.0f, 1.0f);

        padding_.apply(values, 12);
        shadow_.apply(values);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        titleFontSpec_.family = fontSpec_.family;
        titleFontSpec_.style = fontSpec_.style;
        titleFontSpec_.size = std::max(
            1,
            values.getInt("title-font-size", titleFontSpec_.size)
        );
    }

    void ToastManager::show(std::string message, Uint32 durationMs) {
        Toast toast;
        toast.title.clear();
        toast.message = std::move(message);
        toast.durationMs = durationMs;
        toast.startTicks = SDL_GetTicks();
        toasts_.push_back(std::move(toast));
    }

    void ToastManager::show(std::string title, std::string message, Uint32 durationMs) {
        Toast toast;
        toast.title = std::move(title);
        toast.message = std::move(message);
        toast.durationMs = durationMs;
        toast.startTicks = SDL_GetTicks();
        toasts_.push_back(std::move(toast));
    }

    void ToastManager::clear() {
        toasts_.clear();
    }

    void ToastManager::update() {
        const Uint32 now = SDL_GetTicks();

        // Unsigned subtraction keeps this correct across a SDL_GetTicks wrap.
        toasts_.erase(
            std::remove_if(
                toasts_.begin(),
                toasts_.end(),
                [now](const Toast& toast) {
                    return toast.durationMs > 0 &&
                           now - toast.startTicks >= toast.durationMs;
                }
            ),
            toasts_.end()
        );
    }

    SDL_Rect ToastManager::getViewportRect(RendererOpts& renderer) const {
        int viewWidth = 0;
        int viewHeight = 0;
        SDL_Renderer* sdlRenderer = renderer.getRenderer();

        if (sdlRenderer != nullptr) {
            SDL_RenderGetLogicalSize(sdlRenderer, &viewWidth, &viewHeight);
        }

        if (viewWidth <= 0 || viewHeight <= 0) {
            viewWidth = 0;
            viewHeight = 0;

            if (window_ != nullptr) {
                viewWidth = window_->getWidth();
                viewHeight = window_->getHeight();
            }

            if ((viewWidth <= 0 || viewHeight <= 0) && sdlRenderer != nullptr) {
                SDL_GetRendererOutputSize(sdlRenderer, &viewWidth, &viewHeight);
            }
        }

        return {0, 0, std::max(0, viewWidth), std::max(0, viewHeight)};
    }

    void ToastManager::render(RendererOpts& renderer) {
        (void)renderer;
    }

    void ToastManager::renderOverlay(RendererOpts& renderer) {
        if (toasts_.empty()) {
            return;
        }

        const SDL_Rect viewport = getViewportRect(renderer);
        if (viewport.w <= 0 || viewport.h <= 0) {
            return;
        }

        const int cardWidth = viewport.w > 0
            ? std::min(std::max(1, width_), std::max(80, viewport.w - 16))
            : std::max(1, width_);
        const int textWidth = std::max(
            1,
            cardWidth - padding_.horizontal() - 2 * borderWidth_
        );
        const int lineHeight = std::max(1, fontSpec_.size + 4);
        const Uint32 now = SDL_GetTicks();

        int offset = 0;

        for (const Toast& toast : toasts_) {
            const std::vector<std::string> lines = wrapText(
                renderer, toast.message, textWidth, fontSpec_, maxLines_
            );

            const int titleHeight = toast.title.empty()
                ? 0
                : std::max(1, titleFontSpec_.size + 4);
            const int progressHeight = toast.durationMs > 0 ? std::max(0, progressHeight_) : 0;
            const int progressGap = progressHeight > 0 ? 6 : 0;

            int cardHeight = padding_.vertical() + 2 * borderWidth_ + titleHeight;
            if (!toast.title.empty() && !lines.empty()) {
                cardHeight += gap_;
            }
            cardHeight += static_cast<int>(lines.size()) * lineHeight;
            if (progressHeight > 0) {
                cardHeight += progressGap + progressHeight;
            }
            cardHeight = std::max(1, cardHeight);

            if (height_ > 0 && offset + cardHeight > height_) {
                break;
            }

            const int cardX = viewport.w > 0
                ? std::clamp(x_, 0, std::max(0, viewport.w - cardWidth))
                : x_;
            int cardY = bottomUp_ ? y_ - offset - cardHeight : y_ + offset;
            if (viewport.h > 0) {
                cardY = std::clamp(cardY, 0, std::max(0, viewport.h - cardHeight));
            }

            const int radius = borderRadius_;
            const SDL_Rect card{cardX, cardY, cardWidth, cardHeight};

            renderer.drawShadow(
                card,
                radius,
                withOpacity(shadow_.color, opacity_),
                shadow_.offsetX,
                shadow_.offsetY,
                shadow_.spread
            );

            renderer.fillRoundedRect(
                card,
                radius,
                withOpacity(backgroundColor_, opacity_)
            );

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
                    std::max(0, radius - inset),
                    withOpacity(borderColor_, opacity_)
                );
            }

            int cursorY = card.y + borderWidth_ + padding_.top;
            const int textX = card.x + borderWidth_ + padding_.left;

            if (!toast.title.empty()) {
                renderer.drawText(
                    toast.title,
                    {textX, cursorY},
                    withOpacity(titleColor_, opacity_),
                    titleFontSpec_
                );
                cursorY += titleHeight;
                if (!lines.empty()) {
                    cursorY += gap_;
                }
            }

            for (const std::string& line : lines) {
                renderer.drawText(
                    line,
                    {textX, cursorY},
                    withOpacity(textColor_, opacity_),
                    fontSpec_
                );
                cursorY += lineHeight;
            }

            if (progressHeight > 0) {
                const int barY = card.y + card.h - borderWidth_ - padding_.bottom -
                                 progressHeight;
                const SDL_Rect track{
                    textX,
                    barY,
                    std::max(1, cardWidth - padding_.horizontal() - 2 * borderWidth_),
                    progressHeight
                };
                renderer.fillRoundedRect(
                    track,
                    std::min(radius, progressHeight / 2),
                    withOpacity(progressColor_, opacity_ * 0.25f)
                );

                if (toast.durationMs > 0 && now >= toast.startTicks) {
                    const Uint32 elapsed = now - toast.startTicks;
                    const float remaining = elapsed >= toast.durationMs
                        ? 0.0f
                        : 1.0f - static_cast<float>(elapsed) /
                                 static_cast<float>(toast.durationMs);
                    const int barWidth = static_cast<int>(
                        std::lround(static_cast<float>(track.w) * remaining)
                    );

                    if (barWidth > 0) {
                        renderer.fillRoundedRect(
                            {track.x, track.y, std::min(barWidth, track.w), track.h},
                            std::min(radius, progressHeight / 2),
                            withOpacity(progressColor_, opacity_)
                        );
                    }
                }
            }

            offset += cardHeight + gap_;
        }
    }

} // emgui

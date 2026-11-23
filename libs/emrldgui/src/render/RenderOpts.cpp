// Created by epax on 21.11.26.

#include "emrldgui/render/RendererOpts.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "spdlog/spdlog.h"

namespace emgui {

    namespace {

        // Draw calls set their own color and restore the previous one afterwards
        class DrawColorGuard {
        public:
            DrawColorGuard(SDL_Renderer* renderer, const SDL_Color& color)
                : renderer_(renderer), previous_{0, 0, 0, 255} {
                if (renderer_ == nullptr) {
                    return;
                }

                SDL_GetRenderDrawColor(
                    renderer_, &previous_.r, &previous_.g, &previous_.b, &previous_.a
                );
                SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
            }

            ~DrawColorGuard() {
                if (renderer_ != nullptr) {
                    SDL_SetRenderDrawColor(
                        renderer_, previous_.r, previous_.g, previous_.b, previous_.a
                    );
                }
            }

            DrawColorGuard(const DrawColorGuard&) = delete;
            DrawColorGuard& operator=(const DrawColorGuard&) = delete;

        private:
            SDL_Renderer* renderer_ = nullptr;
            SDL_Color previous_{};
        };

        SDL_Color mixColor(const SDL_Color& from, const SDL_Color& to, float amount) {
            const float t = std::clamp(amount, 0.0f, 1.0f);

            const auto channel = [t](Uint8 a, Uint8 b) {
                const float value =
                    static_cast<float>(a) * (1.0f - t) +
                    static_cast<float>(b) * t;
                return static_cast<Uint8>(std::lround(value));
            };

            return {
                channel(from.r, to.r),
                channel(from.g, to.g),
                channel(from.b, to.b),
                channel(from.a, to.a)
            };
        }

        CornerRadii clampRadii(const SDL_Rect& rect, CornerRadii radii) {
            const int maxRadius = std::min(rect.w, rect.h) / 2;

            radii.topLeft = std::clamp(radii.topLeft, 0, maxRadius);
            radii.topRight = std::clamp(radii.topRight, 0, maxRadius);
            radii.bottomRight = std::clamp(radii.bottomRight, 0, maxRadius);
            radii.bottomLeft = std::clamp(radii.bottomLeft, 0, maxRadius);

            return radii;
        }

        // Inset of one corner arc on a pixel row, measured from the outer edge.
        int cornerInset(
            int row,
            int radius,
            int width,
            int height,
            bool top,
            bool leftSide
        ) {
            if (radius <= 0) {
                return 0;
            }

            // A corner only owns the rows on its own side of the rectangle.
            // The circle equation alone also matches rows past the corner
            // center, so the arc bends back in after reaching the straight
            // edge and leaves a notch/crack in the border.
            if (top ? row >= radius : row < height - radius) {
                return 0;
            }

            const double centerX = leftSide
                ? static_cast<double>(radius)
                : static_cast<double>(width - radius);
            const double centerY = top
                ? static_cast<double>(radius)
                : static_cast<double>(height - radius);
            const double distanceY = std::abs(
                (static_cast<double>(row) + 0.5) - centerY
            );

            if (distanceY >= radius) {
                return 0;
            }

            const double halfSpan = std::sqrt(
                static_cast<double>(radius) * static_cast<double>(radius) -
                distanceY * distanceY
            );

            if (leftSide) {
                const int column = static_cast<int>(
                    std::ceil(centerX - halfSpan - 0.5)
                );
                return std::clamp(column, 0, radius);
            }

            const int column = static_cast<int>(
                std::floor(centerX + halfSpan - 0.5)
            );
            return std::clamp(width - 1 - column, 0, radius);
        }

        // Returns how far the left and right edges move inward on one row.
        std::pair<int, int> rowInsets(
            const SDL_Rect& rect,
            const CornerRadii& radii,
            int row
        ) {
            int left = std::max(
                cornerInset(row, radii.topLeft, rect.w, rect.h, true, true),
                cornerInset(row, radii.bottomLeft, rect.w, rect.h, false, true)
            );
            int right = std::max(
                cornerInset(row, radii.topRight, rect.w, rect.h, true, false),
                cornerInset(row, radii.bottomRight, rect.w, rect.h, false, false)
            );

            if (left + right > rect.w) {
                const int excess = left + right - rect.w;
                left -= excess / 2;
                right -= excess - excess / 2;
            }

            return {left, right};
        }

        SDL_Color sampleGradient(const std::vector<SDL_Color>& stops, float position) {
            if (stops.empty()) {
                return {0, 0, 0, 0};
            }

            if (stops.size() == 1) {
                return stops.front();
            }

            const float scaled = std::clamp(position, 0.0f, 1.0f) *
                                 static_cast<float>(stops.size() - 1);
            const std::size_t index = std::min(
                static_cast<std::size_t>(scaled),
                stops.size() - 2
            );
            const float amount = scaled - static_cast<float>(index);
            return mixColor(stops[index], stops[index + 1], amount);
        }

    } // namespace

    RendererOpts::RendererOpts(RenderContext& context, int fontSize, std::string fontPath)
        : context_(context) {
        // An explicit path is treated like a font family, ResourceManager resolves res/font/...
        defaultFontSpec_.family = fontPath.empty() ? "LiberationSans" : std::move(fontPath);
        defaultFontSpec_.size = fontSize;

        // -1 selects the default renderer, 0 means no special flags
        renderer_ = SDL_CreateRenderer(context_.getWindow(), -1, 0);

        if (renderer_ == nullptr) {
            spdlog::error("SDL_CreateRenderer failed: {}", SDL_GetError());
            return;
        }

        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    }

    RendererOpts::~RendererOpts() {
        // Textures belong to this renderer, so they must die before it
        clearTextCache();

        if (renderer_ != nullptr) {
            SDL_DestroyRenderer(renderer_);
        }
    }

    void RendererOpts::clearTextCache() {
        for (const auto& entry : textCache_) {
            if (entry.second.texture != nullptr) {
                SDL_DestroyTexture(entry.second.texture);
            }
        }
        textCache_.clear();
    }

    void RendererOpts::drawRect(const SDL_Rect& rect, const SDL_Color& color) {
        if (renderer_ == nullptr) {
            return;
        }

        DrawColorGuard colorGuard(renderer_, color);
        SDL_RenderDrawRect(renderer_, &rect);
    }

    void RendererOpts::fillRect(const SDL_Rect& rect, const SDL_Color& color) {
        if (renderer_ == nullptr) {
            return;
        }

        DrawColorGuard colorGuard(renderer_, color);
        SDL_RenderFillRect(renderer_, &rect);
    }

    void RendererOpts::drawPoint(const SDL_Point& point, const SDL_Color& color) {
        if (renderer_ == nullptr) {
            return;
        }

        DrawColorGuard colorGuard(renderer_, color);
        SDL_RenderDrawPoint(renderer_, point.x, point.y);
    }

    void RendererOpts::drawLine(
        const SDL_Point& start,
        const SDL_Point& end,
        const SDL_Color& color
    ) {
        if (renderer_ == nullptr) {
            return;
        }

        DrawColorGuard colorGuard(renderer_, color);
        SDL_RenderDrawLine(renderer_, start.x, start.y, end.x, end.y);
    }

    void RendererOpts::drawCircle(
        const SDL_Point& center,
        int radius,
        const SDL_Color& color
    ) {
        if (renderer_ == nullptr || radius <= 0) {
            return;
        }

        DrawColorGuard colorGuard(renderer_, color);

        // Midpoint circle algorithm
        int x = radius;
        int y = 0;
        int error = 1 - radius;

        while (x >= y) {
            SDL_RenderDrawPoint(renderer_, center.x + x, center.y + y);
            SDL_RenderDrawPoint(renderer_, center.x - x, center.y + y);
            SDL_RenderDrawPoint(renderer_, center.x + x, center.y - y);
            SDL_RenderDrawPoint(renderer_, center.x - x, center.y - y);
            SDL_RenderDrawPoint(renderer_, center.x + y, center.y + x);
            SDL_RenderDrawPoint(renderer_, center.x - y, center.y + x);
            SDL_RenderDrawPoint(renderer_, center.x + y, center.y - x);
            SDL_RenderDrawPoint(renderer_, center.x - y, center.y - x);

            ++y;
            if (error < 0) {
                error += 2 * y + 1;
            } else {
                --x;
                error += 2 * (x - y) + 1;
            }
        }
    }

    void RendererOpts::fillCircle(
        const SDL_Point& center,
        int radius,
        const SDL_Color& color
    ) {
        if (renderer_ == nullptr || radius <= 0) {
            return;
        }

        DrawColorGuard colorGuard(renderer_, color);

        const double radiusSquared = static_cast<double>(radius) * static_cast<double>(radius);

        for (int offsetY = -radius; offsetY <= radius; ++offsetY) {
            const double squaredY =
                static_cast<double>(offsetY) * static_cast<double>(offsetY);
            const int halfWidth = static_cast<int>(std::sqrt(radiusSquared - squaredY));

            SDL_Rect row{
                center.x - halfWidth,
                center.y + offsetY,
                2 * halfWidth + 1,
                1
            };
            SDL_RenderFillRect(renderer_, &row);
        }
    }

    void RendererOpts::drawRoundedRect(
        const SDL_Rect& rect,
        int radius,
        const SDL_Color& color
    ) {
        drawRoundedRect(rect, CornerRadii{radius, radius, radius, radius}, color);
    }

    void RendererOpts::fillRoundedRect(
        const SDL_Rect& rect,
        int radius,
        const SDL_Color& color
    ) {
        fillRoundedRect(rect, CornerRadii{radius, radius, radius, radius}, color);
    }

    void RendererOpts::drawRoundedRect(
        const SDL_Rect& rect,
        const CornerRadii& radii,
        const SDL_Color& color
    ) {
        if (renderer_ == nullptr || rect.w <= 0 || rect.h <= 0) {
            return;
        }

        const CornerRadii clamped = clampRadii(rect, radii);

        for (int row = 0; row < rect.h; ++row) {
            const auto insets = rowInsets(rect, clamped, row);
            const int span = rect.w - insets.first - insets.second;

            if (span <= 0) {
                continue;
            }

            const SDL_Point left{rect.x + insets.first, rect.y + row};
            const SDL_Point right{
                rect.x + rect.w - 1 - insets.second,
                rect.y + row
            };

            if (row == 0 || row == rect.h - 1 || span <= 2) {
                drawLine(left, right, color);
            } else {
                drawPoint(left, color);
                drawPoint(right, color);
            }
        }
    }

    void RendererOpts::fillRoundedRect(
        const SDL_Rect& rect,
        const CornerRadii& radii,
        const SDL_Color& color
    ) {
        if (renderer_ == nullptr || rect.w <= 0 || rect.h <= 0) {
            return;
        }

        const CornerRadii clamped = clampRadii(rect, radii);

        for (int row = 0; row < rect.h; ++row) {
            const auto insets = rowInsets(rect, clamped, row);
            const int span = rect.w - insets.first - insets.second;

            if (span > 0) {
                fillRect(
                    {rect.x + insets.first, rect.y + row, span, 1},
                    color
                );
            }
        }
    }

    void RendererOpts::fillRoundedRectGradient(
        const SDL_Rect& rect,
        const CornerRadii& radii,
        const std::vector<SDL_Color>& stops,
        bool horizontal
    ) {
        if (renderer_ == nullptr || rect.w <= 0 || rect.h <= 0 || stops.empty()) {
            return;
        }

        const CornerRadii clamped = clampRadii(rect, radii);
        const float verticalDenominator = rect.h > 1
            ? static_cast<float>(rect.h - 1)
            : 1.0f;
        const float horizontalDenominator = rect.w > 1
            ? static_cast<float>(rect.w - 1)
            : 1.0f;

        for (int row = 0; row < rect.h; ++row) {
            const auto insets = rowInsets(rect, clamped, row);
            const int span = rect.w - insets.first - insets.second;

            if (span <= 0) {
                continue;
            }

            if (horizontal) {
                for (int column = 0; column < span; ++column) {
                    const float position = static_cast<float>(column) /
                                           horizontalDenominator;
                    fillRect(
                        {rect.x + insets.first + column, rect.y + row, 1, 1},
                        sampleGradient(stops, position)
                    );
                }
            } else {
                const float position = static_cast<float>(row) / verticalDenominator;
                fillRect(
                    {rect.x + insets.first, rect.y + row, span, 1},
                    sampleGradient(stops, position)
                );
            }
        }
    }

    void RendererOpts::fillGradientRect(
        const SDL_Rect& rect,
        const SDL_Color& top,
        const SDL_Color& bottom
    ) {
        if (renderer_ == nullptr || rect.w <= 0 || rect.h <= 0) {
            return;
        }

        const float denominator = rect.h > 1 ? static_cast<float>(rect.h - 1) : 1.0f;

        for (int row = 0; row < rect.h; ++row) {
            const float t = static_cast<float>(row) / denominator;
            fillRect(
                {rect.x, rect.y + row, rect.w, 1},
                mixColor(top, bottom, t)
            );
        }
    }

    void RendererOpts::fillRoundedRectGradient(
        const SDL_Rect& rect,
        int radius,
        const SDL_Color& top,
        const SDL_Color& bottom
    ) {
        fillRoundedRectGradient(
            rect,
            CornerRadii{radius, radius, radius, radius},
            {top, bottom},
            false
        );
    }

    void RendererOpts::drawShadow(
        const SDL_Rect& rect,
        int radius,
        const SDL_Color& color,
        int offsetX,
        int offsetY,
        int spread
    ) {
        if (renderer_ == nullptr || color.a == 0 ||
            (offsetX == 0 && offsetY == 0 && spread == 0)) {
            return;
        }

        fillRoundedRect(
            {
                rect.x + offsetX - spread,
                rect.y + offsetY - spread,
                rect.w + 2 * spread,
                rect.h + 2 * spread
            },
            radius + spread,
            color
        );
    }

    void RendererOpts::drawTexture(SDL_Texture* texture, const SDL_Rect& destination) {
        if (renderer_ == nullptr || texture == nullptr) {
            return;
        }

        SDL_RenderCopy(renderer_, texture, nullptr, &destination);
    }

    void RendererOpts::drawText(
        const std::string& text,
        const SDL_Point& position,
        const SDL_Color& color
    ) {
        drawText(text, position, color, defaultFontSpec_);
    }

    void RendererOpts::drawText(
        const std::string& text,
        const SDL_Point& position,
        const SDL_Color& color,
        const FontSpec& fontSpec
    ) {
        if (renderer_ == nullptr || text.empty()) {
            return;
        }

        Font* font = fontManager_.getFont(fontSpec);

        if (font == nullptr || !font->isValid()) {
            return;
        }

        // Text, color and font settings together identify one cached texture
        const TextCacheKey key{
            text,
            color.r, color.g, color.b, color.a,
            fontSpec.family, fontSpec.size, fontSpec.style
        };
        auto cached = textCache_.find(key);

        if (cached == textCache_.end()) {
            SDL_Surface* surface = font->renderSurface(text, color);

            if (surface == nullptr) {
                return;
            }

            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);

            if (texture == nullptr) {
                spdlog::error("SDL_CreateTextureFromSurface failed: {}", SDL_GetError());
                SDL_FreeSurface(surface);
                return;
            }

            const CachedText entry{texture, surface->w, surface->h};
            SDL_FreeSurface(surface);

            cached = textCache_.emplace(key, entry).first;
        }

        const SDL_Rect destination = {
            position.x,
            position.y,
            cached->second.width,
            cached->second.height
        };

        SDL_RenderCopy(renderer_, cached->second.texture, nullptr, &destination);
    }

    SDL_Point RendererOpts::measureText(
        const std::string& text,
        const FontSpec& fontSpec
    ) const {
        SDL_Point size{0, 0};

        if (text.empty()) {
            return size;
        }

        Font* font = fontManager_.getFont(fontSpec);

        if (font == nullptr || !font->isValid()) {
            return size;
        }

        int width = 0;
        int height = 0;

        if (font->measure(text, width, height)) {
            size.x = width;
            size.y = height;
        }

        return size;
    }

    bool RendererOpts::isFontValid() const {
        return fontManager_.getFont(defaultFontSpec_)->isValid();
    }

    void RendererOpts::clear() {
        clear(clearColor_);
    }

    void RendererOpts::clear(const SDL_Color& color) {
        if (renderer_ == nullptr) {
            return;
        }

        DrawColorGuard colorGuard(renderer_, color);

        if (SDL_RenderClear(renderer_) != 0) {
            spdlog::error("SDL_RenderClear failed: {}", SDL_GetError());
        }
    }

    void RendererOpts::present() {
        if (renderer_ != nullptr) {
            SDL_RenderPresent(renderer_);
        }
    }

    RendererOpts::Frame RendererOpts::beginFrame() {
        clear();
        return Frame(*this);
    }

    RendererOpts::Frame RendererOpts::beginFrame(const SDL_Color& clearColor) {
        clear(clearColor);
        return Frame(*this);
    }

    RendererOpts::Frame::~Frame() {
        opts_.present();
    }

    void RendererOpts::Frame::drawRect(const SDL_Rect& rect, const SDL_Color& color) {
        opts_.drawRect(rect, color);
    }

    void RendererOpts::Frame::fillRect(const SDL_Rect& rect, const SDL_Color& color) {
        opts_.fillRect(rect, color);
    }

    void RendererOpts::Frame::drawPoint(const SDL_Point& point, const SDL_Color& color) {
        opts_.drawPoint(point, color);
    }

    void RendererOpts::Frame::drawLine(
        const SDL_Point& start,
        const SDL_Point& end,
        const SDL_Color& color
    ) {
        opts_.drawLine(start, end, color);
    }

    void RendererOpts::Frame::drawCircle(
        const SDL_Point& center,
        int radius,
        const SDL_Color& color
    ) {
        opts_.drawCircle(center, radius, color);
    }

    void RendererOpts::Frame::fillCircle(
        const SDL_Point& center,
        int radius,
        const SDL_Color& color
    ) {
        opts_.fillCircle(center, radius, color);
    }

    void RendererOpts::Frame::drawRoundedRect(
        const SDL_Rect& rect,
        int radius,
        const SDL_Color& color
    ) {
        opts_.drawRoundedRect(rect, radius, color);
    }

    void RendererOpts::Frame::fillRoundedRect(
        const SDL_Rect& rect,
        int radius,
        const SDL_Color& color
    ) {
        opts_.fillRoundedRect(rect, radius, color);
    }

    void RendererOpts::Frame::drawRoundedRect(
        const SDL_Rect& rect,
        const CornerRadii& radii,
        const SDL_Color& color
    ) {
        opts_.drawRoundedRect(rect, radii, color);
    }

    void RendererOpts::Frame::fillRoundedRect(
        const SDL_Rect& rect,
        const CornerRadii& radii,
        const SDL_Color& color
    ) {
        opts_.fillRoundedRect(rect, radii, color);
    }

    void RendererOpts::Frame::fillRoundedRectGradient(
        const SDL_Rect& rect,
        const CornerRadii& radii,
        const std::vector<SDL_Color>& stops,
        bool horizontal
    ) {
        opts_.fillRoundedRectGradient(rect, radii, stops, horizontal);
    }

    void RendererOpts::Frame::fillGradientRect(
        const SDL_Rect& rect,
        const SDL_Color& top,
        const SDL_Color& bottom
    ) {
        opts_.fillGradientRect(rect, top, bottom);
    }

    void RendererOpts::Frame::fillRoundedRectGradient(
        const SDL_Rect& rect,
        int radius,
        const SDL_Color& top,
        const SDL_Color& bottom
    ) {
        opts_.fillRoundedRectGradient(rect, radius, top, bottom);
    }

    void RendererOpts::Frame::drawShadow(
        const SDL_Rect& rect,
        int radius,
        const SDL_Color& color,
        int offsetX,
        int offsetY,
        int spread
    ) {
        opts_.drawShadow(rect, radius, color, offsetX, offsetY, spread);
    }

    void RendererOpts::Frame::drawTexture(SDL_Texture* texture, const SDL_Rect& destination) {
        opts_.drawTexture(texture, destination);
    }

    void RendererOpts::Frame::drawText(
        const std::string& text,
        const SDL_Point& position,
        const SDL_Color& color
    ) {
        opts_.drawText(text, position, color);
    }

    void RendererOpts::Frame::drawText(
        const std::string& text,
        const SDL_Point& position,
        const SDL_Color& color,
        const FontSpec& fontSpec
    ) {
        opts_.drawText(text, position, color, fontSpec);
    }

} // emgui

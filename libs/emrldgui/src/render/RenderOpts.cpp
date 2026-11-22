//
// Created by epax on 21.11.26.
//

#include "emrldgui/render/RendererOpts.h"

#include <cmath>
#include <utility>

#include "spdlog/spdlog.h"

namespace emgui {

    namespace {

        //draw calls set their own color and restore the previous one afterwards ->
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

    } // namespace

    RendererOpts::RendererOpts(RenderContext& context, int fontSize, std::string fontPath)
        : context_(context), font_(fontSize, std::move(fontPath)) {
        //-1 selects the default renderer, 0 means no special flags ->
        renderer_ = SDL_CreateRenderer(context_.getWindow(), -1, 0);

        if (renderer_ == nullptr) {
            spdlog::error("SDL_CreateRenderer failed: {}", SDL_GetError());
        }
    }

    RendererOpts::~RendererOpts() {
        if (renderer_ != nullptr) {
            SDL_DestroyRenderer(renderer_);
        }
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

        //midpoint circle algorithm ->
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
        font_.draw(renderer_, text, position, color);
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

} // emgui

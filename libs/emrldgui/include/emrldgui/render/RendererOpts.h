// Created by epax on 21.11.26.
#pragma once
#include <SDL.h>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "RenderContext.h"
#include "emrldgui/resource/FontManager.h"

namespace emgui {

    // Per-corner radii, all values in pixels
    struct CornerRadii {
        int topLeft = 0;
        int topRight = 0;
        int bottomRight = 0;
        int bottomLeft = 0;
    };

    class RendererOpts {
    public:
        // Small RAII helper: clears at the beginning, presents at the end of the scope
        class Frame {
        public:
            ~Frame();

            Frame(const Frame&) = delete;
            Frame& operator=(const Frame&) = delete;
            Frame(Frame&&) = delete;
            Frame& operator=(Frame&&) = delete;

            // Draw functions forwarded from RendererOpts
            void drawRect(const SDL_Rect& rect, const SDL_Color& color);
            void fillRect(const SDL_Rect& rect, const SDL_Color& color);
            void drawPoint(const SDL_Point& point, const SDL_Color& color);
            void drawLine(const SDL_Point& start, const SDL_Point& end, const SDL_Color& color);
            void drawCircle(const SDL_Point& center, int radius, const SDL_Color& color);
            void fillCircle(const SDL_Point& center, int radius, const SDL_Color& color);
            void drawRoundedRect(const SDL_Rect& rect, int radius, const SDL_Color& color);
            void fillRoundedRect(const SDL_Rect& rect, int radius, const SDL_Color& color);
            void drawRoundedRect(
                const SDL_Rect& rect,
                const CornerRadii& radii,
                const SDL_Color& color
            );
            void fillRoundedRect(
                const SDL_Rect& rect,
                const CornerRadii& radii,
                const SDL_Color& color
            );
            void fillRoundedRectGradient(
                const SDL_Rect& rect,
                const CornerRadii& radii,
                const std::vector<SDL_Color>& stops,
                bool horizontal
            );
            void fillGradientRect(
                const SDL_Rect& rect,
                const SDL_Color& top,
                const SDL_Color& bottom
            );
            void fillRoundedRectGradient(
                const SDL_Rect& rect,
                int radius,
                const SDL_Color& top,
                const SDL_Color& bottom
            );
            void drawShadow(
                const SDL_Rect& rect,
                int radius,
                const SDL_Color& color,
                int offsetX,
                int offsetY,
                int spread = 0
            );
            void drawTexture(SDL_Texture* texture, const SDL_Rect& destination);
            void drawText(const std::string& text, const SDL_Point& position, const SDL_Color& color);
            void drawText(
                const std::string& text,
                const SDL_Point& position,
                const SDL_Color& color,
                const FontSpec& fontSpec
            );

        private:
            friend class RendererOpts;
            explicit Frame(RendererOpts& opts) : opts_(opts) {}

            RendererOpts& opts_;
        };

        // FontPath empty selects a bundled/system TTF font
        explicit RendererOpts(
            RenderContext& context,
            int fontSize = 24,
            std::string fontPath = {}
        );
        ~RendererOpts();

        RendererOpts(const RendererOpts&) = delete;
        RendererOpts& operator=(const RendererOpts&) = delete;

        // Clear color helpers
        void setClearColor(const SDL_Color& color) {
            clearColor_ = color;
        }

        const SDL_Color& getClearColor() const {
            return clearColor_;
        }

        // Draw functions
        void drawRect(const SDL_Rect& rect, const SDL_Color& color);
        void fillRect(const SDL_Rect& rect, const SDL_Color& color);
        void drawPoint(const SDL_Point& point, const SDL_Color& color);
        void drawLine(const SDL_Point& start, const SDL_Point& end, const SDL_Color& color);
        void drawCircle(const SDL_Point& center, int radius, const SDL_Color& color);
        void fillCircle(const SDL_Point& center, int radius, const SDL_Color& color);
        void drawRoundedRect(const SDL_Rect& rect, int radius, const SDL_Color& color);
        void fillRoundedRect(const SDL_Rect& rect, int radius, const SDL_Color& color);
        void drawRoundedRect(
            const SDL_Rect& rect,
            const CornerRadii& radii,
            const SDL_Color& color
        );
        void fillRoundedRect(
            const SDL_Rect& rect,
            const CornerRadii& radii,
            const SDL_Color& color
        );
        void fillRoundedRectGradient(
            const SDL_Rect& rect,
            const CornerRadii& radii,
            const std::vector<SDL_Color>& stops,
            bool horizontal
        );
        void fillGradientRect(
            const SDL_Rect& rect,
            const SDL_Color& top,
            const SDL_Color& bottom
        );
        void fillRoundedRectGradient(
            const SDL_Rect& rect,
            int radius,
            const SDL_Color& top,
            const SDL_Color& bottom
        );
        void drawShadow(
            const SDL_Rect& rect,
            int radius,
            const SDL_Color& color,
            int offsetX,
            int offsetY,
            int spread = 0
        );
        void drawTexture(SDL_Texture* texture, const SDL_Rect& destination);
        void drawText(const std::string& text, const SDL_Point& position, const SDL_Color& color);
        void drawText(
            const std::string& text,
            const SDL_Point& position,
            const SDL_Color& color,
            const FontSpec& fontSpec
        );

        SDL_Point measureText(const std::string& text, const FontSpec& fontSpec) const;

        // Old typo kept as an alias, use drawLine instead
        void drawlLine(const SDL_Point& start, const SDL_Point& end, const SDL_Color& color) {
            drawLine(start, end, color);
        }

        // Clear() always uses clearColor_, so draw colors cannot leak onto the whole screen
        void clear();
        void clear(const SDL_Color& color);

        // Put the rendered backbuffer on the window
        void present();

        // Frame helpers: clear at start, present when the Frame goes out of scope
        Frame beginFrame();
        Frame beginFrame(const SDL_Color& clearColor);

        SDL_Renderer* getRenderer() const {
            return renderer_;
        }

        bool isFontValid() const;

    private:
        using TextCacheKey = std::tuple<
            std::string,
            Uint8, Uint8, Uint8, Uint8,
            std::string, int, std::string
        >;

        struct CachedText {
            SDL_Texture* texture = nullptr;
            int width = 0;
            int height = 0;
        };

        void clearTextCache();

        RenderContext& context_;
        SDL_Renderer* renderer_ = nullptr;
        mutable FontManager fontManager_;
        FontSpec defaultFontSpec_;
        SDL_Color clearColor_{0, 0, 0, 255};
        std::map<TextCacheKey, CachedText> textCache_;
    };
} // emgui

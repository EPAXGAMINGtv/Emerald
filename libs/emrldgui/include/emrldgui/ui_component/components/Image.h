// Created by epax on 24.11.26.

#pragma once

#include <string>

#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {

    class StyleValues;

    // CSS styled image with BMP loading, tinting and configurable fit modes
    class Image : public UIComponent {
    public:
        enum class Fit {
            Stretch,
            Contain,
            Fill
        };

        Image(
            std::string texturePath = {},
            int x = 0,
            int y = 0,
            int width = 64,
            int height = 64
        );
        ~Image() override;

        // This component owns SDL resources, so copies and moves would dangle
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;
        Image(Image&&) = delete;
        Image& operator=(Image&&) = delete;

        std::string getComponentName() const override {
            return "Image";
        }

        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        SDL_Point getLayoutSize() const override;
        void setLayoutRect(const SDL_Rect& rect) override;

        // Resolved through ResourceManager::texturePath(); loaded once a renderer exists
        void setTexturePath(std::string texturePath);
        const std::string& getTexturePath() const {
            return texturePath_;
        }

        // Adopt an existing texture; owned textures are destroyed with this widget
        void setTexture(SDL_Texture* texture, bool owned = false);
        SDL_Texture* getTexture() const {
            return texture_;
        }

        void setFit(Fit fit) {
            fit_ = fit;
        }

        Fit getFit() const {
            return fit_;
        }

        void setTint(SDL_Color tint) {
            tint_ = tint;
        }

        const SDL_Color& getTint() const {
            return tint_;
        }

        void setSrcRect(SDL_Rect rect) {
            srcRect_ = rect;
            hasSrcRect_ = true;
        }

        void clearSrcRect() {
            srcRect_ = {};
            hasSrcRect_ = false;
        }

    private:
        void releaseTexture();
        void loadTexture(SDL_Renderer* renderer);
        SDL_Rect resolveSourceRect(int textureWidth, int textureHeight) const;
        void drawPlaceholder(RendererOpts& renderer, const SDL_Rect& content) const;

        std::string texturePath_;
        SDL_Texture* texture_ = nullptr;
        bool ownsTexture_ = false;

        Fit fit_ = Fit::Stretch;
        SDL_Color tint_{255, 255, 255, 255};
        SDL_Rect srcRect_{};
        bool hasSrcRect_ = false;

        int width_ = 64;
        int height_ = 64;

        SurfaceStyle surface_;
        Padding padding_;
        SDL_Color placeholderColor_{200, 200, 200, 255};
    };

} // emgui

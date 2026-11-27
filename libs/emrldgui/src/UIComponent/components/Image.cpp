// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/Image.h"

#include "emrldgui/Window.h"
#include "emrldgui/resource/ResourceManager.h"
#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace emgui {

    namespace {

        // Keeps another widget's texture modulation untouched by tinting
        class TextureModGuard {
        public:
            explicit TextureModGuard(SDL_Texture* texture)
                : texture_(texture) {
                if (texture_ == nullptr) {
                    return;
                }

                SDL_GetTextureColorMod(texture_, &red_, &green_, &blue_);
                SDL_GetTextureAlphaMod(texture_, &alpha_);
            }

            ~TextureModGuard() {
                if (texture_ == nullptr) {
                    return;
                }

                SDL_SetTextureColorMod(texture_, red_, green_, blue_);
                SDL_SetTextureAlphaMod(texture_, alpha_);
            }

            TextureModGuard(const TextureModGuard&) = delete;
            TextureModGuard& operator=(const TextureModGuard&) = delete;

            void apply(const SDL_Color& tint, Uint8 alpha) const {
                if (texture_ == nullptr) {
                    return;
                }

                SDL_SetTextureColorMod(texture_, tint.r, tint.g, tint.b);
                SDL_SetTextureAlphaMod(texture_, alpha);
            }

        private:
            SDL_Texture* texture_ = nullptr;
            Uint8 red_ = 255;
            Uint8 green_ = 255;
            Uint8 blue_ = 255;
            Uint8 alpha_ = 255;
        };

    } // namespace

    Image::Image(
        std::string texturePath,
        int x,
        int y,
        int width,
        int height
    )
        : UIComponent(x, y),
          texturePath_(std::move(texturePath)),
          width_(width),
          height_(height) {
        surface_.color = {246, 245, 244, 255};
        surface_.borderColor = {223, 223, 223, 255};
        surface_.hasBorderColor = true;
        surface_.borderWidth = 1;
        surface_.radii = {8, 8, 8, 8};
        padding_.top = 4;
        padding_.right = 4;
        padding_.bottom = 4;
        padding_.left = 4;
    }

    Image::~Image() {
        releaseTexture();
    }

    void Image::releaseTexture() {
        if (ownsTexture_ && texture_ != nullptr) {
            SDL_DestroyTexture(texture_);
        }

        texture_ = nullptr;
        ownsTexture_ = false;
    }

    void Image::setTexture(SDL_Texture* texture, bool owned) {
        if (texture == texture_) {
            ownsTexture_ = texture != nullptr && owned;
            texturePath_.clear();
            return;
        }

        releaseTexture();
        texture_ = texture;
        ownsTexture_ = texture != nullptr && owned;
        texturePath_.clear();
    }

    void Image::setTexturePath(std::string path) {
        if (path == texturePath_ && !path.empty() && texture_ != nullptr) {
            return;
        }

        texturePath_ = std::move(path);
        releaseTexture();

        if (window_ != nullptr) {
            loadTexture(window_->getRendererOpts().getRenderer());
        }
    }

    void Image::onWindowAttached(Window& window) {
        loadTexture(window.getRendererOpts().getRenderer());
    }

    void Image::loadTexture(SDL_Renderer* renderer) {
        if (texturePath_.empty() || texture_ != nullptr) {
            return;
        }

        if (renderer == nullptr) {
            spdlog::warn("Image: cannot load '{}' without an SDL renderer", texturePath_);
            return;
        }

        const std::string path = ResourceManager::texturePath(texturePath_).string();
        SDL_Surface* surface = SDL_LoadBMP(path.c_str());

        if (surface == nullptr) {
            spdlog::warn("Image: failed to load '{}': {}", path, SDL_GetError());
            return;
        }

        SDL_Texture* loaded = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        if (loaded == nullptr) {
            spdlog::warn("Image: failed to create texture from '{}': {}", path, SDL_GetError());
            return;
        }

        texture_ = loaded;
        ownsTexture_ = true;
        spdlog::debug("Image: loaded texture '{}'", path);
    }

    void Image::applyStyle(const StyleValues& values) {
        surface_.apply(values);
        padding_.apply(values, 4);

        tint_ = values.getColor("tint-color", tint_);
        placeholderColor_ = values.getColor("placeholder-color", placeholderColor_);

        const std::string fit = lowerCase(values.getString("image-fit", "stretch"));

        if (fit == "contain") {
            fit_ = Fit::Contain;
        } else if (fit == "fill") {
            fit_ = Fit::Fill;
        } else {
            fit_ = Fit::Stretch;
        }
    }

    SDL_Point Image::getLayoutSize() const {
        return {width_, height_};
    }

    void Image::setLayoutRect(const SDL_Rect& rect) {
        x_ = rect.x;
        y_ = rect.y;
        width_ = rect.w;
        height_ = rect.h;
    }

    SDL_Rect Image::resolveSourceRect(int textureWidth, int textureHeight) const {
        SDL_Rect source{0, 0, textureWidth, textureHeight};

        if (!hasSrcRect_) {
            return source;
        }

        source.x = std::clamp(srcRect_.x, 0, textureWidth - 1);
        source.y = std::clamp(srcRect_.y, 0, textureHeight - 1);
        source.w = std::clamp(srcRect_.w, 1, textureWidth - source.x);
        source.h = std::clamp(srcRect_.h, 1, textureHeight - source.y);
        return source;
    }

    void Image::drawPlaceholder(RendererOpts& renderer, const SDL_Rect& content) const {
        const int radius = std::max(
            0,
            std::min(surface_.maxRadius(), std::min(content.w, content.h) / 2)
        );
        renderer.drawRoundedRect(content, radius, withOpacity(placeholderColor_, 0.65f));

        if (content.w >= 8 && content.h >= 8) {
            const SDL_Color faint = withOpacity(placeholderColor_, 0.35f);
            renderer.drawLine(
                {content.x + 2, content.y + 2},
                {content.x + content.w - 3, content.y + content.h - 3},
                faint
            );
            renderer.drawLine(
                {content.x + content.w - 3, content.y + 2},
                {content.x + 2, content.y + content.h - 3},
                faint
            );
        }
    }

    void Image::render(RendererOpts& renderer) {
        const SDL_Rect rect{x_, y_, width_, height_};

        surface_.drawShadow(renderer, rect);
        surface_.drawBackground(renderer, rect, surface_.color);
        surface_.drawBorder(renderer, rect, surface_.borderColor);

        const SDL_Rect content{
            rect.x + surface_.borderWidth + padding_.left,
            rect.y + surface_.borderWidth + padding_.top,
            std::max(0, rect.w - 2 * surface_.borderWidth - padding_.horizontal()),
            std::max(0, rect.h - 2 * surface_.borderWidth - padding_.vertical())
        };

        if (content.w <= 0 || content.h <= 0) {
            surface_.drawFocus(renderer, rect, isFocused());
            return;
        }

        int textureWidth = 0;
        int textureHeight = 0;

        if (texture_ == nullptr ||
            SDL_QueryTexture(texture_, nullptr, nullptr, &textureWidth, &textureHeight) != 0 ||
            textureWidth <= 0 || textureHeight <= 0) {
            drawPlaceholder(renderer, content);
            surface_.drawFocus(renderer, rect, isFocused());
            return;
        }

        const SDL_Rect source = resolveSourceRect(textureWidth, textureHeight);
        SDL_Rect sourceToDraw = source;
        SDL_Rect destination = content;

        const float scaleX = static_cast<float>(content.w) / static_cast<float>(source.w);
        const float scaleY = static_cast<float>(content.h) / static_cast<float>(source.h);

        switch (fit_) {
            case Fit::Stretch:
                break;
            case Fit::Contain: {
                const float scale = std::min(scaleX, scaleY);
                const int drawnWidth = std::max(
                    1,
                    static_cast<int>(std::lround(static_cast<float>(source.w) * scale))
                );
                const int drawnHeight = std::max(
                    1,
                    static_cast<int>(std::lround(static_cast<float>(source.h) * scale))
                );
                destination = {
                    content.x + (content.w - drawnWidth) / 2,
                    content.y + (content.h - drawnHeight) / 2,
                    std::min(drawnWidth, content.w),
                    std::min(drawnHeight, content.h)
                };
                break;
            }
            case Fit::Fill: {
                const float scale = std::max(scaleX, scaleY);
                const int cropWidth = std::clamp(
                    static_cast<int>(std::lround(static_cast<float>(content.w) / scale)),
                    1,
                    source.w
                );
                const int cropHeight = std::clamp(
                    static_cast<int>(std::lround(static_cast<float>(content.h) / scale)),
                    1,
                    source.h
                );
                sourceToDraw = {
                    source.x + (source.w - cropWidth) / 2,
                    source.y + (source.h - cropHeight) / 2,
                    cropWidth,
                    cropHeight
                };
                break;
            }
            default:
                break;
        }

        SDL_Renderer* sdlRenderer = renderer.getRenderer();

        if (sdlRenderer == nullptr) {
            surface_.drawFocus(renderer, rect, isFocused());
            return;
        }

        const Uint8 alpha = static_cast<Uint8>(
            std::lround(static_cast<float>(tint_.a) * surface_.opacity)
        );
        const TextureModGuard guard(texture_);
        guard.apply(tint_, alpha);

        const bool fullSource = sourceToDraw.x == 0 && sourceToDraw.y == 0 &&
                                sourceToDraw.w == textureWidth &&
                                sourceToDraw.h == textureHeight;

        if (fullSource) {
            renderer.drawTexture(texture_, destination);
        } else {
            SDL_RenderCopy(sdlRenderer, texture_, &sourceToDraw, &destination);
        }

        surface_.drawFocus(renderer, rect, isFocused());
    }

} // emgui

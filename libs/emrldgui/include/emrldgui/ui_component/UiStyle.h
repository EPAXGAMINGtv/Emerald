// Created by epax on 22.11.26.

#pragma once

#include "emrldgui/render/RendererOpts.h"
#include "emrldgui/style/StyleValues.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace emgui {

    inline SDL_Color blendColor(const SDL_Color& from, const SDL_Color& to, float amount) {
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

    inline SDL_Color withOpacity(const SDL_Color& color, float opacity) {
        SDL_Color result = color;
        result.a = static_cast<Uint8>(
            std::lround(static_cast<float>(color.a) * std::clamp(opacity, 0.0f, 1.0f))
        );
        return result;
    }

    inline std::string lowerCase(std::string value) {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            }
        );
        return value;
    }

    inline float easeOutCubic(float value) {
        const float t = std::clamp(value, 0.0f, 1.0f);
        const float inverted = 1.0f - t;
        return 1.0f - inverted * inverted * inverted;
    }

    inline bool isAuto(const std::string& value) {
        return lowerCase(value) == "auto";
    }

    // Css-style padding with optional per-side overrides
    struct Padding {
        int top = 0;
        int right = 0;
        int bottom = 0;
        int left = 0;

        void apply(const StyleValues& values, int fallback = 0) {
            const int all = values.getInt("padding", fallback);
            top = values.getInt("padding-top", all);
            right = values.getInt("padding-right", all);
            bottom = values.getInt("padding-bottom", all);
            left = values.getInt("padding-left", all);
        }

        int horizontal() const {
            return left + right;
        }

        int vertical() const {
            return top + bottom;
        }
    };

    // Size overrides so CSS can shape a component without changing constructor calls
    struct GeometryStyle {
        bool autoWidth = false;
        bool autoHeight = false;
        int width = 0;
        int height = 0;
        int minWidth = 0;
        int minHeight = 0;
        int maxWidth = 0;
        int maxHeight = 0;

        // Once a Layout assigns a rect, it stays authoritative over CSS width/height
        bool layoutControlled = false;

        void lockToLayout(int layoutWidth, int layoutHeight) {
            layoutControlled = true;
            autoWidth = false;
            autoHeight = false;
            width = layoutWidth;
            height = layoutHeight;
        }

        void apply(const StyleValues& values, int currentWidth, int currentHeight) {
            const std::string widthValue = values.getString("width");
            const std::string heightValue = values.getString("height");

            autoWidth = isAuto(widthValue);
            autoHeight = isAuto(heightValue);
            width = autoWidth ? currentWidth : values.getInt("width", currentWidth);
            height = autoHeight ? currentHeight : values.getInt("height", currentHeight);

            minWidth = std::max(0, values.getInt("min-width", 0));
            minHeight = std::max(0, values.getInt("min-height", 0));
            maxWidth = std::max(0, values.getInt("max-width", 0));
            maxHeight = std::max(0, values.getInt("max-height", 0));

            width = std::max(width, minWidth);
            height = std::max(height, minHeight);

            if (maxWidth > 0) {
                width = std::min(width, maxWidth);
            }
            if (maxHeight > 0) {
                height = std::min(height, maxHeight);
            }

            if (layoutControlled) {
                autoWidth = false;
                autoHeight = false;
                width = currentWidth;
                height = currentHeight;
            }
        }

        SDL_Rect resolve(int x, int y, int contentWidth, int contentHeight) const {
            int resolvedWidth = autoWidth ? contentWidth : width;
            int resolvedHeight = autoHeight ? contentHeight : height;

            resolvedWidth = std::max(resolvedWidth, minWidth);
            resolvedHeight = std::max(resolvedHeight, minHeight);

            if (maxWidth > 0) {
                resolvedWidth = std::min(resolvedWidth, maxWidth);
            }
            if (maxHeight > 0) {
                resolvedHeight = std::min(resolvedHeight, maxHeight);
            }

            return {x, y, resolvedWidth, resolvedHeight};
        }
    };

    // Shared CSS-driven drop shadow used by all elevated components
    struct ShadowStyle {
        SDL_Color color{0, 0, 0, 0};
        int offsetX = 0;
        int offsetY = 0;
        int spread = 0;

        void apply(const StyleValues& values) {
            color = values.getColor("shadow-color", color);
            offsetX = values.getInt("shadow-offset-x", offsetX);
            offsetY = values.getInt("shadow-offset-y", offsetY);
            spread = std::max(0, values.getInt("shadow-spread", spread));
        }

        void draw(RendererOpts& renderer, const SDL_Rect& rect, int radius) const {
            if (color.a == 0 || (offsetX == 0 && offsetY == 0 && spread == 0)) {
                return;
            }

            renderer.drawShadow(rect, radius, color, offsetX, offsetY, spread);
        }
    };

    // Shared focus/hover ring so keyboard and mouse focus stay visually consistent
    struct FocusRingStyle {
        SDL_Color color{110, 168, 254, 255};
        int width = 0;

        void apply(const StyleValues& values) {
            color = values.getColor("focus-ring-color", color);
            width = std::max(0, values.getInt("focus-ring-width", width));
        }

        void draw(RendererOpts& renderer, const SDL_Rect& rect, int radius, bool visible) const {
            if (!visible || width <= 0 || color.a == 0) {
                return;
            }

            renderer.drawRoundedRect(
                {
                    rect.x - width,
                    rect.y - width,
                    rect.w + 2 * width,
                    rect.h + 2 * width
                },
                radius + width,
                color
            );
        }
    };

    // Common rectangle styling shared by buttons, fields, panels and popups
    struct SurfaceStyle {
        SDL_Color color{0, 0, 0, 0};
        std::vector<SDL_Color> gradient;
        bool horizontalGradient = false;
        SDL_Color borderColor{0, 0, 0, 0};
        bool hasBorderColor = false;
        int borderWidth = 0;
        CornerRadii radii{};
        float opacity = 1.0f;
        ShadowStyle shadow;
        FocusRingStyle focusRing;

        void apply(const StyleValues& values) {
            color = values.getColor("background-color", color);
            gradient = values.getColors("background-gradient");
            horizontalGradient = lowerCase(
                values.getString("background-gradient-direction", "vertical")
            ) == "horizontal";

            hasBorderColor = values.contains("border-color");
            borderColor = values.getColor("border-color", borderColor);
            borderWidth = std::max(0, values.getInt("border-width", borderWidth));

            const int radius = values.getInt("border-radius", 0);

            radii.topLeft = values.getInt("border-top-left-radius", radius);
            radii.topRight = values.getInt("border-top-right-radius", radius);
            radii.bottomRight = values.getInt("border-bottom-right-radius", radius);
            radii.bottomLeft = values.getInt("border-bottom-left-radius", radius);

            opacity = std::clamp(values.getFloat("opacity", opacity), 0.0f, 1.0f);
            shadow.apply(values);
            focusRing.apply(values);
        }

        int maxRadius() const {
            return std::max({radii.topLeft, radii.topRight,
                             radii.bottomRight, radii.bottomLeft});
        }

        void drawShadow(RendererOpts& renderer, const SDL_Rect& rect) const {
            shadow.draw(renderer, rect, maxRadius());
        }

        void drawBackgroundStops(
            RendererOpts& renderer,
            const SDL_Rect& rect,
            const SDL_Color& solidColor,
            const std::vector<SDL_Color>& stops
        ) const {
            if (!stops.empty()) {
                std::vector<SDL_Color> faded;
                faded.reserve(stops.size());

                for (const SDL_Color& stop : stops) {
                    faded.push_back(withOpacity(stop, opacity));
                }

                renderer.fillRoundedRectGradient(
                    rect,
                    radii,
                    faded,
                    horizontalGradient
                );
                return;
            }

            renderer.fillRoundedRect(rect, radii, withOpacity(solidColor, opacity));
        }

        void drawBackground(
            RendererOpts& renderer,
            const SDL_Rect& rect,
            const SDL_Color& fallbackColor,
            const std::vector<SDL_Color>& fallbackGradient = {}
        ) const {
            const bool useOwnGradient = !gradient.empty();
            drawBackgroundStops(
                renderer,
                rect,
                useOwnGradient ? color : fallbackColor,
                useOwnGradient ? gradient : fallbackGradient
            );
        }

        void drawBorderColor(
            RendererOpts& renderer,
            const SDL_Rect& rect,
            const SDL_Color& borderColorValue
        ) const {
            const SDL_Color colorToDraw = withOpacity(borderColorValue, opacity);

            for (int inset = 0; inset < borderWidth; ++inset) {
                const SDL_Rect inner{
                    rect.x + inset,
                    rect.y + inset,
                    rect.w - 2 * inset,
                    rect.h - 2 * inset
                };

                if (inner.w <= 0 || inner.h <= 0) {
                    break;
                }

                renderer.drawRoundedRect(inner, radii, colorToDraw);

                if (maxRadius() <= inset) {
                    break;
                }
            }
        }

        void drawBorder(
            RendererOpts& renderer,
            const SDL_Rect& rect,
            const SDL_Color& fallbackColor
        ) const {
            drawBorderColor(
                renderer,
                rect,
                hasBorderColor ? borderColor : fallbackColor
            );
        }

        void drawFocus(
            RendererOpts& renderer,
            const SDL_Rect& rect,
            bool visible
        ) const {
            focusRing.draw(renderer, rect, maxRadius(), visible);
        }

        void draw(
            RendererOpts& renderer,
            const SDL_Rect& rect,
            const SDL_Color& background,
            const SDL_Color& border,
            bool focusVisible
        ) const {
            drawShadow(renderer, rect);
            drawBackground(renderer, rect, background);
            drawBorder(renderer, rect, border);
            drawFocus(renderer, rect, focusVisible);
        }
    };

    // Text decorations that work for every label rendered through this helper
    struct TextStyle {
        SDL_Color shadowColor{0, 0, 0, 0};
        int shadowOffsetX = 0;
        int shadowOffsetY = 0;
        int letterSpacing = 0;
        std::string transform = "none";

        void apply(const StyleValues& values) {
            shadowColor = values.getColor("text-shadow-color", shadowColor);
            shadowOffsetX = values.getInt("text-shadow-offset-x", shadowOffsetX);
            shadowOffsetY = values.getInt("text-shadow-offset-y", shadowOffsetY);
            letterSpacing = values.getInt("letter-spacing", letterSpacing);

            const std::string value = lowerCase(
                values.getString("text-transform", transform)
            );
            transform = value == "uppercase" || value == "lowercase"
                ? value
                : "none";
        }

        std::string prepare(std::string text) const {
            if (transform == "uppercase") {
                std::transform(
                    text.begin(),
                    text.end(),
                    text.begin(),
                    [](unsigned char character) {
                        return static_cast<char>(std::toupper(character));
                    }
                );
            } else if (transform == "lowercase") {
                std::transform(
                    text.begin(),
                    text.end(),
                    text.begin(),
                    [](unsigned char character) {
                        return static_cast<char>(std::tolower(character));
                    }
                );
            }

            return text;
        }

        void draw(
            RendererOpts& renderer,
            const std::string& text,
            const SDL_Point& position,
            const SDL_Color& color,
            const FontSpec& fontSpec
        ) const {
            if (text.empty()) {
                return;
            }

            const std::string prepared = prepare(text);

            if (shadowColor.a > 0 &&
                (shadowOffsetX != 0 || shadowOffsetY != 0)) {
                drawSpaced(
                    renderer,
                    prepared,
                    {position.x + shadowOffsetX, position.y + shadowOffsetY},
                    shadowColor,
                    fontSpec
                );
            }

            drawSpaced(renderer, prepared, position, color, fontSpec);
        }

    private:
        void drawSpaced(
            RendererOpts& renderer,
            const std::string& text,
            const SDL_Point& position,
            const SDL_Color& color,
            const FontSpec& fontSpec
        ) const {
            if (letterSpacing == 0) {
                renderer.drawText(text, position, color, fontSpec);
                return;
            }

            int x = position.x;

            for (const char character : text) {
                const std::string single(1, character);
                renderer.drawText(single, {x, position.y}, color, fontSpec);
                x += renderer.measureText(single, fontSpec).x + letterSpacing;
            }
        }
    };

} // emgui

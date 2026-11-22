//
// Created by epax on 22.11.26.
//

#pragma once

#include "emrldgui/render/RendererOpts.h"
#include "emrldgui/style/StyleValues.h"

#include <algorithm>
#include <cmath>

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

    inline float easeOutCubic(float value) {
        const float t = std::clamp(value, 0.0f, 1.0f);
        const float inverted = 1.0f - t;
        return 1.0f - inverted * inverted * inverted;
    }

    //shared CSS-driven drop shadow used by all elevated components
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

    //shared focus/hover ring so keyboard and mouse focus stay visually consistent
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

} // emgui

//
// Created by epax on 22.11.26.
//

#include "emrldgui/ui_component/components/Panel.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <utility>

namespace emgui {

    Panel::Panel(int x, int y, int width, int height, std::string title)
        : UIComponent(x, y),
          title_(std::move(title)),
          width_(width),
          height_(height) {
    }

    void Panel::applyStyle(const StyleValues& values) {
        backgroundColor_ = values.getColor("background-color", backgroundColor_);
        borderColor_ = values.getColor("border-color", borderColor_);

        const auto gradient = values.getColors("background-gradient");

        if (gradient.size() >= 2) {
            useGradient_ = true;
            gradientTopColor_ = gradient[0];
            gradientBottomColor_ = gradient[1];
        } else {
            useGradient_ = false;
        }

        shadow_.apply(values);
        titleColor_ = values.getColor("title-color", titleColor_);

        borderWidth_ = std::max(0, values.getInt("border-width", borderWidth_));
        borderRadius_ = std::max(0, values.getInt("border-radius", borderRadius_));
        padding_ = std::max(0, values.getInt("padding", padding_));
        titleSpacing_ = std::max(0, values.getInt("title-spacing", titleSpacing_));

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        spdlog::debug(
            "Panel style: size={}x{} radius={} title='{}'",
            width_,
            height_,
            borderRadius_,
            title_
        );
    }

    void Panel::render(RendererOpts& renderer) {
        const SDL_Rect rect{x_, y_, width_, height_};

        shadow_.draw(renderer, rect, borderRadius_);

        if (useGradient_) {
            renderer.fillRoundedRectGradient(
                rect,
                borderRadius_,
                gradientTopColor_,
                gradientBottomColor_
            );
        } else {
            renderer.fillRoundedRect(rect, borderRadius_, backgroundColor_);
        }

        for (int inset = 0; inset < borderWidth_; ++inset) {
            const SDL_Rect borderRect{
                rect.x + inset,
                rect.y + inset,
                rect.w - 2 * inset,
                rect.h - 2 * inset
            };

            if (borderRect.w <= 0 || borderRect.h <= 0) {
                break;
            }

            renderer.drawRoundedRect(
                borderRect,
                std::max(0, borderRadius_ - inset),
                borderColor_
            );
        }

        if (title_.empty()) {
            titleOffset_ = 0;
            return;
        }

        const SDL_Point titleSize = renderer.measureText(title_, fontSpec_);
        titleOffset_ = titleSize.y + titleSpacing_;

        renderer.drawText(
            title_,
            {x_ + padding_, y_ + padding_},
            titleColor_,
            fontSpec_
        );
    }

} // emgui

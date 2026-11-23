// Created by epax on 22.11.26.

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
        surface_.color = {30, 30, 46, 235};
        surface_.borderColor = {70, 74, 96, 255};
        surface_.hasBorderColor = true;
        surface_.borderWidth = 1;
        surface_.radii = {12, 12, 12, 12};
        padding_.top = 14;
        padding_.right = 14;
        padding_.bottom = 14;
        padding_.left = 14;
        geometry_.width = width_;
        geometry_.height = height_;
    }

    void Panel::applyStyle(const StyleValues& values) {
        surface_.apply(values);
        titleStyle_.apply(values);
        geometry_.apply(values, width_, height_);

        width_ = geometry_.width;
        height_ = geometry_.height;

        titleColor_ = values.getColor("title-color", titleColor_);
        padding_.apply(values, 14);
        titleSpacing_ = std::max(0, values.getInt("title-spacing", titleSpacing_));

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        spdlog::debug(
            "Panel style: size={}x{} radius={} title='{}'",
            width_,
            height_,
            surface_.maxRadius(),
            title_
        );
    }

    void Panel::setLayoutRect(const SDL_Rect& rect) {
        x_ = rect.x;
        y_ = rect.y;
        width_ = rect.w;
        height_ = rect.h;
        geometry_.lockToLayout(rect.w, rect.h);
    }

    void Panel::render(RendererOpts& renderer) {
        const SDL_Rect rect{x_, y_, width_, height_};

        surface_.drawShadow(renderer, rect);
        surface_.drawBackground(renderer, rect, surface_.color);
        surface_.drawBorder(renderer, rect, surface_.borderColor);

        if (title_.empty()) {
            titleOffset_ = 0;
            return;
        }

        const std::string prepared = titleStyle_.prepare(title_);
        const SDL_Point titleSize = renderer.measureText(prepared, fontSpec_);
        titleOffset_ = titleSize.y + titleSpacing_;

        titleStyle_.draw(
            renderer,
            prepared,
            {x_ + padding_.left, y_ + padding_.top},
            titleColor_,
            fontSpec_
        );
    }

} // emgui

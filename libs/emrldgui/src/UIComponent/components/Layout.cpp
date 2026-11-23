// Created by epax on 23.11.26.

#include "emrldgui/ui_component/components/Layout.h"

#include "emrldgui/style/StyleValues.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace emgui {

    namespace {

        std::string lower(std::string value) {
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

        Layout::Direction parseDirection(const std::string& value) {
            return lower(value) == "column"
                ? Layout::Direction::Column
                : Layout::Direction::Row;
        }

        Layout::Align parseAlign(const std::string& value) {
            const std::string parsed = lower(value);

            if (parsed == "start" || parsed == "flex-start") {
                return Layout::Align::Start;
            }
            if (parsed == "end" || parsed == "flex-end") {
                return Layout::Align::End;
            }
            if (parsed == "center") {
                return Layout::Align::Center;
            }
            return Layout::Align::Stretch;
        }

        Layout::Justify parseJustify(const std::string& value) {
            const std::string parsed = lower(value);

            if (parsed == "center") {
                return Layout::Justify::Center;
            }
            if (parsed == "end" || parsed == "flex-end") {
                return Layout::Justify::End;
            }
            if (parsed == "space-between") {
                return Layout::Justify::SpaceBetween;
            }
            if (parsed == "space-around") {
                return Layout::Justify::SpaceAround;
            }
            return Layout::Justify::Start;
        }

    } // namespace

    Layout::Layout(int x, int y, int width, int height)
        : UIComponent(x, y), width_(width), height_(height) {
        surface_.color = {18, 26, 48, 0};
        surface_.radii = {12, 12, 12, 12};
        geometry_.width = width_;
        geometry_.height = height_;
        padding_.top = 8;
        padding_.right = 8;
        padding_.bottom = 8;
        padding_.left = 8;
    }

    void Layout::applyStyle(const StyleValues& values) {
        surface_.apply(values);
        geometry_.apply(values, width_, height_);

        width_ = geometry_.width;
        height_ = geometry_.height;

        padding_.apply(values, 8);
        direction_ = parseDirection(values.getString("flex-direction", "row"));
        align_ = parseAlign(values.getString("align-items", "stretch"));
        justify_ = parseJustify(values.getString("justify-content", "start"));
        gap_ = std::max(0, values.getInt("gap", gap_));
    }

    void Layout::setLayoutRect(const SDL_Rect& rect) {
        x_ = rect.x;
        y_ = rect.y;
        width_ = rect.w;
        height_ = rect.h;
        geometry_.lockToLayout(rect.w, rect.h);
    }

    void Layout::addChild(UIComponent* child) {
        if (child == nullptr || child == this) {
            return;
        }

        if (std::find(children_.begin(), children_.end(), child) == children_.end()) {
            children_.push_back(child);
        }
    }

    void Layout::removeChild(UIComponent* child) {
        children_.erase(
            std::remove(children_.begin(), children_.end(), child),
            children_.end()
        );
    }

    void Layout::clearChildren() {
        children_.clear();
    }

    void Layout::setSubtreeVisible(bool visible) {
        setVisible(visible);

        for (UIComponent* child : children_) {
            if (child != nullptr) {
                child->setSubtreeVisible(visible);
            }
        }
    }

    void Layout::layout() {
        if (children_.empty()) {
            return;
        }

        const int innerX = x_ + padding_.left;
        const int innerY = y_ + padding_.top;
        const int innerWidth = std::max(0, width_ - padding_.horizontal());
        const int innerHeight = std::max(0, height_ - padding_.vertical());

        struct Item {
            UIComponent* child = nullptr;
            int mainSize = 0;
            int crossSize = 0;
            float flex = 0.0f;
            Align align = Align::Stretch;
        };

        const bool row = direction_ == Direction::Row;
        std::vector<Item> items;
        int baseMain = 0;
        float flexTotal = 0.0f;

        for (UIComponent* child : children_) {
            if (child == nullptr || !child->isVisible()) {
                continue;
            }

            const SDL_Point natural = child->getLayoutSize();
            Item item;
            item.child = child;
            item.mainSize = std::max(0, row ? natural.x : natural.y);
            item.crossSize = std::max(0, row ? natural.y : natural.x);
            item.flex = std::max(0.0f, child->getFlex());
            item.align = parseAlign(child->getAlignSelf());

            baseMain += item.mainSize;
            flexTotal += item.flex;
            items.push_back(item);
        }

        if (items.empty()) {
            return;
        }

        const int gaps = gap_ * static_cast<int>(items.size() - 1);
        const int mainTotal = row ? innerWidth : innerHeight;
        const int crossTotal = row ? innerHeight : innerWidth;
        const int available = mainTotal - gaps - baseMain;

        if (available > 0 && flexTotal > 0.0f) {
            for (Item& item : items) {
                if (item.flex > 0.0f) {
                    item.mainSize += static_cast<int>(
                        static_cast<float>(available) * item.flex / flexTotal
                    );
                }
            }
        }

        int usedMain = gaps;

        for (const Item& item : items) {
            usedMain += item.mainSize;
        }

        const int remaining = mainTotal - usedMain;
        int effectiveGap = gap_;
        int offset = 0;

        switch (justify_) {
            case Justify::Center:
                offset = remaining / 2;
                break;
            case Justify::End:
                offset = remaining;
                break;
            case Justify::SpaceBetween:
                if (items.size() > 1) {
                    effectiveGap += remaining / static_cast<int>(items.size() - 1);
                }
                break;
            case Justify::SpaceAround:
                effectiveGap += remaining / static_cast<int>(items.size());
                offset = remaining / (2 * static_cast<int>(items.size()));
                break;
            case Justify::Start:
            default:
                break;
        }

        int cursor = offset;

        for (const Item& item : items) {
            const Align crossAlign = item.align == Align::Stretch ? align_ : item.align;
            const int naturalCross = item.crossSize;
            const int crossSize = crossAlign == Align::Stretch ? crossTotal : naturalCross;
            int crossOffset = 0;

            switch (crossAlign) {
                case Align::Center:
                    crossOffset = (crossTotal - crossSize) / 2;
                    break;
                case Align::End:
                    crossOffset = crossTotal - crossSize;
                    break;
                case Align::Start:
                case Align::Stretch:
                default:
                    break;
            }

            SDL_Rect rect;

            if (row) {
                rect = {
                    innerX + cursor,
                    innerY + crossOffset,
                    item.mainSize,
                    crossSize
                };
            } else {
                rect = {
                    innerX + crossOffset,
                    innerY + cursor,
                    crossSize,
                    item.mainSize
                };
            }

            item.child->setLayoutRect(rect);
            item.child->layout();
            cursor += item.mainSize + effectiveGap;
        }
    }

    void Layout::render(RendererOpts& renderer) {
        const SDL_Rect rect{x_, y_, width_, height_};
        surface_.drawShadow(renderer, rect);
        surface_.drawBackground(renderer, rect, surface_.color);
        surface_.drawBorder(renderer, rect, surface_.borderColor);
    }

} // emgui

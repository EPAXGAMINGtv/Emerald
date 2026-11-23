// Created by epax on 23.11.26.

#pragma once

#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

#include <string>
#include <vector>

namespace emgui {

    class StyleValues;

    // CSS-driven flex container: arranges registered child components
    class Layout : public UIComponent {
    public:
        enum class Direction { Row, Column };
        enum class Align { Start, Center, End, Stretch };
        enum class Justify { Start, Center, End, SpaceBetween, SpaceAround };

        Layout(int x, int y, int width, int height);

        std::string getComponentName() const override {
            return "Layout";
        }

        void applyStyle(const StyleValues& values) override;
        void layout() override;
        void render(RendererOpts& renderer) override;

        SDL_Point getLayoutSize() const override {
            return {width_, height_};
        }

        void setLayoutRect(const SDL_Rect& rect) override;
        void setSubtreeVisible(bool visible) override;

        void addChild(UIComponent* child);
        void removeChild(UIComponent* child);
        void clearChildren();

        const std::vector<UIComponent*>& getChildren() const {
            return children_;
        }

        void setDirection(Direction direction) {
            direction_ = direction;
        }
        void setGap(int gap) {
            gap_ = gap > 0 ? gap : 0;
        }
        void setAlign(Align align) {
            align_ = align;
        }
        void setJustify(Justify justify) {
            justify_ = justify;
        }

        int getWidth() const {
            return width_;
        }
        int getHeight() const {
            return height_;
        }

    private:
        int width_ = 0;
        int height_ = 0;

        std::vector<UIComponent*> children_;

        SurfaceStyle surface_;
        GeometryStyle geometry_;
        Padding padding_;

        Direction direction_ = Direction::Row;
        Align align_ = Align::Stretch;
        Justify justify_ = Justify::Start;
        int gap_ = 0;
    };

} // emgui

// Created by epax on 24.11.26.

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "emrldgui/Window.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"
#include "emrldgui/ui_component/components/ScrollBar.h"

namespace emgui {

    class StyleValues;

    // Clipping container with stacked children, optional callback content and
    // internal CSS-styled scrollbars
    class ScrollArea : public UIComponent {
    public:
        using ContentRenderer = std::function<void(
            RendererOpts&,
            const SDL_Rect&,
            int,
            int
        )>;

        ScrollArea(int x, int y, int width, int height);
        ~ScrollArea() override;

        // The listeners and internal scrollbars capture this, so copies and
        // moves would dangle
        ScrollArea(const ScrollArea&) = delete;
        ScrollArea& operator=(const ScrollArea&) = delete;
        ScrollArea(ScrollArea&&) = delete;
        ScrollArea& operator=(ScrollArea&&) = delete;

        std::string getComponentName() const override {
            return "ScrollArea";
        }

        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;
        void layout() override;
        void render(RendererOpts& renderer) override;

        SDL_Point getLayoutSize() const override {
            return {width_, height_};
        }

        void setLayoutRect(const SDL_Rect& rect) override;
        void setSubtreeVisible(bool visible) override;

        // Children are drawn by this area; register them with the manager as
        // usual so styling, layout, updates and events keep working
        void addChild(UIComponent* child);
        void removeChild(UIComponent* child);
        void clearChildren();

        const std::vector<UIComponent*>& getChildren() const {
            return children_;
        }

        void setContentSize(int width, int height);
        SDL_Point getContentSize() const {
            return {contentWidth_, contentHeight_};
        }

        void setScroll(int x, int y);
        int getScrollX() const {
            return scrollX_;
        }
        int getScrollY() const {
            return scrollY_;
        }
        void scrollBy(int dx, int dy);

        void setContentRenderer(ContentRenderer renderer);

    private:
        void onMouseEvent(EventMouse& event);
        void updateScrollBars();
        void syncBarsFromScroll();
        void clampScroll();

        SDL_Rect getViewportRect() const;
        int getBarViewportWidth() const;
        int getBarViewportHeight() const;
        int getMaxScrollX() const;
        int getMaxScrollY() const;

        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;

        int width_ = 0;
        int height_ = 0;
        int contentWidth_ = 0;
        int contentHeight_ = 0;
        int scrollX_ = 0;
        int scrollY_ = 0;
        bool contentSizeExplicit_ = false;

        SDL_Color backgroundColor_{255, 255, 255, 255};
        SDL_Color borderColor_{223, 223, 223, 255};
        int borderWidth_ = 1;
        int radius_ = 8;

        Padding padding_;
        int gap_ = 0;
        bool rowDirection_ = false;
        int wheelStep_ = 48;
        int scrollbarThickness_ = 12;

        std::vector<UIComponent*> children_;
        ContentRenderer contentRenderer_;

        ScrollBar verticalBar_{
            ScrollBar::Orientation::Vertical, 0, 0, 0, 0
        };
        ScrollBar horizontalBar_{
            ScrollBar::Orientation::Horizontal, 0, 0, 0, 0
        };
    };

} // emgui

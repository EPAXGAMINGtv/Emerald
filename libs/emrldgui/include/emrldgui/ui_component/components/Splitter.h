// Created by epax on 24.11.26.

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "emrldgui/Window.h"
#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventManager.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {

    class StyleValues;

    // Two-pane container with a draggable, CSS-styled divider
    class Splitter : public UIComponent {
    public:
        // Vertical draws a vertical divider (left/right panes);
        // Horizontal draws a horizontal divider (top/bottom panes)
        enum class Orientation { Vertical, Horizontal };

        using SplitCallback = std::function<void(Splitter&)>;

        Splitter(
            int x,
            int y,
            int width,
            int height,
            Orientation orientation = Orientation::Vertical
        );
        ~Splitter() override;

        // Listeners and callbacks capture this, so copies and moves would dangle
        Splitter(const Splitter&) = delete;
        Splitter& operator=(const Splitter&) = delete;
        Splitter(Splitter&&) = delete;
        Splitter& operator=(Splitter&&) = delete;

        std::string getComponentName() const override {
            return "Splitter";
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
        void collectFocusable(std::vector<UIComponent*>& out) override;

        // Panes are positioned here; register them with the UIComponentManager
        // as usual so styling, updates and input keep working
        void setFirst(UIComponent* component);
        void setSecond(UIComponent* component);

        UIComponent* getFirst() const {
            return first_;
        }

        UIComponent* getSecond() const {
            return second_;
        }

        // Fraction of the main axis occupied by the first pane (0..1)
        void setSplit(float fraction);
        float getSplit() const {
            return split_;
        }

        void setMinPaneSize(int pixels);
        int getMinPaneSize() const {
            return minPaneSize_;
        }

        void onSplitChanged(SplitCallback callback);

        bool isDragging() const {
            return dragging_;
        }

        Orientation getOrientation() const {
            return orientation_;
        }

    private:
        struct Geometry {
            SDL_Rect first{};
            SDL_Rect second{};
            SDL_Rect handle{};
        };

        void onMouseEvent(EventMouse& event);
        void onKeyEvent(EventKeyboard& event);
        void notifySplitChanged();
        void updateCursor(bool overHandle);
        void updateSplitFromPointer(int coordinate);
        float clampFraction(float value) const;
        bool containsHandle(int x, int y) const;
        SDL_Rect getInnerRect() const;
        Geometry computeGeometry() const;
        void drawHandle(RendererOpts& renderer, const SDL_Rect& rect) const;
        SDL_SystemCursor getResizeCursor() const;

        EventManager::ListenerId mouseListener_ = EventManager::kInvalidListener;
        EventManager::ListenerId keyListener_ = EventManager::kInvalidListener;

        int width_ = 0;
        int height_ = 0;
        Orientation orientation_ = Orientation::Vertical;

        UIComponent* first_ = nullptr;
        UIComponent* second_ = nullptr;

        float split_ = 0.5f;
        int minPaneSize_ = 0;
        int dragOffset_ = 0;
        bool dragging_ = false;
        bool cursorActive_ = false;

        SDL_Cursor* resizeCursor_ = nullptr;

        SurfaceStyle surface_;
        Padding padding_;
        FocusRingStyle focusRing_;

        SDL_Color handleColor_{198, 210, 230, 255};
        SDL_Color handleHoverColor_{216, 233, 251, 255};
        SDL_Color handleActiveColor_{53, 132, 228, 255};
        SDL_Color gripColor_{90, 108, 138, 255};

        int handleWidth_ = 8;
        int gripSize_ = 4;
        int gripGap_ = 6;
        std::string cursorHint_ = "auto";

        std::vector<SplitCallback> splitCallbacks_;
    };

} // emgui

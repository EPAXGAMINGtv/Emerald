// Created by epax on 24.11.26.

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "emrldgui/Window.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UIComponent.h"

namespace emgui {

    class StyleValues;

    // CSS-styled scrollbar with a proportional thumb, wheel, drag and paging
    class ScrollBar : public UIComponent {
    public:
        enum class Orientation { Vertical, Horizontal };

        using ValueChangedAction = std::function<void(int)>;

        ScrollBar(Orientation orientation, int x, int y, int width, int height);
        ~ScrollBar() override;

        // The mouse listener captures this, so copies and moves would dangle
        ScrollBar(const ScrollBar&) = delete;
        ScrollBar& operator=(const ScrollBar&) = delete;
        ScrollBar(ScrollBar&&) = delete;
        ScrollBar& operator=(ScrollBar&&) = delete;

        std::string getComponentName() const override {
            return "ScrollBar";
        }

        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        SDL_Point getLayoutSize() const override;
        void setLayoutRect(const SDL_Rect& rect) override;

        void setRange(int contentSize, int viewportSize);
        void setValue(int value);
        int getValue() const {
            return value_;
        }
        int getMax() const {
            return maxValue_;
        }
        int getContentSize() const {
            return contentSize_;
        }
        int getViewportSize() const {
            return viewportSize_;
        }

        void setPageStep(int pageStep);
        int getPageStep() const {
            return pageStep_;
        }

        // Containers with their own wheel handling can disable this listener action
        void setWheelEnabled(bool enabled) {
            wheelEnabled_ = enabled;
        }

        void onValueChanged(ValueChangedAction action);

        Orientation getOrientation() const {
            return orientation_;
        }

    private:
        void onMouseEvent(EventMouse& event);
        void setHoveredState(bool hovered);
        void setPressedState(bool pressed);
        void notifyValueChanged();

        bool containsPoint(int x, int y) const;
        SDL_Rect getTrackRect() const;
        SDL_Rect getThumbRect() const;

        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;

        Orientation orientation_ = Orientation::Vertical;
        int width_ = 0;
        int height_ = 0;

        int contentSize_ = 0;
        int viewportSize_ = 0;
        int maxValue_ = 0;
        int value_ = 0;
        int pageStep_ = 1;

        SDL_Color trackColor_{246, 245, 244, 255};
        SDL_Color thumbColor_{192, 192, 192, 255};
        SDL_Color thumbHoverColor_{53, 132, 228, 255};
        SDL_Color thumbPressedColor_{27, 111, 196, 255};
        SDL_Color borderColor_{223, 223, 223, 255};

        int borderWidth_ = 1;
        int radius_ = 6;
        int thickness_ = 12;
        int wheelStep_ = 48;

        bool hovered_ = false;
        bool pressed_ = false;
        bool dragging_ = false;
        bool wheelEnabled_ = true;
        int dragOffset_ = 0;

        std::vector<ValueChangedAction> valueChangedActions_;
    };

} // emgui

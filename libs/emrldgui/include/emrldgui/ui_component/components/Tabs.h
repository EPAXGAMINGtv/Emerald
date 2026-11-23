// Created by epax on 23.11.26.

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "emrldgui/Window.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {

    class StyleValues;

    // Tab bar that switches the visibility of non-owning page components
    class Tabs : public UIComponent {
    public:
        using SelectionCallback = std::function<void(Tabs&, int)>;

        struct Page {
            std::string title;
            UIComponent* component = nullptr;
        };

        Tabs(int x, int y, int width, int height);
        ~Tabs() override;

        Tabs(const Tabs&) = delete;
        Tabs& operator=(const Tabs&) = delete;
        Tabs(Tabs&&) = delete;
        Tabs& operator=(Tabs&&) = delete;

        std::string getComponentName() const override {
            return "Tabs";
        }

        void applyStyle(const StyleValues& values) override;
        void layout() override;
        void render(RendererOpts& renderer) override;
        void onWindowAttached(Window& window) override;

        SDL_Point getLayoutSize() const override {
            return {width_, height_};
        }

        void setLayoutRect(const SDL_Rect& rect) override;
        void setSubtreeVisible(bool visible) override;

        int addPage(std::string title, UIComponent* page);
        void setActivePage(int index);
        int getActivePage() const {
            return activePage_;
        }
        int getPageCount() const {
            return static_cast<int>(pages_.size());
        }
        const Page& getPage(int index) const {
            return pages_.at(static_cast<std::size_t>(index));
        }

        void onSelectionChanged(SelectionCallback callback) {
            if (callback) {
                selectionCallbacks_.push_back(std::move(callback));
            }
        }

    private:
        void onMouseEvent(EventMouse& event);
        int tabAt(int x, int y) const;
        void updateTabRects(RendererOpts* renderer);
        void refreshPageVisibility();
        void syncCssState();

        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;

        std::vector<Page> pages_;
        std::vector<SDL_Rect> tabRects_;
        std::vector<SelectionCallback> selectionCallbacks_;

        int width_ = 480;
        int height_ = 280;
        int activePage_ = -1;
        int hoveredTab_ = -1;

        SurfaceStyle surface_;
        GeometryStyle geometry_;
        Padding padding_;
        TextStyle labelStyle_;

        SDL_Color tabBarColor_{12, 18, 36, 180};
        SDL_Color tabBackground_{26, 36, 66, 255};
        SDL_Color tabHoverBackground_{40, 56, 96, 255};
        SDL_Color tabActiveBackground_{79, 110, 247, 255};
        SDL_Color tabBorderColor_{70, 84, 118, 255};
        SDL_Color tabTextColor_{200, 210, 240, 255};
        SDL_Color tabActiveTextColor_{255, 255, 255, 255};
        SDL_Color indicatorColor_{124, 156, 255, 255};

        int tabHeight_ = 44;
        int tabGap_ = 6;
        int tabPadding_ = 14;
        int tabRadius_ = 12;
        int indicatorHeight_ = 3;

        FontSpec fontSpec_{"LiberationSans", 16, "bold"};
    };

} // emgui

// Created by epax on 24.11.26.

#pragma once

#include <memory>
#include <vector>

#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"
#include "emrldgui/ui_component/components/Splitter.h"

namespace emgui {

    class Splitter;
    class StyleValues;
    class UIComponentManager;
    class Window;

    // Border layout built from a nested chain of owned splitters
    class DockPanel : public UIComponent {
    public:
        enum class Edge { Left, Right, Top, Bottom, Center };

        DockPanel(int x, int y, int width, int height);
        ~DockPanel() override;

        // Splitters are owned here and their listeners capture this
        DockPanel(const DockPanel&) = delete;
        DockPanel& operator=(const DockPanel&) = delete;
        DockPanel(DockPanel&&) = delete;
        DockPanel& operator=(DockPanel&&) = delete;

        std::string getComponentName() const override {
            return "DockPanel";
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

        // Docked widgets stay externally owned; register them with the
        // UIComponentManager so styling, updates and input keep working
        void dock(
            UIComponent* widget,
            Edge edge,
            float fraction = 0.25f
        );
        void undock(UIComponent* widget);
        void clear();

        std::vector<UIComponent*> getWidgets() const;

        void setSplitterEnabled(bool enabled);
        bool isSplitterEnabled() const {
            return splitterEnabled_;
        }

    private:
        struct Docked {
            UIComponent* widget = nullptr;
            Edge edge = Edge::Center;
            float fraction = 0.25f;
        };

        // dockedIndex == -1 leaves the splitter fraction unbound
        struct Slot {
            UIComponent* component = nullptr;
            float fraction = 0.5f;
            int dockedIndex = -1;
        };

        void disposeSplitters();
        void rebuildSplitters();
        Splitter* makeSplitter(Splitter::Orientation orientation);
        UIComponent* buildChain(
            const std::vector<Slot>& slots,
            Splitter::Orientation orientation
        );
        SDL_Rect getContentRect() const;
        Edge edgeAt(int x, int y) const;
        void drawDropHighlight(RendererOpts& renderer) const;

        int width_ = 0;
        int height_ = 0;

        std::vector<Docked> docked_;
        std::vector<std::unique_ptr<Splitter>> splitters_;
        UIComponent* rootComponent_ = nullptr;

        // Captured at attach time; never fetched from a dying window
        UIComponentManager* componentManager_ = nullptr;

        SurfaceStyle surface_;
        Padding padding_;
        FocusRingStyle focusRing_;

        SDL_Color edgeHighlightColor_{53, 132, 228, 150};

        int edgeHighlightWidth_ = 4;
        int gap_ = 4;
        int minPaneSize_ = 24;
        bool splitterEnabled_ = true;
    };

} // emgui

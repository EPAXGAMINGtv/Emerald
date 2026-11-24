// Created by epax on 24.11.26.

#pragma once

#include "emrldgui/event/EventManager.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

#include <string>
#include <utility>
#include <vector>

namespace emgui {

    class ContextMenu;
    class StyleValues;
    class Window;

    // Horizontal strip of labels that opens ContextMenu popups below the bar
    class MenuBar : public UIComponent {
    public:
        struct MenuEntry {
            std::string label;
            ContextMenu* menu = nullptr;
        };

        MenuBar(int x, int y, int width, int height);
        ~MenuBar() override;

        MenuBar(const MenuBar&) = delete;
        MenuBar& operator=(const MenuBar&) = delete;
        MenuBar(MenuBar&&) = delete;
        MenuBar& operator=(MenuBar&&) = delete;

        std::string getComponentName() const override {
            return "MenuBar";
        }

        void update() override;
        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        void addMenu(std::string label, ContextMenu* menu) {
            MenuEntry entry;
            entry.label = std::move(label);
            entry.menu = menu;
            entries_.push_back(std::move(entry));
        }

        const std::vector<MenuEntry>& getEntries() const {
            return entries_;
        }

        SDL_Point getLayoutSize() const override {
            return {width_, height_};
        }

    private:
        void onMouseEvent(EventMouse& event);
        void closeActiveMenu();
        static bool containsPoint(const SDL_Rect& rect, int x, int y);
        SDL_Rect getBarRect() const;
        SDL_Rect getItemRect(int index, RendererOpts& renderer) const;
        int getItemIndexAt(int x, int y) const;

        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;

        std::vector<MenuEntry> entries_;
        ContextMenu* activeMenu_ = nullptr;
        int hoveredIndex_ = -1;
        int activeIndex_ = -1;
        int width_ = 320;
        int height_ = 40;

        SDL_Color backgroundColor_{246, 245, 244, 255};
        SDL_Color barColor_{255, 255, 255, 255};
        SDL_Color borderColor_{223, 223, 223, 255};
        SDL_Color textColor_{36, 36, 36, 255};
        SDL_Color hoverBackgroundColor_{216, 233, 251, 255};
        SDL_Color activeBackgroundColor_{53, 132, 228, 255};
        SDL_Color hoverTextColor_{36, 36, 36, 255};
        SDL_Color activeTextColor_{255, 255, 255, 255};

        int borderWidth_ = 1;
        int borderRadius_ = 8;
        int itemSpacing_ = 6;
        int itemPadding_ = 12;

        Padding padding_;
        ShadowStyle shadow_;
        FontSpec fontSpec_{"LiberationSans", 15, "normal"};
    };

} // emgui

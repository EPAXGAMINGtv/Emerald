// Created by epax on 24.11.26.

#pragma once

#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventManager.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace emgui {

    class StyleValues;

    // Overlay popup with actions, separators, check marks and nested menus
    class ContextMenu : public UIComponent {
    public:
        struct MenuItem {
            std::string text;
            std::function<void(ContextMenu&)> action;
            ContextMenu* submenu = nullptr;
            bool separator = false;
            bool enabled = true;
            bool checked = false;
        };

        ContextMenu() = default;
        ~ContextMenu() override;

        ContextMenu(const ContextMenu&) = delete;
        ContextMenu& operator=(const ContextMenu&) = delete;
        ContextMenu(ContextMenu&&) = delete;
        ContextMenu& operator=(ContextMenu&&) = delete;

        std::string getComponentName() const override {
            return "ContextMenu";
        }

        // The popup draws in the overlay pass, never in the widget pass
        void render(RendererOpts& renderer) override;
        void renderOverlay(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        void addItem(std::string text, std::function<void(ContextMenu&)> action) {
            MenuItem item;
            item.text = std::move(text);
            item.action = std::move(action);
            items_.push_back(std::move(item));
        }

        void addSeparator() {
            MenuItem item;
            item.separator = true;
            items_.push_back(std::move(item));
        }

        void addSubmenu(std::string text, ContextMenu* submenu) {
            MenuItem item;
            item.text = std::move(text);
            item.submenu = submenu;
            items_.push_back(std::move(item));
        }

        void setItems(std::vector<MenuItem> items) {
            items_ = std::move(items);
            hoveredIndex_ = -1;
        }

        const std::vector<MenuItem>& getItems() const {
            return items_;
        }

        void showAt(int x, int y);
        void close();
        void closeAll();
        bool isOpen() const {
            return open_;
        }

        void setParent(ContextMenu* parent) {
            parent_ = parent;
        }

        ContextMenu* getParent() const {
            return parent_;
        }

    private:
        using Action = std::function<void(ContextMenu&)>;

        void onMouseEvent(EventMouse& event);
        void onKeyboardEvent(EventKeyboard& event);
        void closeAllImpl(std::vector<ContextMenu*>& visited);
        void runAction(const Action& action);
        static bool containsPoint(const SDL_Rect& rect, int x, int y);
        SDL_Rect getPopupRect(RendererOpts& renderer) const;
        int getItemIndexAt(int x, int y) const;
        bool isPointInSubmenuTree(int x, int y) const;
        int itemHeightAt(std::size_t index) const;

        EventManager::ListenerId mouseListener_ = EventManager::kInvalidListener;
        EventManager::ListenerId keyboardListener_ = EventManager::kInvalidListener;

        std::vector<MenuItem> items_;
        ContextMenu* parent_ = nullptr;
        ContextMenu* activeSubmenu_ = nullptr;
        int hoveredIndex_ = -1;
        bool open_ = false;

        SDL_Color backgroundColor_{255, 255, 255, 250};
        SDL_Color borderColor_{223, 223, 223, 255};
        SDL_Color textColor_{36, 36, 36, 255};
        SDL_Color itemTextColor_{36, 36, 36, 255};
        SDL_Color itemHoverColor_{216, 233, 251, 255};
        SDL_Color itemHoverTextColor_{36, 36, 36, 255};
        SDL_Color itemSelectedColor_{53, 132, 228, 255};
        SDL_Color itemSelectedTextColor_{255, 255, 255, 255};
        SDL_Color itemDisabledColor_{130, 135, 142, 255};
        SDL_Color separatorColor_{223, 223, 223, 255};
        SDL_Color arrowColor_{98, 105, 114, 255};
        SDL_Color checkColor_{53, 132, 228, 255};

        int borderWidth_ = 1;
        int borderRadius_ = 8;
        int itemRadius_ = 6;
        int itemHeight_ = 34;
        int separatorHeight_ = 9;
        int itemPadding_ = 10;
        int menuWidth_ = 220;

        Padding padding_;
        ShadowStyle shadow_;
        FontSpec fontSpec_{"LiberationSans", 15, "normal"};
    };

} // emgui

// Created by epax on 24.11.26.

#pragma once

#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventManager.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/components/ScrollArea.h"

#include <functional>
#include <string>
#include <vector>

namespace emgui {

    class StyleValues;

    // Scrollable, virtualized list of text items with keyboard selection
    class ListView : public ScrollArea {
    public:
        using SelectionAction = std::function<void(ListView&)>;

        ListView(int x = 0, int y = 0, int width = 240, int height = 240);
        ~ListView() override;

        ListView(const ListView&) = delete;
        ListView& operator=(const ListView&) = delete;
        ListView(ListView&&) = delete;
        ListView& operator=(ListView&&) = delete;

        std::string getComponentName() const override {
            return "ListView";
        }

        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        void setItems(std::vector<std::string> items);
        const std::vector<std::string>& getItems() const {
            return items_;
        }

        void setSelectedIndex(int index);
        int getSelectedIndex() const {
            return selectedIndex_;
        }
        std::string getSelectedText() const;

        void onSelectionChanged(SelectionAction action);

    private:
        void onMouseEvent(EventMouse& event);
        void onKeyEvent(EventKeyboard& event);
        void drawItems(
            RendererOpts& renderer,
            const SDL_Rect& viewport,
            int scrollY
        );

        void ensureVisible(int index);
        void runSelectionActions();

        EventManager::ListenerId mouseListener_ = EventManager::kInvalidListener;
        EventManager::ListenerId keyListener_ = EventManager::kInvalidListener;

        std::vector<std::string> items_;
        int selectedIndex_ = -1;
        int hoveredIndex_ = -1;

        SDL_Color itemTextColor_{235, 240, 255, 255};
        SDL_Color itemHoverColor_{45, 116, 218, 255};
        SDL_Color itemHoverTextColor_{255, 255, 255, 255};
        SDL_Color itemSelectedColor_{31, 157, 85, 255};
        SDL_Color itemSelectedTextColor_{255, 255, 255, 255};

        int itemHeight_ = 34;
        int itemPadding_ = 10;
        int itemRadius_ = 6;
        int padding_ = 4;

        FontSpec fontSpec_{"LiberationSans", 16, "normal"};
        std::vector<SelectionAction> selectionActions_;
    };

} // emgui

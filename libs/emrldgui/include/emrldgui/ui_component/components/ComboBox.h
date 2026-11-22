// Created by epax on 22.11.26.

#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "emrldgui/Window.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {

    class StyleValues;

    // Dropdown with a selectable popup list, hover highlight and actions
    class ComboBox : public UIComponent {
    public:
        using SelectionAction = std::function<void(ComboBox&)>;

        ComboBox(
            std::vector<std::string> items,
            int selectedIndex = 0,
            int x = 0,
            int y = 0,
            int width = 240,
            int height = 48
        );
        ~ComboBox() override;

        ComboBox(const ComboBox&) = delete;
        ComboBox& operator=(const ComboBox&) = delete;
        ComboBox(ComboBox&&) = delete;
        ComboBox& operator=(ComboBox&&) = delete;

        std::string getComponentName() const override {
            return "ComboBox";
        }

        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        void onSelectionChanged(SelectionAction action);

        void setItems(std::vector<std::string> items);
        const std::vector<std::string>& getItems() const {
            return items_;
        }

        void setSelectedIndex(int index);
        int getSelectedIndex() const {
            return selectedIndex_;
        }
        std::string getSelectedText() const;

        void setPlaceholder(std::string placeholder) {
            placeholder_ = std::move(placeholder);
        }

        const std::string& getPlaceholder() const {
            return placeholder_;
        }

        void setEnabled(bool enabled);
        bool isEnabled() const {
            return enabled_;
        }

        void setOpen(bool open);
        bool isOpen() const {
            return open_;
        }

    private:
        void onMouseEvent(EventMouse& event);
        bool containsPoint(const SDL_Rect& rect, int x, int y) const;
        SDL_Rect getBoxRect() const;
        SDL_Rect getPopupRect() const;
        int getItemIndexAt(int x, int y) const;
        void runSelectionActions();

        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;

        std::vector<std::string> items_;
        std::string placeholder_ = "Select...";
        int selectedIndex_ = 0;
        int hoveredIndex_ = -1;
        bool hoveredBox_ = false;
        bool open_ = false;
        bool enabled_ = true;

        int width_ = 240;
        int height_ = 48;

        SDL_Color backgroundColor_{30, 30, 46, 255};
        SDL_Color backgroundHoverColor_{37, 37, 64, 255};
        SDL_Color disabledBackgroundColor_{55, 55, 55, 255};
        SDL_Color borderColor_{85, 85, 105, 255};
        SDL_Color borderHoverColor_{110, 168, 254, 255};
        SDL_Color textColor_{240, 244, 255, 255};
        SDL_Color disabledTextColor_{180, 180, 180, 255};
        SDL_Color placeholderColor_{135, 140, 160, 255};

        SDL_Color popupBackgroundColor_{24, 24, 38, 250};
        SDL_Color popupBorderColor_{110, 168, 254, 255};
        SDL_Color itemHoverColor_{45, 116, 218, 255};
        SDL_Color itemSelectedColor_{31, 157, 85, 255};
        SDL_Color itemTextColor_{235, 240, 255, 255};
        SDL_Color itemSelectedTextColor_{255, 255, 255, 255};
        SDL_Color arrowColor_{180, 190, 220, 255};

        int borderWidth_ = 2;
        int borderRadius_ = 8;
        int padding_ = 10;
        int arrowSize_ = 8;
        int itemHeight_ = 36;
        int itemPadding_ = 10;
        int itemRadius_ = 6;
        int maxVisibleItems_ = 6;

        ShadowStyle shadow_;
        FocusRingStyle focusRing_;

        FontSpec fontSpec_{"LiberationSans", 20, "normal"};
        std::vector<SelectionAction> selectionActions_;
    };

} // emgui

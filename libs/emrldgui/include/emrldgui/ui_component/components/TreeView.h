// Created by epax on 24.11.26.

#pragma once

#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventManager.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UiStyle.h"
#include "emrldgui/ui_component/components/ScrollArea.h"

#include <functional>
#include <string>
#include <vector>

namespace emgui {

    class StyleValues;

    // Scrollable, virtualized hierarchy with expandable nodes and keyboard selection
    class TreeView : public ScrollArea {
    public:
        struct Node {
            std::string id;
            std::string label;
            std::vector<Node> children;
            bool expanded = false;
        };

        using SelectionAction = std::function<void(TreeView&, const std::string&)>;

        TreeView(int x = 0, int y = 0, int width = 260, int height = 300);
        ~TreeView() override;

        TreeView(const TreeView&) = delete;
        TreeView& operator=(const TreeView&) = delete;
        TreeView(TreeView&&) = delete;
        TreeView& operator=(TreeView&&) = delete;

        std::string getComponentName() const override {
            return "TreeView";
        }

        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;
        void layout() override;
        void render(RendererOpts& renderer) override;

        void setRoots(std::vector<Node> roots);
        const std::vector<Node>& getRoots() const {
            return roots_;
        }

        Node* findNode(const std::string& id);
        bool selectNode(const std::string& id);
        std::string getSelectedId() const {
            return selectedId_;
        }

        void expandAll();
        void collapseAll();
        void setExpanded(const std::string& id, bool expanded);
        bool isExpanded(const std::string& id) const;

        void onSelectionChanged(SelectionAction action);

    private:
        struct FlatRow {
            Node* node = nullptr;
            Node* parent = nullptr;
            int depth = 0;
        };

        void onMouseEvent(EventMouse& event);
        void onKeyEvent(EventKeyboard& event);
        void drawRows(
            RendererOpts& renderer,
            const SDL_Rect& viewport,
            int scrollY
        );
        void collectRows(
            std::vector<Node>& nodes,
            Node* parent,
            int depth,
            std::vector<FlatRow>& out
        );
        void refreshRows();
        void updateContentSize();
        void ensureVisible(int rowIndex);
        int selectedRowIndex() const;
        void selectRowIndex(int rowIndex);
        void runSelectionActions(const std::string& id);

        EventManager::ListenerId mouseListener_ = EventManager::kInvalidListener;
        EventManager::ListenerId keyListener_ = EventManager::kInvalidListener;

        std::vector<Node> roots_;
        std::vector<FlatRow> rows_;
        std::string selectedId_;
        int hoveredRow_ = -1;

        SDL_Color textColor_{235, 240, 255, 255};
        SDL_Color textSelectedColor_{255, 255, 255, 255};
        SDL_Color rowHoverColor_{45, 116, 218, 255};
        SDL_Color rowSelectedColor_{31, 157, 85, 255};
        SDL_Color triangleColor_{110, 168, 254, 255};
        SDL_Color iconColor_{110, 168, 254, 255};

        int rowHeight_ = 30;
        int indent_ = 18;
        int triangleSize_ = 10;
        int iconSize_ = 6;
        int iconGap_ = 8;
        int radius_ = 8;

        Padding padding_;
        ShadowStyle shadow_;
        FocusRingStyle focusRing_;
        FontSpec fontSpec_{"LiberationSans", 15, "normal"};

        std::vector<SelectionAction> selectionActions_;
    };

} // emgui

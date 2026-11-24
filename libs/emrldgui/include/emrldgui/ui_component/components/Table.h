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

    // Virtualized data grid with a sticky, clickable header and cell elision
    class Table : public ScrollArea {
    public:
        struct Column {
            std::string key;
            std::string title;
            int width = 120;
            bool sortable = true;
        };

        using Row = std::vector<std::string>;
        using RowSelectedAction = std::function<void(Table&, int)>;
        using ColumnSortedAction = std::function<
            void(Table&, const std::string&, bool)
        >;

        Table(int x = 0, int y = 0, int width = 480, int height = 300);
        ~Table() override;

        Table(const Table&) = delete;
        Table& operator=(const Table&) = delete;
        Table(Table&&) = delete;
        Table& operator=(Table&&) = delete;

        std::string getComponentName() const override {
            return "Table";
        }

        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;
        void layout() override;
        void render(RendererOpts& renderer) override;

        void setColumns(std::vector<Column> columns);
        const std::vector<Column>& getColumns() const {
            return columns_;
        }

        void setRows(std::vector<Row> rows);
        const std::vector<Row>& getRows() const {
            return rows_;
        }

        void setRowHeight(int height);
        int getRowHeight() const {
            return rowHeight_;
        }

        void sortBy(const std::string& key, bool ascending);
        std::string getSortKey() const {
            return sortKey_;
        }
        bool isSortAscending() const {
            return sortAscending_;
        }

        void selectRow(int row);
        int getSelectedRow() const {
            return selectedRow_;
        }

        void onRowSelected(RowSelectedAction action);
        void onColumnSorted(ColumnSortedAction action);

    private:
        void onMouseEvent(EventMouse& event);
        void onKeyEvent(EventKeyboard& event);
        void drawContent(
            RendererOpts& renderer,
            const SDL_Rect& viewport,
            int scrollX,
            int scrollY
        );
        void drawBody(
            RendererOpts& renderer,
            const SDL_Rect& viewport,
            int scrollX,
            int scrollY
        );
        void drawHeader(
            RendererOpts& renderer,
            const SDL_Rect& viewport,
            int scrollX
        );
        void updateContentSize();
        void ensureVisible(int rowIndex);
        void runRowActions(int rowIndex);
        void runSortActions(const std::string& key, bool ascending);

        EventManager::ListenerId mouseListener_ = EventManager::kInvalidListener;
        EventManager::ListenerId keyListener_ = EventManager::kInvalidListener;

        std::vector<Column> columns_;
        std::vector<Row> rows_;
        int selectedRow_ = -1;
        int hoveredRow_ = -1;
        std::string sortKey_;
        bool sortAscending_ = true;

        SDL_Color headerBackgroundColor_{246, 245, 244, 255};
        SDL_Color headerTextColor_{36, 36, 36, 255};
        SDL_Color headerBorderColor_{223, 223, 223, 255};
        SDL_Color gridColor_{223, 223, 223, 255};
        SDL_Color rowColor_{255, 255, 255, 255};
        SDL_Color rowAlternateColor_{246, 245, 244, 255};
        SDL_Color rowHoverColor_{216, 233, 251, 255};
        SDL_Color rowSelectedColor_{53, 132, 228, 255};
        SDL_Color textColor_{36, 36, 36, 255};
        SDL_Color textSelectedColor_{255, 255, 255, 255};
        SDL_Color sortArrowColor_{98, 105, 114, 255};

        int headerHeight_ = 34;
        int headerBorderWidth_ = 1;
        int rowHeight_ = 30;
        int cellPadding_ = 8;
        int gridWidth_ = 1;
        int sortArrowSize_ = 5;
        int radius_ = 8;

        Padding padding_;
        ShadowStyle shadow_;
        FocusRingStyle focusRing_;
        FontSpec fontSpec_{"LiberationSans", 14, "normal"};

        std::vector<RowSelectedAction> rowActions_;
        std::vector<ColumnSortedAction> sortActions_;
    };

} // emgui

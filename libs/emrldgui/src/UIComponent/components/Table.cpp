// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/Table.h"

#include "emrldgui/Window.h"
#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/style/StyleValues.h"
#include "emrldgui/ui_component/UIComponentManager.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace emgui {

    namespace {

        bool inside(const SDL_Rect& rect, int x, int y) {
            return x >= rect.x && x < rect.x + rect.w &&
                   y >= rect.y && y < rect.y + rect.h;
        }

        std::string elideText(
            RendererOpts& renderer,
            const std::string& text,
            int maxWidth,
            const FontSpec& fontSpec
        ) {
            if (maxWidth <= 0) {
                return {};
            }

            if (renderer.measureText(text, fontSpec).x <= maxWidth) {
                return text;
            }

            const std::string ellipsis = "...";
            const int ellipsisWidth = renderer.measureText(
                ellipsis,
                fontSpec
            ).x;

            if (ellipsisWidth >= maxWidth) {
                std::string dots;
                for (int index = 0; index < 3; ++index) {
                    const std::string candidate = dots + '.';
                    if (renderer.measureText(candidate, fontSpec).x > maxWidth) {
                        break;
                    }
                    dots.push_back('.');
                }
                return dots;
            }

            std::string result;
            for (const char character : text) {
                const std::string candidate = result + character;
                if (renderer.measureText(candidate + ellipsis, fontSpec).x >
                    maxWidth) {
                    break;
                }
                result.push_back(character);
            }

            return result + ellipsis;
        }

        void drawSortArrow(
            RendererOpts& renderer,
            const SDL_Point& origin,
            int size,
            bool ascending,
            const SDL_Color& color
        ) {
            const int half = std::max(1, size);

            if (ascending) {
                renderer.drawLine(
                    {origin.x, origin.y + half},
                    {origin.x - half, origin.y},
                    color
                );
                renderer.drawLine(
                    {origin.x, origin.y + half},
                    {origin.x + half, origin.y},
                    color
                );
                renderer.drawLine(
                    {origin.x - half, origin.y},
                    {origin.x + half, origin.y},
                    color
                );
                return;
            }

            renderer.drawLine(
                {origin.x, origin.y},
                {origin.x - half, origin.y + half},
                color
            );
            renderer.drawLine(
                {origin.x, origin.y},
                {origin.x + half, origin.y + half},
                color
            );
            renderer.drawLine(
                {origin.x - half, origin.y + half},
                {origin.x + half, origin.y + half},
                color
            );
        }

    } // namespace

    Table::Table(int x, int y, int width, int height)
        : ScrollArea(x, y, width, height) {
        setFocusable(true);
        padding_.top = 4;
        padding_.right = 4;
        padding_.bottom = 4;
        padding_.left = 4;

        setContentRenderer(
            [this](
                RendererOpts& renderer,
                const SDL_Rect& viewport,
                int scrollX,
                int scrollY
            ) {
                drawContent(renderer, viewport, scrollX, scrollY);
            }
        );
        updateContentSize();
    }

    Table::~Table() {
        if (window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            if (mouseListener_ != EventManager::kInvalidListener) {
                manager->removeListener(mouseListener_);
            }
            if (keyListener_ != EventManager::kInvalidListener) {
                manager->removeListener(keyListener_);
            }
        }
    }

    void Table::applyStyle(const StyleValues& values) {
        ScrollArea::applyStyle(values);

        headerBackgroundColor_ = values.getColor(
            "header-background-color",
            headerBackgroundColor_
        );
        headerTextColor_ = values.getColor(
            "header-text-color",
            headerTextColor_
        );
        headerBorderColor_ = values.getColor(
            "header-border-color",
            headerBorderColor_
        );
        gridColor_ = values.getColor("grid-color", gridColor_);
        rowColor_ = values.getColor("row-color", rowColor_);
        rowAlternateColor_ = values.getColor(
            "row-alternate-color",
            rowAlternateColor_
        );
        rowHoverColor_ = values.getColor("row-hover-color", rowHoverColor_);
        rowSelectedColor_ = values.getColor(
            "row-selected-color",
            rowSelectedColor_
        );
        textColor_ = values.getColor("text-color", textColor_);
        textSelectedColor_ = values.getColor(
            "text-selected-color",
            textSelectedColor_
        );
        sortArrowColor_ = values.getColor(
            "sort-arrow-color",
            sortArrowColor_
        );

        headerHeight_ = std::max(16, values.getInt("header-height", headerHeight_));
        headerBorderWidth_ = std::max(
            0,
            values.getInt("header-border-width", headerBorderWidth_)
        );
        rowHeight_ = std::max(12, values.getInt("row-height", rowHeight_));
        cellPadding_ = std::max(0, values.getInt("cell-padding", cellPadding_));
        gridWidth_ = std::clamp(
            values.getInt("grid-width", gridWidth_),
            0,
            4
        );
        sortArrowSize_ = std::max(
            2,
            values.getInt("sort-arrow-size", sortArrowSize_)
        );
        radius_ = std::max(0, values.getInt("radius", radius_));

        padding_.apply(values, 4);
        shadow_.apply(values);
        focusRing_.apply(values);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        updateContentSize();
    }

    void Table::onWindowAttached(Window& window) {
        ScrollArea::onWindowAttached(window);

        SDL_Window* sdlWindow = window.getWindow();

        if (sdlWindow == nullptr) {
            return;
        }

        if (mouseListener_ == EventManager::kInvalidListener) {
            mouseListener_ = EventManager::registerEvent(
                sdlWindow,
                EventMouse(sdlWindow),
                [this](EventMouse& event) {
                    onMouseEvent(event);
                }
            );
        }

        if (keyListener_ == EventManager::kInvalidListener) {
            keyListener_ = EventManager::registerEvent(
                sdlWindow,
                EventKeyboard(sdlWindow),
                [this](EventKeyboard& event) {
                    onKeyEvent(event);
                }
            );
        }
    }

    void Table::layout() {
        ScrollArea::layout();
        updateContentSize();
    }

    void Table::render(RendererOpts& renderer) {
        const SDL_Point position = getPosition();
        const SDL_Point size = getLayoutSize();
        const SDL_Rect rect{position.x, position.y, size.x, size.y};

        if (rect.w <= 0 || rect.h <= 0) {
            return;
        }

        shadow_.draw(renderer, rect, radius_);
        ScrollArea::render(renderer);
        focusRing_.draw(renderer, rect, radius_, isFocused());
    }

    void Table::setColumns(std::vector<Column> columns) {
        columns_ = std::move(columns);

        const bool sortKeyExists = std::any_of(
            columns_.begin(),
            columns_.end(),
            [this](const Column& column) {
                return column.key == sortKey_;
            }
        );

        if (!sortKeyExists) {
            sortKey_.clear();
            sortAscending_ = true;
        }

        updateContentSize();
    }

    void Table::setRows(std::vector<Row> rows) {
        rows_ = std::move(rows);

        if (selectedRow_ >= static_cast<int>(rows_.size())) {
            selectedRow_ = -1;
            setSelected(false);
        }

        hoveredRow_ = -1;
        updateContentSize();
    }

    void Table::setRowHeight(int height) {
        rowHeight_ = std::max(12, height);
        updateContentSize();
    }

    void Table::sortBy(const std::string& key, bool ascending) {
        int columnIndex = -1;

        for (std::size_t index = 0; index < columns_.size(); ++index) {
            if (columns_[index].key == key) {
                columnIndex = static_cast<int>(index);
                break;
            }
        }

        if (columnIndex < 0 ||
            !columns_[static_cast<std::size_t>(columnIndex)].sortable) {
            return;
        }

        sortKey_ = key;
        sortAscending_ = ascending;

        const std::size_t valueIndex = static_cast<std::size_t>(columnIndex);
        std::stable_sort(
            rows_.begin(),
            rows_.end(),
            [valueIndex, ascending](const Row& left, const Row& right) {
                const std::string leftValue = valueIndex < left.size()
                    ? left[valueIndex]
                    : std::string{};
                const std::string rightValue = valueIndex < right.size()
                    ? right[valueIndex]
                    : std::string{};
                return ascending
                    ? leftValue < rightValue
                    : leftValue > rightValue;
            }
        );

        runSortActions(key, ascending);
    }

    void Table::selectRow(int row) {
        if (row < 0 || row >= static_cast<int>(rows_.size())) {
            row = -1;
        }

        if (selectedRow_ == row) {
            return;
        }

        selectedRow_ = row;
        setSelected(row >= 0);

        if (row >= 0) {
            ensureVisible(row);
        }

        runRowActions(row);
    }

    void Table::onRowSelected(RowSelectedAction action) {
        if (action) {
            rowActions_.push_back(std::move(action));
        }
    }

    void Table::onColumnSorted(ColumnSortedAction action) {
        if (action) {
            sortActions_.push_back(std::move(action));
        }
    }

    void Table::updateContentSize() {
        int width = padding_.horizontal();
        for (const Column& column : columns_) {
            width += std::max(0, column.width);
        }

        const int height = headerHeight_ +
                           static_cast<int>(rows_.size()) * rowHeight_ +
                           padding_.bottom;
        setContentSize(width, height);
    }

    void Table::drawContent(
        RendererOpts& renderer,
        const SDL_Rect& viewport,
        int scrollX,
        int scrollY
    ) {
        drawBody(renderer, viewport, scrollX, scrollY);
        // The header is drawn last so it stays pinned while the body scrolls.
        drawHeader(renderer, viewport, scrollX);
    }

    void Table::drawBody(
        RendererOpts& renderer,
        const SDL_Rect& viewport,
        int scrollX,
        int scrollY
    ) {
        if (rows_.empty()) {
            return;
        }

        const int first = std::max(0, scrollY / rowHeight_ - 1);
        const int last = std::min(
            static_cast<int>(rows_.size()) - 1,
            (scrollY + viewport.h) / rowHeight_ + 1
        );
        const int columnsX = viewport.x + padding_.left - scrollX;

        int totalWidth = 0;
        for (const Column& column : columns_) {
            totalWidth += std::max(0, column.width);
        }

        for (int index = first; index <= last; ++index) {
            const int rowTop = viewport.y + headerHeight_ +
                               index * rowHeight_ - scrollY;
            if (rowTop + rowHeight_ <= viewport.y ||
                rowTop >= viewport.y + viewport.h) {
                continue;
            }

            const SDL_Rect rowRect{viewport.x, rowTop, viewport.w, rowHeight_};
            const SDL_Color& baseColor = index % 2 == 0
                ? rowColor_
                : rowAlternateColor_;
            renderer.fillRect(rowRect, baseColor);

            if (index == selectedRow_) {
                renderer.fillRect(rowRect, rowSelectedColor_);
            } else if (index == hoveredRow_) {
                renderer.fillRect(rowRect, rowHoverColor_);
            }

            focusRing_.draw(renderer, rowRect, 0, isFocused() &&
                index == selectedRow_);
        }

        if (gridWidth_ > 0) {
            const int bodyTop = viewport.y + headerHeight_;
            const int bodyBottom = viewport.y + viewport.h;

            for (int line = 0; line < gridWidth_; ++line) {
                int separatorX = columnsX + line;
                for (const Column& column : columns_) {
                    separatorX += std::max(0, column.width);
                    renderer.drawLine(
                        {separatorX, bodyTop},
                        {separatorX, bodyBottom},
                        gridColor_
                    );
                }
            }

            for (int index = first; index <= last; ++index) {
                const int rowTop = viewport.y + headerHeight_ +
                                   index * rowHeight_ - scrollY;
                if (rowTop + rowHeight_ <= viewport.y ||
                    rowTop >= viewport.y + viewport.h) {
                    continue;
                }

                for (int line = 0; line < gridWidth_; ++line) {
                    const int lineY = rowTop + rowHeight_ - 1 - line;
                    renderer.drawLine(
                        {columnsX, lineY},
                        {columnsX + totalWidth, lineY},
                        gridColor_
                    );
                }
            }
        }

        for (int index = first; index <= last; ++index) {
            const int rowTop = viewport.y + headerHeight_ +
                               index * rowHeight_ - scrollY;
            if (rowTop + rowHeight_ <= viewport.y ||
                rowTop >= viewport.y + viewport.h) {
                continue;
            }

            const SDL_Color& foreground = index == selectedRow_
                ? textSelectedColor_
                : textColor_;
            const Row& row = rows_[static_cast<std::size_t>(index)];
            int cellX = columnsX;

            for (std::size_t columnIndex = 0;
                 columnIndex < columns_.size();
                 ++columnIndex) {
                const Column& column = columns_[columnIndex];
                const int columnWidth = std::max(0, column.width);

                if (columnWidth > 0 &&
                    cellX + columnWidth > viewport.x &&
                    cellX < viewport.x + viewport.w) {
                    const std::string value = columnIndex < row.size()
                        ? row[columnIndex]
                        : std::string{};
                    const int textWidth = std::max(
                        0,
                        columnWidth - 2 * cellPadding_
                    );
                    const std::string text = elideText(
                        renderer,
                        value,
                        textWidth,
                        fontSpec_
                    );
                    const SDL_Point textSize = renderer.measureText(
                        text,
                        fontSpec_
                    );
                    renderer.drawText(
                        text,
                        {cellX + cellPadding_,
                         rowTop + (rowHeight_ - textSize.y) / 2},
                        foreground,
                        fontSpec_
                    );
                }

                cellX += columnWidth;
            }
        }
    }

    void Table::drawHeader(
        RendererOpts& renderer,
        const SDL_Rect& viewport,
        int scrollX
    ) {
        const int headerHeight = std::min(headerHeight_, viewport.h);
        const SDL_Rect headerRect{
            viewport.x,
            viewport.y,
            viewport.w,
            headerHeight
        };
        renderer.fillRect(headerRect, headerBackgroundColor_);

        for (int line = 0; line < headerBorderWidth_; ++line) {
            const int borderY = viewport.y + headerHeight - 1 - line;
            if (borderY < viewport.y) {
                break;
            }
            renderer.drawLine(
                {viewport.x, borderY},
                {viewport.x + viewport.w, borderY},
                headerBorderColor_
            );
        }

        int separatorX = viewport.x + padding_.left - scrollX;
        for (const Column& column : columns_) {
            separatorX += std::max(0, column.width);
            renderer.drawLine(
                {separatorX, viewport.y},
                {separatorX, viewport.y + headerHeight},
                headerBorderColor_
            );
        }

        int cellX = viewport.x + padding_.left - scrollX;
        for (const Column& column : columns_) {
            const int columnWidth = std::max(0, column.width);

            if (columnWidth > 0 &&
                cellX + columnWidth > viewport.x &&
                cellX < viewport.x + viewport.w) {
                const bool sorted = column.sortable &&
                                    column.key == sortKey_;
                const int arrowSpace = sorted
                    ? 2 * sortArrowSize_ + cellPadding_
                    : 0;
                const std::string title = elideText(
                    renderer,
                    column.title,
                    std::max(0, columnWidth - 2 * cellPadding_ - arrowSpace),
                    fontSpec_
                );
                const SDL_Point textSize = renderer.measureText(
                    title,
                    fontSpec_
                );
                renderer.drawText(
                    title,
                    {cellX + cellPadding_,
                     viewport.y + (headerHeight - textSize.y) / 2},
                    headerTextColor_,
                    fontSpec_
                );

                if (sorted) {
                    drawSortArrow(
                        renderer,
                        {cellX + columnWidth - cellPadding_ - sortArrowSize_ - 1,
                         viewport.y + headerHeight / 2},
                        sortArrowSize_,
                        sortAscending_,
                        sortArrowColor_
                    );
                }
            }

            cellX += columnWidth;
        }
    }

    void Table::onMouseEvent(EventMouse& event) {
        if (event.isWheel()) {
            return;
        }

        const SDL_Point size = getLayoutSize();
        const SDL_Point position = getPosition();
        const SDL_Rect viewport{position.x, position.y, size.x, size.y};
        const bool hovering = inside(viewport, event.getX(), event.getY());

        setHovered(hovering);

        if (!hovering) {
            hoveredRow_ = -1;
            return;
        }

        const bool headerHit = event.getY() < viewport.y + headerHeight_;
        hoveredRow_ = -1;

        if (!headerHit && !rows_.empty()) {
            const int contentY = event.getY() - viewport.y - headerHeight_ +
                                 getScrollY();
            const int rowIndex = contentY / rowHeight_;

            if (rowIndex >= 0 && rowIndex < static_cast<int>(rows_.size())) {
                hoveredRow_ = rowIndex;
            }
        }

        if (!event.isPressed() || event.getButton() != SDL_BUTTON_LEFT) {
            return;
        }

        if (window_ != nullptr && window_->getWindow() != nullptr) {
            window_->getUIComponentManager().setFocusedComponent(this);
        }

        if (headerHit) {
            const int contentX = event.getX() - viewport.x - padding_.left +
                                 getScrollX();
            int columnStart = 0;

            for (const Column& column : columns_) {
                const int columnWidth = std::max(0, column.width);

                if (contentX >= columnStart &&
                    contentX < columnStart + columnWidth) {
                    if (column.sortable) {
                        const bool ascending = !(sortKey_ == column.key &&
                                                 sortAscending_);
                        sortBy(column.key, ascending);
                    }
                    break;
                }

                columnStart += columnWidth;
            }

            return;
        }

        if (hoveredRow_ >= 0) {
            selectRow(hoveredRow_);
        }
    }

    void Table::onKeyEvent(EventKeyboard& event) {
        if (!event.isPressed() || event.isRepeat() || isDisabled() ||
            !isFocused() || rows_.empty()) {
            return;
        }

        const int rowCount = static_cast<int>(rows_.size());
        int row = selectedRow_;

        switch (event.getKey()) {
            case SDLK_DOWN:
                row = row < 0 ? 0 : std::min(row + 1, rowCount - 1);
                break;
            case SDLK_UP:
                row = row < 0 ? rowCount - 1 : std::max(0, row - 1);
                break;
            case SDLK_HOME:
                row = 0;
                break;
            case SDLK_END:
                row = rowCount - 1;
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (selectedRow_ >= 0) {
                    runRowActions(selectedRow_);
                }
                break;
            default:
                return;
        }

        selectRow(row);
        event.handled = true;
    }

    void Table::ensureVisible(int rowIndex) {
        if (rowIndex < 0 || rowIndex >= static_cast<int>(rows_.size())) {
            return;
        }

        const SDL_Point size = getLayoutSize();
        const int top = headerHeight_ + rowIndex * rowHeight_;
        const int bottom = top + rowHeight_;
        const int viewTop = getScrollY() + headerHeight_;
        const int viewBottom = getScrollY() + size.y;

        if (top < viewTop) {
            setScroll(getScrollX(), std::max(0, top - headerHeight_));
        } else if (bottom > viewBottom) {
            setScroll(getScrollX(), bottom - size.y);
        }
    }

    void Table::runRowActions(int rowIndex) {
        const auto snapshot = rowActions_;

        for (const RowSelectedAction& action : snapshot) {
            try {
                action(*this, rowIndex);
            } catch (const std::exception& error) {
                spdlog::error("Table row action failed: {}", error.what());
            }
        }
    }

    void Table::runSortActions(const std::string& key, bool ascending) {
        const auto snapshot = sortActions_;

        for (const ColumnSortedAction& action : snapshot) {
            try {
                action(*this, key, ascending);
            } catch (const std::exception& error) {
                spdlog::error("Table sort action failed: {}", error.what());
            }
        }
    }

} // emgui

// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/TreeView.h"

#include "emrldgui/Window.h"
#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/style/StyleValues.h"
#include "emrldgui/ui_component/UIComponentManager.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <utility>
#include <vector>

namespace emgui {

    namespace {

        bool inside(const SDL_Rect& rect, int x, int y) {
            return x >= rect.x && x < rect.x + rect.w &&
                   y >= rect.y && y < rect.y + rect.h;
        }

        TreeView::Node* findNodeRecursive(
            std::vector<TreeView::Node>& nodes,
            const std::string& id
        ) {
            for (TreeView::Node& node : nodes) {
                if (node.id == id) {
                    return &node;
                }

                if (TreeView::Node* found = findNodeRecursive(node.children, id)) {
                    return found;
                }
            }

            return nullptr;
        }

        const TreeView::Node* findNodeRecursive(
            const std::vector<TreeView::Node>& nodes,
            const std::string& id
        ) {
            for (const TreeView::Node& node : nodes) {
                if (node.id == id) {
                    return &node;
                }

                if (const TreeView::Node* found =
                        findNodeRecursive(node.children, id)) {
                    return found;
                }
            }

            return nullptr;
        }

        bool findPath(
            std::vector<TreeView::Node>& nodes,
            const std::string& id,
            std::vector<TreeView::Node*>& path
        ) {
            for (TreeView::Node& node : nodes) {
                path.push_back(&node);

                if (node.id == id) {
                    return true;
                }

                if (findPath(node.children, id, path)) {
                    return true;
                }

                path.pop_back();
            }

            return false;
        }

        void setExpandedRecursive(
            std::vector<TreeView::Node>& nodes,
            bool expanded
        ) {
            for (TreeView::Node& node : nodes) {
                node.expanded = expanded;
                setExpandedRecursive(node.children, expanded);
            }
        }

        // RendererOpts has no polygon primitive, so triangles are built from
        // one-pixel horizontal spans.
        void drawTriangle(
            RendererOpts& renderer,
            const SDL_Point& origin,
            int size,
            bool expanded,
            const SDL_Color& color
        ) {
            const int side = std::max(1, size);

            if (expanded) {
                const int denominator = std::max(1, side - 1);

                for (int row = 0; row < side; ++row) {
                    const int span = row * side / denominator;
                    const int left = origin.x + (side - span) / 2;
                    renderer.fillRect(
                        {left, origin.y + row, span, 1},
                        color
                    );
                }

                return;
            }

            const int center = side / 2;
            const int denominator = std::max(1, center + 1);

            for (int row = 0; row < side; ++row) {
                const int distance = std::abs(row - center);
                const int span = side - distance * side / denominator;
                renderer.fillRect(
                    {origin.x, origin.y + row, span, 1},
                    color
                );
            }
        }

    } // namespace

    TreeView::TreeView(int x, int y, int width, int height)
        : ScrollArea(x, y, width, height) {
        setFocusable(true);
        padding_.top = 4;
        padding_.right = 4;
        padding_.bottom = 4;
        padding_.left = 4;

        setContentRenderer(
            [this](RendererOpts& renderer, const SDL_Rect& viewport, int, int scrollY) {
                drawRows(renderer, viewport, scrollY);
            }
        );
        refreshRows();
    }

    TreeView::~TreeView() {
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

    void TreeView::applyStyle(const StyleValues& values) {
        ScrollArea::applyStyle(values);

        textColor_ = values.getColor("text-color", textColor_);
        textSelectedColor_ = values.getColor(
            "text-selected-color",
            textSelectedColor_
        );
        rowHoverColor_ = values.getColor("row-hover-color", rowHoverColor_);
        rowSelectedColor_ = values.getColor(
            "row-selected-color",
            rowSelectedColor_
        );
        triangleColor_ = values.getColor("triangle-color", triangleColor_);
        iconColor_ = values.getColor("icon-color", iconColor_);

        rowHeight_ = std::max(12, values.getInt("row-height", rowHeight_));
        indent_ = std::max(0, values.getInt("indent", indent_));
        triangleSize_ = std::max(1, values.getInt("triangle-size", triangleSize_));
        iconSize_ = std::max(0, values.getInt("icon-size", iconSize_));
        iconGap_ = std::max(0, values.getInt("icon-gap", iconGap_));
        radius_ = std::max(0, values.getInt("radius", radius_));

        padding_.apply(values, 4);
        shadow_.apply(values);
        focusRing_.apply(values);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        updateContentSize();
    }

    void TreeView::onWindowAttached(Window& window) {
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

    void TreeView::layout() {
        ScrollArea::layout();
        updateContentSize();
    }

    void TreeView::render(RendererOpts& renderer) {
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

    void TreeView::setRoots(std::vector<Node> roots) {
        roots_ = std::move(roots);

        if (findNodeRecursive(roots_, selectedId_) == nullptr) {
            selectedId_.clear();
            setSelected(false);
        }

        refreshRows();
    }

    TreeView::Node* TreeView::findNode(const std::string& id) {
        return findNodeRecursive(roots_, id);
    }

    bool TreeView::selectNode(const std::string& id) {
        std::vector<Node*> path;
        if (!findPath(roots_, id, path)) {
            return false;
        }

        bool hierarchyChanged = false;
        for (std::size_t index = 0; index + 1 < path.size(); ++index) {
            if (!path[index]->expanded) {
                path[index]->expanded = true;
                hierarchyChanged = true;
            }
        }

        if (hierarchyChanged) {
            refreshRows();
        }

        const bool changed = selectedId_ != id;
        selectedId_ = id;
        setSelected(true);

        const int index = selectedRowIndex();
        if (index >= 0) {
            ensureVisible(index);
        }

        if (changed) {
            runSelectionActions(id);
        }

        return true;
    }

    void TreeView::expandAll() {
        setExpandedRecursive(roots_, true);
        refreshRows();
    }

    void TreeView::collapseAll() {
        setExpandedRecursive(roots_, false);
        refreshRows();
    }

    void TreeView::setExpanded(const std::string& id, bool expanded) {
        Node* node = findNodeRecursive(roots_, id);

        if (node == nullptr || node->expanded == expanded) {
            return;
        }

        node->expanded = expanded;
        refreshRows();

        const int index = selectedRowIndex();
        if (index >= 0) {
            ensureVisible(index);
        }
    }

    bool TreeView::isExpanded(const std::string& id) const {
        const Node* node = findNodeRecursive(roots_, id);
        return node != nullptr && node->expanded;
    }

    void TreeView::onSelectionChanged(SelectionAction action) {
        if (action) {
            selectionActions_.push_back(std::move(action));
        }
    }

    void TreeView::collectRows(
        std::vector<Node>& nodes,
        Node* parent,
        int depth,
        std::vector<FlatRow>& out
    ) {
        for (Node& node : nodes) {
            out.push_back({&node, parent, depth});

            if (node.expanded) {
                collectRows(node.children, &node, depth + 1, out);
            }
        }
    }

    void TreeView::refreshRows() {
        rows_.clear();
        collectRows(roots_, nullptr, 0, rows_);
        hoveredRow_ = -1;
        updateContentSize();
    }

    void TreeView::updateContentSize() {
        const SDL_Point size = getLayoutSize();
        const int height = static_cast<int>(rows_.size()) * rowHeight_ +
                           padding_.vertical();
        setContentSize(std::max(0, size.x), height);
    }

    void TreeView::drawRows(
        RendererOpts& renderer,
        const SDL_Rect& viewport,
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

        for (int index = first; index <= last; ++index) {
            const FlatRow& row = rows_[static_cast<std::size_t>(index)];
            const SDL_Rect rowRect{
                viewport.x + padding_.left,
                viewport.y + padding_.top + index * rowHeight_ - scrollY,
                std::max(0, viewport.w - padding_.horizontal()),
                rowHeight_
            };

            const bool isSelected = row.node->id == selectedId_;
            const bool isHovered = index == hoveredRow_;

            if (isSelected || isHovered) {
                renderer.fillRoundedRect(
                    rowRect,
                    4,
                    isSelected ? rowSelectedColor_ : rowHoverColor_
                );
            }

            focusRing_.draw(renderer, rowRect, 4, isFocused() && isSelected);

            int cursorX = rowRect.x + row.depth * indent_;

            if (!row.node->children.empty()) {
                const SDL_Point triangleOrigin{
                    cursorX,
                    rowRect.y + (rowHeight_ - triangleSize_) / 2
                };
                drawTriangle(
                    renderer,
                    triangleOrigin,
                    triangleSize_,
                    row.node->expanded,
                    triangleColor_
                );
            }

            cursorX += triangleSize_ + iconGap_;

            if (iconSize_ > 0 && iconColor_.a > 0) {
                renderer.fillRect(
                    {
                        cursorX,
                        rowRect.y + (rowHeight_ - iconSize_) / 2,
                        iconSize_,
                        iconSize_
                    },
                    iconColor_
                );
                cursorX += iconSize_ + iconGap_;
            }

            const SDL_Color textColor = isSelected
                ? textSelectedColor_
                : textColor_;
            const SDL_Point textSize = renderer.measureText(
                row.node->label,
                fontSpec_
            );
            renderer.drawText(
                row.node->label,
                {cursorX, rowRect.y + (rowHeight_ - textSize.y) / 2},
                textColor,
                fontSpec_
            );
        }
    }

    void TreeView::onMouseEvent(EventMouse& event) {
        if (event.isWheel()) {
            return;
        }

        const SDL_Point size = getLayoutSize();
        const SDL_Point position = getPosition();
        const SDL_Rect viewport{position.x, position.y, size.x, size.y};
        const bool hovering = inside(viewport, event.getX(), event.getY());

        setHovered(hovering);

        int index = -1;
        if (hovering && !rows_.empty()) {
            const int contentY =
                event.getY() - viewport.y - padding_.top + getScrollY();
            index = contentY / rowHeight_;

            if (index < 0 || index >= static_cast<int>(rows_.size())) {
                index = -1;
            }
        }

        hoveredRow_ = index;

        if (!hovering || !event.isPressed() ||
            event.getButton() != SDL_BUTTON_LEFT || index < 0) {
            return;
        }

        if (window_ != nullptr && window_->getWindow() != nullptr) {
            window_->getUIComponentManager().setFocusedComponent(this);
        }

        const FlatRow& row = rows_[static_cast<std::size_t>(index)];
        const int contentX = event.getX() - viewport.x - padding_.left;
        const int triangleStart = row.depth * indent_;

        if (!row.node->children.empty() &&
            contentX >= triangleStart &&
            contentX < triangleStart + triangleSize_) {
            setExpanded(row.node->id, !row.node->expanded);
            return;
        }

        selectNode(row.node->id);
    }

    void TreeView::onKeyEvent(EventKeyboard& event) {
        if (!event.isPressed() || event.isRepeat() || isDisabled() ||
            !isFocused() || rows_.empty()) {
            return;
        }

        const int index = selectedRowIndex();

        switch (event.getKey()) {
            case SDLK_DOWN:
                selectRowIndex(index < 0 ? 0 : std::min(
                    index + 1,
                    static_cast<int>(rows_.size()) - 1
                ));
                break;
            case SDLK_UP:
                selectRowIndex(index < 0
                    ? static_cast<int>(rows_.size()) - 1
                    : std::max(0, index - 1));
                break;
            case SDLK_LEFT: {
                if (index < 0) {
                    return;
                }

                const FlatRow& row = rows_[static_cast<std::size_t>(index)];
                if (!row.node->children.empty() && row.node->expanded) {
                    setExpanded(row.node->id, false);
                } else if (row.parent != nullptr) {
                    selectNode(row.parent->id);
                }
                break;
            }
            case SDLK_RIGHT: {
                if (index < 0) {
                    return;
                }

                const FlatRow& row = rows_[static_cast<std::size_t>(index)];
                if (row.node->children.empty()) {
                    return;
                }

                if (!row.node->expanded) {
                    setExpanded(row.node->id, true);
                } else if (index + 1 < static_cast<int>(rows_.size())) {
                    selectRowIndex(index + 1);
                }
                break;
            }
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (!selectedId_.empty()) {
                    runSelectionActions(selectedId_);
                }
                break;
            default:
                return;
        }

        event.handled = true;
    }

    void TreeView::ensureVisible(int rowIndex) {
        if (rowIndex < 0 || rowIndex >= static_cast<int>(rows_.size())) {
            return;
        }

        const SDL_Point size = getLayoutSize();
        const int top = padding_.top + rowIndex * rowHeight_;
        const int bottom = top + rowHeight_;
        const int viewTop = getScrollY();
        const int viewBottom = viewTop + size.y;

        if (top < viewTop) {
            setScroll(getScrollX(), std::max(0, top - padding_.top));
        } else if (bottom > viewBottom) {
            setScroll(
                getScrollX(),
                bottom - size.y + padding_.bottom
            );
        }
    }

    int TreeView::selectedRowIndex() const {
        for (std::size_t index = 0; index < rows_.size(); ++index) {
            if (rows_[index].node->id == selectedId_) {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    void TreeView::selectRowIndex(int rowIndex) {
        if (rowIndex < 0 || rowIndex >= static_cast<int>(rows_.size())) {
            return;
        }

        selectNode(rows_[static_cast<std::size_t>(rowIndex)].node->id);
    }

    void TreeView::runSelectionActions(const std::string& id) {
        const auto snapshot = selectionActions_;

        for (const SelectionAction& action : snapshot) {
            try {
                action(*this, id);
            } catch (const std::exception& error) {
                spdlog::error("TreeView action failed: {}", error.what());
            }
        }
    }

} // emgui

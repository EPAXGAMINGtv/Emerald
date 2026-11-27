// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/DockPanel.h"

#include "emrldgui/dragdrop/DragDropManager.h"
#include "emrldgui/style/StyleValues.h"
#include "emrldgui/ui_component/components/Splitter.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace emgui {

    namespace {

        float boundedShare(float value) {
            return std::clamp(value, 0.02f, 0.9f);
        }

    } // namespace

    DockPanel::DockPanel(int x, int y, int width, int height)
        : UIComponent(x, y),
          width_(std::max(0, width)),
          height_(std::max(0, height)) {
        setFocusable(true);

        surface_.color = {246, 245, 244, 255};
        surface_.borderColor = {223, 223, 223, 255};
        surface_.hasBorderColor = true;
        surface_.borderWidth = 1;
        surface_.radii = {8, 8, 8, 8};

        padding_.top = 4;
        padding_.right = 4;
        padding_.bottom = 4;
        padding_.left = 4;
    }

    DockPanel::~DockPanel() {
        // Never ask the window for the manager here: it may already be gone.
        disposeSplitters();
    }

    void DockPanel::applyStyle(const StyleValues& values) {
        surface_.apply(values);
        padding_.apply(values, 4);
        focusRing_.apply(values);

        edgeHighlightColor_ = values.getColor(
            "edge-highlight-color",
            edgeHighlightColor_
        );
        edgeHighlightWidth_ = std::max(
            1,
            values.getInt("edge-highlight-width", edgeHighlightWidth_)
        );
        gap_ = std::max(0, values.getInt("gap", gap_));
        minPaneSize_ = std::max(
            4,
            values.getInt("min-pane-size", minPaneSize_)
        );

        for (const auto& splitter : splitters_) {
            splitter->setMinPaneSize(minPaneSize_);
        }

        layout();
    }

    void DockPanel::onWindowAttached(Window& window) {
        componentManager_ = &window.getUIComponentManager();
        rebuildSplitters();
    }

    void DockPanel::setLayoutRect(const SDL_Rect& rect) {
        x_ = rect.x;
        y_ = rect.y;
        width_ = std::max(0, rect.w);
        height_ = std::max(0, rect.h);
        layout();
    }

    void DockPanel::setSubtreeVisible(bool visible) {
        setVisible(visible);

        for (const Docked& entry : docked_) {
            if (entry.widget != nullptr) {
                entry.widget->setSubtreeVisible(visible);
            }
        }

        for (const auto& splitter : splitters_) {
            splitter->setSubtreeVisible(visible);
        }
    }

    void DockPanel::collectFocusable(std::vector<UIComponent*>& out) {
        UIComponent::collectFocusable(out);

        for (const Docked& entry : docked_) {
            if (entry.widget != nullptr) {
                entry.widget->collectFocusable(out);
            }
        }

        for (const auto& splitter : splitters_) {
            splitter->collectFocusable(out);
        }
    }

    void DockPanel::dock(
        UIComponent* widget,
        Edge edge,
        float fraction
    ) {
        if (widget == nullptr || widget == this) {
            return;
        }

        const float clamped = std::clamp(fraction, 0.05f, 0.95f);

        const auto existing = std::find_if(
            docked_.begin(),
            docked_.end(),
            [widget](const Docked& entry) {
                return entry.widget == widget;
            }
        );

        if (existing != docked_.end()) {
            existing->edge = edge;
            existing->fraction = clamped;
        } else {
            docked_.push_back({widget, edge, clamped});
        }

        widget->setRenderedByParent(true);
        rebuildSplitters();
    }

    void DockPanel::undock(UIComponent* widget) {
        if (widget == nullptr) {
            return;
        }

        const auto existing = std::find_if(
            docked_.begin(),
            docked_.end(),
            [widget](const Docked& entry) {
                return entry.widget == widget;
            }
        );

        if (existing == docked_.end()) {
            return;
        }

        widget->setRenderedByParent(false);
        docked_.erase(existing);
        rebuildSplitters();
    }

    void DockPanel::clear() {
        for (const Docked& entry : docked_) {
            if (entry.widget != nullptr) {
                entry.widget->setRenderedByParent(false);
            }
        }

        docked_.clear();
        disposeSplitters();
    }

    std::vector<UIComponent*> DockPanel::getWidgets() const {
        std::vector<UIComponent*> widgets;
        widgets.reserve(docked_.size());

        for (const Docked& entry : docked_) {
            widgets.push_back(entry.widget);
        }

        return widgets;
    }

    void DockPanel::setSplitterEnabled(bool enabled) {
        if (splitterEnabled_ == enabled) {
            return;
        }

        splitterEnabled_ = enabled;
        rebuildSplitters();
    }

    void DockPanel::disposeSplitters() {
        if (componentManager_ != nullptr &&
            !componentManager_->isShuttingDown()) {
            for (const auto& splitter : splitters_) {
                componentManager_->removeComponent(splitter.get());
            }
        }

        splitters_.clear();
        rootComponent_ = nullptr;
    }

    Splitter* DockPanel::makeSplitter(Splitter::Orientation orientation) {
        auto splitter = std::make_unique<Splitter>(
            0,
            0,
            0,
            0,
            orientation
        );
        Splitter* raw = splitter.get();

        raw->setStyleClass(splitterEnabled_ ? "" : "fixed");
        raw->setDisabled(!splitterEnabled_);
        raw->setMinPaneSize(minPaneSize_);
        raw->setRenderedByParent(true);

        splitters_.push_back(std::move(splitter));

        if (componentManager_ != nullptr) {
            componentManager_->addComponent(raw);
        }

        return raw;
    }

    UIComponent* DockPanel::buildChain(
        const std::vector<Slot>& slots,
        Splitter::Orientation orientation
    ) {
        if (slots.empty()) {
            return nullptr;
        }

        UIComponent* root = slots.front().component;

        for (std::size_t index = 1; index < slots.size(); ++index) {
            Splitter* splitter = makeSplitter(orientation);
            splitter->setFirst(root);
            splitter->setSecond(slots[index].component);
            splitter->setSplit(slots[index - 1].fraction);

            const int bound = slots[index - 1].dockedIndex;

            if (bound >= 0) {
                const std::size_t dockedIndex = static_cast<std::size_t>(bound);

                splitter->onSplitChanged(
                    [this, dockedIndex](Splitter& source) noexcept {
                        if (dockedIndex < docked_.size()) {
                            docked_[dockedIndex].fraction = source.getSplit();
                        }
                    }
                );
            }

            root = splitter;
        }

        return root;
    }

    void DockPanel::rebuildSplitters() {
        disposeSplitters();

        if (docked_.empty()) {
            return;
        }

        const auto collect = [this](Edge edge) {
            std::vector<Slot> slots;

            for (std::size_t index = 0; index < docked_.size(); ++index) {
                if (docked_[index].edge == edge) {
                    slots.push_back(
                        {
                            docked_[index].widget,
                            docked_[index].fraction,
                            static_cast<int>(index)
                        }
                    );
                }
            }

            return slots;
        };

        const std::vector<Slot> leftSlots = collect(Edge::Left);
        const std::vector<Slot> rightSlots = collect(Edge::Right);
        const std::vector<Slot> topSlots = collect(Edge::Top);
        const std::vector<Slot> bottomSlots = collect(Edge::Bottom);
        const std::vector<Slot> centerSlots = collect(Edge::Center);

        // Widgets on one edge stack along that edge's cross axis
        UIComponent* leftRoot = buildChain(
            leftSlots,
            Splitter::Orientation::Horizontal
        );
        UIComponent* rightRoot = buildChain(
            rightSlots,
            Splitter::Orientation::Horizontal
        );
        UIComponent* topRoot = buildChain(
            topSlots,
            Splitter::Orientation::Vertical
        );
        UIComponent* bottomRoot = buildChain(
            bottomSlots,
            Splitter::Orientation::Vertical
        );
        UIComponent* centerRoot = buildChain(
            centerSlots,
            Splitter::Orientation::Horizontal
        );

        float topShare = boundedShare(
            topSlots.empty() ? 0.0f : topSlots.front().fraction
        );
        float bottomShare = boundedShare(
            bottomSlots.empty() ? 0.0f : bottomSlots.front().fraction
        );
        const float verticalTotal = topShare + bottomShare;

        if (verticalTotal > 0.9f) {
            const float scale = 0.9f / verticalTotal;
            topShare *= scale;
            bottomShare *= scale;
        }

        const int topIndex = topSlots.empty() ? -1 : topSlots.front().dockedIndex;
        const int bottomIndex = bottomSlots.empty()
            ? -1
            : bottomSlots.front().dockedIndex;

        std::vector<Slot> verticalSlots;

        if (topRoot != nullptr && centerRoot != nullptr &&
            bottomRoot != nullptr) {
            verticalSlots.push_back({topRoot, topShare, topIndex});
            verticalSlots.push_back(
                {
                    centerRoot,
                    (1.0f - topShare - bottomShare) / (1.0f - topShare),
                    -1
                }
            );
            verticalSlots.push_back({bottomRoot, bottomShare, bottomIndex});
        } else if (topRoot != nullptr && centerRoot != nullptr) {
            verticalSlots.push_back({topRoot, topShare, topIndex});
            verticalSlots.push_back({centerRoot, 1.0f - topShare, -1});
        } else if (topRoot != nullptr && bottomRoot != nullptr) {
            const float total = topShare + bottomShare;

            verticalSlots.push_back({topRoot, topShare / total, topIndex});
            verticalSlots.push_back(
                {bottomRoot, bottomShare / total, bottomIndex}
            );
        } else if (centerRoot != nullptr && bottomRoot != nullptr) {
            verticalSlots.push_back({centerRoot, 1.0f - bottomShare, -1});
            verticalSlots.push_back({bottomRoot, bottomShare, bottomIndex});
        } else if (topRoot != nullptr) {
            verticalSlots.push_back({topRoot, 1.0f, topIndex});
        } else if (centerRoot != nullptr) {
            verticalSlots.push_back({centerRoot, 1.0f, -1});
        } else if (bottomRoot != nullptr) {
            verticalSlots.push_back({bottomRoot, 1.0f, bottomIndex});
        }

        UIComponent* middleRoot = buildChain(
            verticalSlots,
            Splitter::Orientation::Horizontal
        );

        float leftShare = boundedShare(
            leftSlots.empty() ? 0.0f : leftSlots.front().fraction
        );
        float rightShare = boundedShare(
            rightSlots.empty() ? 0.0f : rightSlots.front().fraction
        );
        const float horizontalTotal = leftShare + rightShare;

        if (horizontalTotal > 0.9f) {
            const float scale = 0.9f / horizontalTotal;
            leftShare *= scale;
            rightShare *= scale;
        }

        const int leftIndex = leftSlots.empty()
            ? -1
            : leftSlots.front().dockedIndex;
        const int rightIndex = rightSlots.empty()
            ? -1
            : rightSlots.front().dockedIndex;

        std::vector<Slot> horizontalSlots;

        if (leftRoot != nullptr && middleRoot != nullptr &&
            rightRoot != nullptr) {
            horizontalSlots.push_back({leftRoot, leftShare, leftIndex});
            horizontalSlots.push_back(
                {
                    middleRoot,
                    (1.0f - leftShare - rightShare) / (1.0f - leftShare),
                    -1
                }
            );
            horizontalSlots.push_back({rightRoot, rightShare, rightIndex});
        } else if (leftRoot != nullptr && middleRoot != nullptr) {
            horizontalSlots.push_back({leftRoot, leftShare, leftIndex});
            horizontalSlots.push_back({middleRoot, 1.0f - leftShare, -1});
        } else if (leftRoot != nullptr && rightRoot != nullptr) {
            const float total = leftShare + rightShare;

            horizontalSlots.push_back({leftRoot, leftShare / total, leftIndex});
            horizontalSlots.push_back(
                {rightRoot, rightShare / total, rightIndex}
            );
        } else if (middleRoot != nullptr && rightRoot != nullptr) {
            horizontalSlots.push_back({middleRoot, 1.0f - rightShare, -1});
            horizontalSlots.push_back({rightRoot, rightShare, rightIndex});
        } else if (leftRoot != nullptr) {
            horizontalSlots.push_back({leftRoot, 1.0f, leftIndex});
        } else if (middleRoot != nullptr) {
            horizontalSlots.push_back({middleRoot, 1.0f, -1});
        } else if (rightRoot != nullptr) {
            horizontalSlots.push_back({rightRoot, 1.0f, rightIndex});
        }

        rootComponent_ = buildChain(
            horizontalSlots,
            Splitter::Orientation::Vertical
        );
        layout();
    }

    SDL_Rect DockPanel::getContentRect() const {
        return {
            x_ + padding_.left,
            y_ + padding_.top,
            std::max(0, width_ - padding_.horizontal()),
            std::max(0, height_ - padding_.vertical())
        };
    }

    void DockPanel::layout() {
        if (rootComponent_ == nullptr) {
            return;
        }

        rootComponent_->setLayoutRect(getContentRect());
        rootComponent_->layout();
    }

    void DockPanel::render(RendererOpts& renderer) {
        const SDL_Rect rect{x_, y_, width_, height_};

        if (rect.w <= 0 || rect.h <= 0) {
            return;
        }

        surface_.drawShadow(renderer, rect);
        surface_.drawBackground(renderer, rect, surface_.color);
        surface_.drawBorder(renderer, rect, surface_.borderColor);

        if (rootComponent_ != nullptr && rootComponent_->isVisible() &&
            rootComponent_->isRenderedByParent()) {
            rootComponent_->render(renderer);
        }

        focusRing_.draw(renderer, rect, surface_.maxRadius(), isFocused());
        drawDropHighlight(renderer);
    }

    DockPanel::Edge DockPanel::edgeAt(int x, int y) const {
        const float relativeX = std::clamp(
            static_cast<float>(x) / static_cast<float>(std::max(1, width_)),
            0.0f,
            1.0f
        );
        const float relativeY = std::clamp(
            static_cast<float>(y) / static_cast<float>(std::max(1, height_)),
            0.0f,
            1.0f
        );

        Edge edge = Edge::Center;
        float best = 0.25f;

        if (relativeX < best) {
            best = relativeX;
            edge = Edge::Left;
        }
        if (1.0f - relativeX < best) {
            best = 1.0f - relativeX;
            edge = Edge::Right;
        }
        if (relativeY < best) {
            best = relativeY;
            edge = Edge::Top;
        }
        if (1.0f - relativeY < best) {
            edge = Edge::Bottom;
        }

        return edge;
    }

    void DockPanel::drawDropHighlight(RendererOpts& renderer) const {
        auto& dragDrop = DragDropManager::instance();

        if (!dragDrop.isDragging()) {
            return;
        }

        UIComponent* target = dragDrop.getDropTarget();
        bool isTarget = target == this;

        for (const Docked& entry : docked_) {
            if (entry.widget == target) {
                isTarget = true;
                break;
            }
        }

        if (!isTarget) {
            return;
        }

        const SDL_Point position = dragDrop.getDragPosition();
        const Edge edge = edgeAt(position.x - x_, position.y - y_);

        if (edge == Edge::Center) {
            return;
        }

        const SDL_Rect inner = getContentRect();

        if (inner.w <= 0 || inner.h <= 0) {
            return;
        }

        const int thickness = std::min(
            std::max(1, edgeHighlightWidth_),
            edge == Edge::Left || edge == Edge::Right ? inner.w : inner.h
        );

        SDL_Rect strip = inner;

        switch (edge) {
            case Edge::Left:
                strip.w = thickness;
                break;
            case Edge::Right:
                strip.x = inner.x + inner.w - thickness;
                strip.w = thickness;
                break;
            case Edge::Top:
                strip.h = thickness;
                break;
            case Edge::Bottom:
                strip.y = inner.y + inner.h - thickness;
                strip.h = thickness;
                break;
            case Edge::Center:
            default:
                return;
        }

        renderer.fillRect(strip, edgeHighlightColor_);
    }

} // emgui

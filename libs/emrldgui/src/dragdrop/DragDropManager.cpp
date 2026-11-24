// Created by epax on 24.11.26.

#include "emrldgui/dragdrop/DragDropManager.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

namespace emgui {

    namespace {

        constexpr int kPreviewOffset = 14;
        constexpr int kPreviewPadding = 8;
        constexpr int kPreviewRadius = 6;
        constexpr int kPreviewMinWidth = 36;
        constexpr int kPreviewMinHeight = 24;
        constexpr std::size_t kMaxLabelChars = 32;

        // Payloads are opaque; only plain strings get a readable preview label
        std::string payloadLabel(const std::any& payload) {
            if (const auto* value = std::any_cast<std::string>(&payload)) {
                return *value;
            }

            if (const auto* value = std::any_cast<const char*>(&payload)) {
                return *value == nullptr ? std::string{} : std::string{*value};
            }

            if (const auto* value = std::any_cast<char*>(&payload)) {
                return *value == nullptr ? std::string{} : std::string{*value};
            }

            if (const auto* value = std::any_cast<std::string_view>(&payload)) {
                return value->empty()
                    ? std::string{}
                    : std::string{value->data(), value->size()};
            }

            return {};
        }

        std::string shortLabel(const std::any& payload) {
            std::string label = payloadLabel(payload);

            const std::string::size_type lineEnd = label.find('\n');
            if (lineEnd != std::string::npos) {
                label.resize(lineEnd);
            }

            if (label.size() > kMaxLabelChars) {
                label.resize(kMaxLabelChars);
                label += "...";
            }

            return label;
        }

    } // namespace

    DragDropManager& DragDropManager::instance() {
        static DragDropManager manager;
        return manager;
    }

    void DragDropManager::setEnabled(bool enabled) {
        enabled_ = enabled;
    }

    bool DragDropManager::isEnabled() const {
        return enabled_;
    }

    bool DragDropManager::isDragging() const {
        return dragging_;
    }

    void DragDropManager::setPayload(std::any payload) {
        payload_ = std::move(payload);
    }

    const std::any& DragDropManager::getPayload() const {
        return payload_;
    }

    void DragDropManager::clearPayload() {
        payload_ = std::any{};
    }

    void DragDropManager::beginDrag(UIComponent* source, std::any payload, int x, int y) {
        if (!enabled_) {
            return;
        }

        source_ = source;
        payload_ = std::move(payload);
        startPosition_ = {x, y};
        currentPosition_ = {x, y};
        dropTarget_ = nullptr;
        dragging_ = true;

        if (dragBeginCallback_) {
            dragBeginCallback_(source_, payload_);
        }
    }

    void DragDropManager::updateDrag(int x, int y) {
        currentPosition_ = {x, y};
    }

    void DragDropManager::endDrag(bool dropped) {
        if (!dragging_) {
            return;
        }

        UIComponent* source = source_;
        UIComponent* target = dropTarget_;

        if (dropped && target != nullptr && dropCallback_) {
            dropCallback_(source, target, payload_);
        }

        if (dragEndCallback_) {
            dragEndCallback_(source, dropped);
        }

        dragging_ = false;
        source_ = nullptr;
        dropTarget_ = nullptr;
        clearPayload();
    }

    void DragDropManager::setDropTarget(UIComponent* target) {
        dropTarget_ = target;
    }

    UIComponent* DragDropManager::getDropTarget() const {
        return dropTarget_;
    }

    UIComponent* DragDropManager::getDragSource() const {
        return source_;
    }

    SDL_Point DragDropManager::getDragPosition() const {
        return currentPosition_;
    }

    void DragDropManager::onDrop(
        std::function<void(UIComponent*, UIComponent*, const std::any&)> callback
    ) {
        dropCallback_ = std::move(callback);
    }

    void DragDropManager::onDragBegin(
        std::function<void(UIComponent*, const std::any&)> callback
    ) {
        dragBeginCallback_ = std::move(callback);
    }

    void DragDropManager::onDragEnd(
        std::function<void(UIComponent*, bool)> callback
    ) {
        dragEndCallback_ = std::move(callback);
    }

    void DragDropManager::attachWindow(SDL_Window* window) {
        if (window == nullptr || window == window_) {
            return;
        }

        // Only one window owns the listener at a time
        if (window_ != nullptr) {
            detachWindow(window_);
        }

        if (EventManager::getForWindow(window) == nullptr) {
            return;
        }

        window_ = window;
        mouseListener_ = EventManager::registerEvent(
            window_,
            EventMouse(window_),
            [this](EventMouse& event) {
                onMouseEvent(event);
            }
        );
    }

    void DragDropManager::detachWindow(SDL_Window* window) {
        if (window == nullptr || window_ == nullptr || window != window_) {
            return;
        }

        if (mouseListener_ != EventManager::kInvalidListener) {
            if (auto* manager = EventManager::getForWindow(window_)) {
                manager->removeListener(mouseListener_);
            }
        }

        mouseListener_ = EventManager::kInvalidListener;
        window_ = nullptr;
    }

    void DragDropManager::onMouseEvent(EventMouse& event) {
        if (!dragging_) {
            return;
        }

        if (event.isMoved()) {
            updateDrag(event.getX(), event.getY());
            return;
        }

        // A press is ignored: the source component starts the drag itself
        if (event.isReleased()) {
            endDrag(true);
        }
    }

    void DragDropManager::renderDragPreview(RendererOpts& renderer) {
        if (!dragging_) {
            return;
        }

        const std::string label = shortLabel(payload_);
        const FontSpec fontSpec{"LiberationSans", 13, "normal"};
        const SDL_Point textSize = label.empty()
            ? SDL_Point{0, 0}
            : renderer.measureText(label, fontSpec);

        const SDL_Rect rect{
            currentPosition_.x + kPreviewOffset,
            currentPosition_.y + kPreviewOffset,
            std::max(kPreviewMinWidth, textSize.x + 2 * kPreviewPadding),
            std::max(kPreviewMinHeight, textSize.y + 2 * kPreviewPadding)
        };

        const SDL_Color background{38, 42, 48, 190};
        const SDL_Color border{180, 188, 198, 220};
        const SDL_Color foreground{240, 242, 245, 255};

        renderer.fillRoundedRect(rect, kPreviewRadius, background);
        renderer.drawRoundedRect(rect, kPreviewRadius, border);

        if (!label.empty()) {
            renderer.drawText(
                label,
                {rect.x + kPreviewPadding, rect.y + kPreviewPadding},
                foreground,
                fontSpec
            );
        }
    }

} // emgui

// Created by epax on 24.11.26.

// Shared drag and drop state for every component of one GUI instance

#pragma once

#include "emrldgui/event/EventManager.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/render/RendererOpts.h"
#include "emrldgui/ui_component/UIComponent.h"

#include <SDL.h>

#include <any>
#include <functional>
#include <memory>
#include <string>

namespace emgui {

    class DragDropManager {
    public:
        static DragDropManager& instance();

        void setEnabled(bool enabled);
        bool isEnabled() const;
        bool isDragging() const;

        void setPayload(std::any payload);
        const std::any& getPayload() const;
        void clearPayload();

        void beginDrag(UIComponent* source, std::any payload, int x, int y);
        void updateDrag(int x, int y);
        void endDrag(bool dropped);

        void setDropTarget(UIComponent* target);
        UIComponent* getDropTarget() const;
        UIComponent* getDragSource() const;
        SDL_Point getDragPosition() const;

        void onDrop(std::function<void(UIComponent*, UIComponent*, const std::any&)> callback);
        void onDragBegin(std::function<void(UIComponent*, const std::any&)> callback);
        void onDragEnd(std::function<void(UIComponent*, bool)> callback);

        void attachWindow(SDL_Window* window);
        void detachWindow(SDL_Window* window);

        void renderDragPreview(RendererOpts& renderer);

        DragDropManager(const DragDropManager&) = delete;
        DragDropManager& operator=(const DragDropManager&) = delete;

    private:
        DragDropManager() = default;

        void onMouseEvent(EventMouse& event);

        bool enabled_ = true;
        bool dragging_ = false;

        SDL_Window* window_ = nullptr;
        EventManager::ListenerId mouseListener_ = EventManager::kInvalidListener;

        UIComponent* source_ = nullptr;
        UIComponent* dropTarget_ = nullptr;
        SDL_Point startPosition_{0, 0};
        SDL_Point currentPosition_{0, 0};
        std::any payload_;

        std::function<void(UIComponent*, UIComponent*, const std::any&)> dropCallback_;
        std::function<void(UIComponent*, const std::any&)> dragBeginCallback_;
        std::function<void(UIComponent*, bool)> dragEndCallback_;
    };

} // emgui

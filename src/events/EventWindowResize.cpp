// Created by epax on 27.11.26.

#include "events/EventWindowResize.h"

#include <memory>
#include <vector>

namespace emerald {

    EventWindowResize::EventWindowResize(SDL_Window* window)
        : window_(window),
          windowID_(window != nullptr ? SDL_GetWindowID(window) : 0) {}

    EventWindowResize::EventWindowResize(
        const SDL_Event& event, SDL_Window* window
    )
        : window_(window),
          windowID_(event.window.windowID),
          resized_(
              event.window.event == SDL_WINDOWEVENT_RESIZED ||
              event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
          ) {
        if (resized_) {
            width_ = event.window.data1;
            height_ = event.window.data2;
        }
    }

    emgui::EventType EventWindowResize::getType() const {
        return TYPE;
    }

    std::vector<Uint32> EventWindowResize::getSDLEventTypes() const {
        return {SDL_WINDOWEVENT};
    }

    emgui::EventFactory EventWindowResize::getFactory() const {
        return [window = window_](const SDL_Event& event) {
            return std::make_unique<EventWindowResize>(event, window);
        };
    }

    bool EventWindowResize::isResized() const {
        return resized_;
    }

    Uint32 EventWindowResize::getWindowID() const {
        return windowID_;
    }

    int EventWindowResize::getWidth() const {
        return width_;
    }

    int EventWindowResize::getHeight() const {
        return height_;
    }

} // namespace emerald

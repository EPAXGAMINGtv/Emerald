// Created by epax on 22.11.26.

#include "emrldgui/event/EventTextInput.h"

namespace emgui {

    EventTextInput::EventTextInput(SDL_Window* window)
        : window_(window),
          windowID_(window != nullptr ? SDL_GetWindowID(window) : 0) {
    }

    EventTextInput::EventTextInput(const SDL_Event& event, SDL_Window* window)
        : window_(window),
          windowID_(window != nullptr ? SDL_GetWindowID(window) : event.text.windowID),
          text_(event.text.text) {
    }

    EventType EventTextInput::getType() const {
        return TYPE;
    }

    std::vector<Uint32> EventTextInput::getSDLEventTypes() const {
        return {SDL_TEXTINPUT};
    }

    EventFactory EventTextInput::getFactory() const {
        return [window = window_](const SDL_Event& event) {
            return std::make_unique<EventTextInput>(event, window);
        };
    }

    Uint32 EventTextInput::getWindowID() const {
        return windowID_;
    }

    SDL_Window* EventTextInput::getWindow() const {
        return window_;
    }

    const std::string& EventTextInput::getText() const {
        return text_;
    }

} // emgui

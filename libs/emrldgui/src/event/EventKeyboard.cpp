// Created by epax on 21.11.26.

#include "../../include/emrldgui/event/EventKeyboard.h"

namespace emgui {

    EventKeyboard::EventKeyboard(SDL_Window* window)
        : window_(window),
          windowID_(window != nullptr ? SDL_GetWindowID(window) : 0) {
    }

    EventKeyboard::EventKeyboard(const SDL_Event& event, SDL_Window* window)
        : window_(window),
          windowID_(window != nullptr ? SDL_GetWindowID(window) : event.key.windowID),
          key_(event.key.keysym.sym),
          modifiers_(static_cast<SDL_Keymod>(event.key.keysym.mod)),
          pressed_(event.type == SDL_KEYDOWN),
          released_(event.type == SDL_KEYUP),
          repeat_(event.key.repeat != 0) {
    }

    EventType EventKeyboard::getType() const {
        return TYPE;
    }

    std::vector<Uint32> EventKeyboard::getSDLEventTypes() const {
        return {SDL_KEYDOWN, SDL_KEYUP};
    }

    EventFactory EventKeyboard::getFactory() const {
        return [window = window_](const SDL_Event& event) {
            return std::make_unique<EventKeyboard>(event, window);
        };
    }

    SDL_Keycode EventKeyboard::getKey() const {
        return key_;
    }

    SDL_Keymod EventKeyboard::getModifiers() const {
        return modifiers_;
    }

    bool EventKeyboard::isPressed() const {
        return pressed_;
    }

    bool EventKeyboard::isReleased() const {
        return released_;
    }

    bool EventKeyboard::isRepeat() const {
        return repeat_;
    }

    Uint32 EventKeyboard::getWindowID() const {
        return windowID_;
    }

    SDL_Window* EventKeyboard::getWindow() const {
        return window_;
    }

} // emgui

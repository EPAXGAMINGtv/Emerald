//
// Created by epax on 21.11.26.
//

#include "../../include/emrldgui/event/EventMouse.h"

namespace emgui {

    //Constructor and Destructor here ->
    EventMouse::EventMouse(SDL_Window* window)
        : window_(window),
          windowID_(window != nullptr ? SDL_GetWindowID(window) : 0) {
    }

    EventMouse::EventMouse(const SDL_Event& event, SDL_Window* window)
        : window_(window),
          windowID_(window != nullptr ? SDL_GetWindowID(window) : 0),
          sdlType_(event.type) {

        //the SDL union member depends on the event type ->
        if (window_ == nullptr) {
            switch (event.type) {
                case SDL_MOUSEMOTION:
                    windowID_ = event.motion.windowID;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    windowID_ = event.button.windowID;
                    break;
                case SDL_MOUSEWHEEL:
                    windowID_ = event.wheel.windowID;
                    break;
                default:
                    break;
            }
        }

        switch (event.type) {
            case SDL_MOUSEMOTION:
                x_ = event.motion.x;
                y_ = event.motion.y;
                relativeX_ = event.motion.xrel;
                relativeY_ = event.motion.yrel;
                moved_ = true;
                break;
            case SDL_MOUSEBUTTONDOWN:
                x_ = event.button.x;
                y_ = event.button.y;
                button_ = event.button.button;
                pressed_ = true;
                break;
            case SDL_MOUSEBUTTONUP:
                x_ = event.button.x;
                y_ = event.button.y;
                button_ = event.button.button;
                released_ = true;
                break;
            case SDL_MOUSEWHEEL:
                scrollX_ = event.wheel.x;
                scrollY_ = event.wheel.y;
                wheel_ = true;
                break;
            default:
                break;
        }
    }

    //some functions here ->
    EventType EventMouse::getType() const {
        return TYPE;
    }

    std::vector<Uint32> EventMouse::getSDLEventTypes() const {
        return {
            SDL_MOUSEMOTION,
            SDL_MOUSEBUTTONDOWN,
            SDL_MOUSEBUTTONUP,
            SDL_MOUSEWHEEL
        };
    }

    EventFactory EventMouse::getFactory() const {
        return [window = window_](const SDL_Event& event) {
            return std::make_unique<EventMouse>(event, window);
        };
    }

    bool EventMouse::isMoved() const {
        return moved_;
    }

    bool EventMouse::isPressed() const {
        return pressed_;
    }

    bool EventMouse::isReleased() const {
        return released_;
    }

    bool EventMouse::isWheel() const {
        return wheel_;
    }

    //some getters here ->
    Uint32 EventMouse::getWindowID() const {
        return windowID_;
    }

    SDL_Window* EventMouse::getWindow() const {
        return window_;
    }

    Uint32 EventMouse::getSDLType() const {
        return sdlType_;
    }

    int EventMouse::getX() const {
        return x_;
    }

    int EventMouse::getY() const {
        return y_;
    }

    int EventMouse::getRelativeX() const {
        return relativeX_;
    }

    int EventMouse::getRelativeY() const {
        return relativeY_;
    }

    Uint8 EventMouse::getButton() const {
        return button_;
    }

    int EventMouse::getScrollX() const {
        return scrollX_;
    }

    int EventMouse::getScrollY() const {
        return scrollY_;
    }

} // emgui

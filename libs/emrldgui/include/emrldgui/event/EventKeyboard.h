// Created by epax on 21.11.26.

// Keyboard event created from SDL_KEYDOWN / SDL_KEYUP for one window

#pragma once

#include "Event.h"

#include <SDL.h>

namespace emgui {

    class EventKeyboard : public Event {
    public:
        static constexpr EventType TYPE = 1;

        explicit EventKeyboard(SDL_Window* window);
        EventKeyboard(const SDL_Event& event, SDL_Window* window);

        EventType getType() const override;
        std::vector<Uint32> getSDLEventTypes() const override;
        EventFactory getFactory() const override;
        SDL_Keycode getKey() const;
        SDL_Keymod getModifiers() const;
        bool isPressed() const;
        bool isReleased() const;
        bool isRepeat() const;

        Uint32 getWindowID() const;
        SDL_Window* getWindow() const;

    private:
        // Variables
        SDL_Window* window_ = nullptr;
        Uint32 windowID_ = 0;

        SDL_Keycode key_ = SDLK_UNKNOWN;
        SDL_Keymod modifiers_ = KMOD_NONE;

        bool pressed_ = false;
        bool released_ = false;
        bool repeat_ = false;
    };

} // emgui

// Created by epax on 21.11.26.

// Mouse event created from SDL_MOUSEMOTION / SDL_MOUSEBUTTONDOWN / SDL_MOUSEBUTTONUP / SDL_MOUSEWHEEL for one window

#pragma once

#include "Event.h"

#include <SDL.h>

namespace emgui {

    class EventMouse : public Event {
    public:
        static constexpr EventType TYPE = 2;

        explicit EventMouse(SDL_Window* window);
        EventMouse(const SDL_Event& event, SDL_Window* window);

        EventType getType() const override;
        std::vector<Uint32> getSDLEventTypes() const override;
        EventFactory getFactory() const override;

        bool isMoved() const;
        bool isPressed() const;
        bool isReleased() const;
        bool isWheel() const;

        Uint32 getWindowID() const;
        SDL_Window* getWindow() const;
        Uint32 getSDLType() const;

        int getX() const;
        int getY() const;
        int getRelativeX() const;
        int getRelativeY() const;

        Uint8 getButton() const;
        int getScrollX() const;
        int getScrollY() const;

    private:
        // Variables
        SDL_Window* window_ = nullptr;
        Uint32 windowID_ = 0;
        Uint32 sdlType_ = 0;

        int x_ = 0;
        int y_ = 0;
        int relativeX_ = 0;
        int relativeY_ = 0;

        Uint8 button_ = 0;
        int scrollX_ = 0;
        int scrollY_ = 0;

        bool moved_ = false;
        bool pressed_ = false;
        bool released_ = false;
        bool wheel_ = false;
    };

} // emgui

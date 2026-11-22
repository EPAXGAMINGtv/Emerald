// Created by epax on 21.11.26.

// Input helper that uses EventKeyboard from the window event manager

#pragma once

#include "../Window.h"
#include "../event/EventKeyboard.h"

#include <SDL.h>

#include <unordered_map>

namespace emgui {
    class Keyboard {
    public:
        explicit Keyboard(Window& window);
        ~Keyboard();

        // A listener captures 'this', so copies and moves would dangle
        Keyboard(const Keyboard&) = delete;
        Keyboard& operator=(const Keyboard&) = delete;
        Keyboard(Keyboard&&) = delete;
        Keyboard& operator=(Keyboard&&) = delete;

        bool isDown(SDL_Keycode key) const;
        bool isPressed(SDL_Keycode key) const;
        bool isReleased(SDL_Keycode key) const;

        // Call once per frame to clear the pressed / released states
        void clearFrameState();

    private:
        void onKeyboardEvent(EventKeyboard& event);

        // Variables
        Window& window_;
        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;
        std::unordered_map<SDL_Keycode, bool> down_;
        std::unordered_map<SDL_Keycode, bool> pressed_;
        std::unordered_map<SDL_Keycode, bool> released_;
    };
} // emgui

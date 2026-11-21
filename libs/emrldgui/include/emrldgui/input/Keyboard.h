//
// Created by epax on 21.11.26.
//

//
// input helper that uses EventKeyboard from the window event manager
//

#pragma once

#include "../Window.h"
#include "../event/EventKeyboard.h"

#include <SDL.h>

#include <unordered_map>

namespace emgui {
    class Keyboard {
    public:
        //Constructor and Destructor here ->
        explicit Keyboard(Window& window);
        ~Keyboard();

        //some functions here ->
        bool isDown(SDL_Keycode key) const;
        bool isPressed(SDL_Keycode key) const;
        bool isReleased(SDL_Keycode key) const;

        //call once per frame to clear the pressed / released states ->
        void clearFrameState();

    private:
        //some functions here ->
        void onKeyboardEvent(EventKeyboard& event);

        //variables
        Window& window_;
        bool registered_ = false;
        std::unordered_map<SDL_Keycode, bool> down_;
        std::unordered_map<SDL_Keycode, bool> pressed_;
        std::unordered_map<SDL_Keycode, bool> released_;
    };
} // emgui

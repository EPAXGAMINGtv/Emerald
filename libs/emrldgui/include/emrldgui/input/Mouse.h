//
// Created by epax on 21.11.26.
//

//
// input helper that uses EventMouse from the window event manager
//

#pragma once

#include "../Window.h"
#include "../event/EventMouse.h"

#include <SDL.h>

#include <unordered_map>

namespace emgui {
    class Mouse {
    public:
        //Constructor and Destructor here ->
        explicit Mouse(Window& window);
        ~Mouse();

        //some functions here ->
        bool isButtonDown(Uint8 button) const;
        bool isButtonPressed(Uint8 button) const;
        bool isButtonReleased(Uint8 button) const;

        int getX() const;
        int getY() const;
        int getRelativeX() const;
        int getRelativeY() const;
        int getScrollX() const;
        int getScrollY() const;

        //call once per frame to clear the pressed / released / scroll states ->
        void clearFrameState();

    private:
        //some functions here ->
        void onMouseEvent(EventMouse& event);

        //variables
        Window& window_;
        bool registered_ = false;

        int x_ = 0;
        int y_ = 0;
        int relativeX_ = 0;
        int relativeY_ = 0;
        int scrollX_ = 0;
        int scrollY_ = 0;

        std::unordered_map<Uint8, bool> down_;
        std::unordered_map<Uint8, bool> pressed_;
        std::unordered_map<Uint8, bool> released_;
    };
} // emgui

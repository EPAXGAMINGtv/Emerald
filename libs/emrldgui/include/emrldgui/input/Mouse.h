// Created by epax on 21.11.26.

// Input helper that uses EventMouse from the window event manager

#pragma once

#include "../Window.h"
#include "../event/EventMouse.h"

#include <SDL.h>

#include <unordered_map>

namespace emgui {
    class Mouse {
    public:
        explicit Mouse(Window& window);
        ~Mouse();

        // A listener captures 'this', so copies and moves would dangle
        Mouse(const Mouse&) = delete;
        Mouse& operator=(const Mouse&) = delete;
        Mouse(Mouse&&) = delete;
        Mouse& operator=(Mouse&&) = delete;

        bool isButtonDown(Uint8 button) const;
        bool isButtonPressed(Uint8 button) const;
        bool isButtonReleased(Uint8 button) const;

        int getX() const;
        int getY() const;
        int getRelativeX() const;
        int getRelativeY() const;
        int getScrollX() const;
        int getScrollY() const;

        // Call once per frame to clear the pressed / released / scroll states
        void clearFrameState();

    private:
        void onMouseEvent(EventMouse& event);

        // Variables
        Window& window_;
        EventManager::ListenerId listenerId_ = EventManager::kInvalidListener;

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

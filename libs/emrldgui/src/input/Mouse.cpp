//
// Created by epax on 21.11.26.
//

#include "../include/emrldgui/input/Mouse.h"

namespace emgui {

    //Constructor and Destructor here ->
    Mouse::Mouse(Window& window) : window_(window) {
        //register EventMouse(window) with a void callback on this window ->
        EventManager::registerEvent(
            window_.getWindow(),
            EventMouse(window_.getWindow()),
            [this](EventMouse& event) {
                onMouseEvent(event);
            }
        );

        registered_ = EventManager::getForWindow(window_.getWindow()) != nullptr;
    }

    Mouse::~Mouse() {
        //remove the callbacks that still hold this mouse ->
        if (registered_ && window_.getWindow() != nullptr) {
            if (auto* manager = EventManager::getForWindow(window_.getWindow())) {
                manager->removeListeners(EventMouse::TYPE);
            }
        }
    }

    //some functions here ->
    bool Mouse::isButtonDown(Uint8 button) const {
        auto it = down_.find(button);
        return it != down_.end() && it->second;
    }

    bool Mouse::isButtonPressed(Uint8 button) const {
        auto it = pressed_.find(button);
        return it != pressed_.end() && it->second;
    }

    bool Mouse::isButtonReleased(Uint8 button) const {
        auto it = released_.find(button);
        return it != released_.end() && it->second;
    }

    int Mouse::getX() const {
        return x_;
    }

    int Mouse::getY() const {
        return y_;
    }

    int Mouse::getRelativeX() const {
        return relativeX_;
    }

    int Mouse::getRelativeY() const {
        return relativeY_;
    }

    int Mouse::getScrollX() const {
        return scrollX_;
    }

    int Mouse::getScrollY() const {
        return scrollY_;
    }

    void Mouse::clearFrameState() {
        pressed_.clear();
        released_.clear();
        relativeX_ = 0;
        relativeY_ = 0;
        scrollX_ = 0;
        scrollY_ = 0;
    }

    void Mouse::onMouseEvent(EventMouse& event) {
        //uses the data delivered by EventMouse ->
        x_ = event.getX();
        y_ = event.getY();

        if (event.isMoved()) {
            relativeX_ = event.getRelativeX();
            relativeY_ = event.getRelativeY();
        }

        const Uint8 button = event.getButton();

        if (button != 0) {
            if (event.isPressed()) {
                down_[button] = true;
                pressed_[button] = true;
                released_[button] = false;
            } else if (event.isReleased()) {
                down_[button] = false;
                released_[button] = true;
                pressed_[button] = false;
            }
        }

        if (event.isWheel()) {
            scrollX_ += event.getScrollX();
            scrollY_ += event.getScrollY();
        }
    }

} // emgui

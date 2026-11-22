// Created by epax on 21.11.26.

#include "emrldgui/input/Keyboard.h"

namespace emgui {

    Keyboard::Keyboard(Window& window) : window_(window) {
        // Register EventKeyboard(window) and keep only our own listener id
        listenerId_ = EventManager::registerEvent(
            window_.getWindow(),
            EventKeyboard(window_.getWindow()),
            [this](EventKeyboard& event) {
                onKeyboardEvent(event);
            }
        );
    }

    Keyboard::~Keyboard() {
        // Remove only the callback that holds this keyboard
        if (listenerId_ != EventManager::kInvalidListener &&
            window_.getWindow() != nullptr) {
            if (auto* manager = EventManager::getForWindow(window_.getWindow())) {
                manager->removeListener(listenerId_);
            }
        }
    }

    bool Keyboard::isDown(SDL_Keycode key) const {
        auto it = down_.find(key);
        return it != down_.end() && it->second;
    }

    bool Keyboard::isPressed(SDL_Keycode key) const {
        auto it = pressed_.find(key);
        return it != pressed_.end() && it->second;
    }

    bool Keyboard::isReleased(SDL_Keycode key) const {
        auto it = released_.find(key);
        return it != released_.end() && it->second;
    }

    void Keyboard::clearFrameState() {
        pressed_.clear();
        released_.clear();
    }

    void Keyboard::onKeyboardEvent(EventKeyboard& event) {
        // Uses the data delivered by EventKeyboard
        const SDL_Keycode key = event.getKey();

        if (event.isPressed()) {
            down_[key] = true;
            pressed_[key] = true;
            released_[key] = false;
        } else if (event.isReleased()) {
            down_[key] = false;
            released_[key] = true;
            pressed_[key] = false;
        }

        // Do not mark handled, other listeners must still receive keyboard events
    }

} // emgui

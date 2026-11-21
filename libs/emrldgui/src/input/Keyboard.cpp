//
// Created by epax on 21.11.26.
//

#include "../include/emrldgui/input/Keyboard.h"

namespace emgui {

    //Constructor and Destructor here ->
    Keyboard::Keyboard(Window& window) : window_(window) {
        //register EventKeyboard(window) with a void callback on this window ->
        EventManager::registerEvent(
            window_.getWindow(),
            EventKeyboard(window_.getWindow()),
            [this](EventKeyboard& event) {
                onKeyboardEvent(event);
            }
        );

        registered_ = EventManager::getForWindow(window_.getWindow()) != nullptr;
    }

    Keyboard::~Keyboard() {
        //remove the callbacks that still hold this keyboard ->
        if (registered_ && window_.getWindow() != nullptr) {
            if (auto* manager = EventManager::getForWindow(window_.getWindow())) {
                manager->removeListeners(EventKeyboard::TYPE);
            }
        }
    }

    //some functions here ->
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
        //uses the data delivered by EventKeyboard ->
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

        event.handled = true;
    }

} // emgui

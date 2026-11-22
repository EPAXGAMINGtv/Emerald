//
// Created by epax on 21.11.26.
//

#include "emrldgui/event/EventManager.h"

#include <algorithm>

#include "spdlog/spdlog.h"

namespace emgui {

    //key used to store the manager on its own window ->
    static constexpr const char* kWindowDataKey = "emgui.EventManager";

    //Constructor and Destructor here ->
    EventManager::EventManager(SDL_Window* window) {
        init(window);
    }

    EventManager::~EventManager() {
        //do not leave a dangling manager pointer on a still living window ->
        if (window_ != nullptr) {
            SDL_SetWindowData(window_, kWindowDataKey, nullptr);
        }
    }

    //static functions here ->
    std::unique_ptr<EventManager> EventManager::create(SDL_Window* window) {
        return std::make_unique<EventManager>(window);
    }

    EventManager* EventManager::getForWindow(SDL_Window* window) {
        if (window == nullptr) {
            return nullptr;
        }

        return static_cast<EventManager*>(
            SDL_GetWindowData(window, kWindowDataKey)
        );
    }

    Uint32 EventManager::getWindowID(const SDL_Event& event) {
        switch (event.type) {
            case SDL_WINDOWEVENT: return event.window.windowID;
            case SDL_KEYDOWN:
            case SDL_KEYUP: return event.key.windowID;
            case SDL_TEXTEDITING: return event.edit.windowID;
            case SDL_TEXTINPUT: return event.text.windowID;
            case SDL_MOUSEMOTION: return event.motion.windowID;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: return event.button.windowID;
            case SDL_MOUSEWHEEL: return event.wheel.windowID;
            default: return 0;
        }
    }

    //some functions here ->
    void EventManager::init(SDL_Window* window) {
        //called by the window constructor, binds this manager to exactly one window
        window_ = window;
        windowID_ = window != nullptr ? SDL_GetWindowID(window) : 0;

        if (window_ != nullptr) {
            SDL_SetWindowData(window_, kWindowDataKey, this);
        }

        spdlog::debug("EventManager initialized for window {}", windowID_);
    }

    void EventManager::registerEventImpl(const Event& event) {
        //the prototype provides the factory and the SDL event types it listens to
        const EventFactory factory = event.getFactory();

        if (!factory) {
            spdlog::error("Event {} has no factory", event.getType());
            return;
        }

        for (const Uint32 sdlEventType : event.getSDLEventTypes()) {
            auto& registrations = registrations_[sdlEventType];

            //register once per gui event type, so adding a listener twice is safe ->
            auto existing = std::find_if(
                registrations.begin(),
                registrations.end(),
                [&event](const EventRegistration& registration) {
                    return registration.type == event.getType();
                }
            );

            if (existing != registrations.end()) {
                *existing = {factory, event.getType()};
            } else {
                registrations.push_back({factory, event.getType()});
            }
        }
    }

    EventManager::ListenerId EventManager::addEventListener(
        EventType type,
        EventCallback callback
    ) {
        const ListenerId id = nextListenerId_++;

        listeners_[type].push_back(Listener{id, std::move(callback)});
        listenerTypes_[id] = type;

        return id;
    }

    void EventManager::removeListener(ListenerId id) {
        const auto typeEntry = listenerTypes_.find(id);

        if (typeEntry == listenerTypes_.end()) {
            return;
        }

        const auto listenerEntry = listeners_.find(typeEntry->second);

        if (listenerEntry != listeners_.end()) {
            auto& listeners = listenerEntry->second;
            listeners.erase(
                std::remove_if(
                    listeners.begin(),
                    listeners.end(),
                    [id](const Listener& listener) {
                        return listener.id == id;
                    }
                ),
                listeners.end()
            );

            if (listeners.empty()) {
                listeners_.erase(listenerEntry);
            }
        }

        listenerTypes_.erase(typeEntry);
    }

    void EventManager::removeListeners(EventType type) {
        const auto listenerEntry = listeners_.find(type);

        if (listenerEntry == listeners_.end()) {
            return;
        }

        for (const Listener& listener : listenerEntry->second) {
            listenerTypes_.erase(listener.id);
        }

        listeners_.erase(listenerEntry);
    }

    void EventManager::processEvent(const SDL_Event& event) {
        auto registrationRange = registrations_.equal_range(event.type);

        for (auto entry = registrationRange.first;
             entry != registrationRange.second;
             ++entry) {

            for (const auto& registration : entry->second) {
                auto guiEvent = registration.factory(event);

                if (!guiEvent) {
                    continue;
                }

                auto listeners = listeners_.find(registration.type);

                if (listeners == listeners_.end()) {
                    continue;
                }

                for (auto& listener : listeners->second) {
                    if (guiEvent->handled) {
                        break;
                    }

                    listener.callback(*guiEvent);
                }
            }
        }
    }

} // emgui

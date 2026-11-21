//
// Created by epax on 21.11.26.
//

//
// file for managing events, every window initializes its own EventManager
//

#pragma once

#include "Event.h"

#include <SDL.h>

#include <functional>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace emgui {

    class EventManager {
    public:
        using EventFactory = emgui::EventFactory;
        using EventCallback = std::function<void(Event&)>;

        //Constructor and Destructor here ->
        explicit EventManager(SDL_Window* window);
        ~EventManager() = default;

        //static functions here ->
        static std::unique_ptr<EventManager> create(SDL_Window* window);
        static EventManager* getForWindow(SDL_Window* window);

        //register an event for a window, callback is a void function ->
        template<typename T, typename Callback>
        static void registerEvent(SDL_Window* window, const T& event, Callback callback) {
            static_assert(
                std::is_base_of_v<Event, T>,
                "T must inherit from Event"
            );

            if (auto* manager = getForWindow(window)) {
                manager->registerEvent(event, std::move(callback));
            }
        }

        template<typename T>
        static void registerEvent(SDL_Window* window, const T& event) {
            static_assert(
                std::is_base_of_v<Event, T>,
                "T must inherit from Event"
            );

            if (auto* manager = getForWindow(window)) {
                manager->registerEvent(event);
            }
        }

        static Uint32 getWindowID(const SDL_Event& event);

        //some functions here ->
        void init(SDL_Window* window);

        //register an event for this window, works for 3rd party events too ->
        template<typename T>
        void registerEvent(const T& event) {
            static_assert(
                std::is_base_of_v<Event, T>,
                "T must inherit from Event"
            );

            registerEventImpl(event);
        }

        template<typename T, typename Callback>
        void registerEvent(const T& event, Callback callback) {
            static_assert(
                std::is_base_of_v<Event, T>,
                "T must inherit from Event"
            );

            registerEventImpl(event);
            addEventListener<T>(std::move(callback));
        }

        template<typename T, typename Callback>
        void addEventListener(Callback callback) {
            static_assert(
                std::is_base_of_v<Event, T>,
                "T must inherit from Event"
            );

            addEventListener(
                T::TYPE,
                [callback = std::move(callback)](Event& event) {
                    if (auto* typedEvent = dynamic_cast<T*>(&event)) {
                        callback(*typedEvent);
                    }
                }
            );
        }

        void addEventListener(EventType type, EventCallback callback);
        void removeListeners(EventType type);
        void processEvent(const SDL_Event& event);

        //some getters here ->
        SDL_Window* getWindow() const {
            return window_;
        }

        Uint32 getWindowID() const {
            return windowID_;
        }

    private:
        void registerEventImpl(const Event& event);

        struct EventRegistration {
            EventFactory factory;
            EventType type = Event::TYPE;
        };

        //variables
        SDL_Window* window_ = nullptr;
        Uint32 windowID_ = 0;
        std::unordered_map<Uint32, std::vector<EventRegistration>> registrations_;
        std::unordered_map<EventType, std::vector<EventCallback>> listeners_;
    };

} // emgui

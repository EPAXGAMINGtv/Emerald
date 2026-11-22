// Created by epax on 21.11.26.

// File for managing events, every window initializes its own EventManager

#pragma once

#include "Event.h"

#include <SDL.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace emgui {

    class EventManager {
    public:
        using EventFactory = emgui::EventFactory;
        using EventCallback = std::function<void(Event&)>;
        using ListenerId = std::size_t;

        static constexpr ListenerId kInvalidListener = 0;

        struct Listener {
            ListenerId id = kInvalidListener;
            EventCallback callback;
        };

        explicit EventManager(SDL_Window* window);
        ~EventManager();

        static std::unique_ptr<EventManager> create(SDL_Window* window);
        static EventManager* getForWindow(SDL_Window* window);

        // Register an event for a window, returns the id of the added listener
        template<typename T, typename Callback>
        static ListenerId registerEvent(SDL_Window* window, const T& event, Callback callback) {
            static_assert(
                std::is_base_of_v<Event, T>,
                "T must inherit from Event"
            );

            if (auto* manager = getForWindow(window)) {
                return manager->registerEvent(event, std::move(callback));
            }

            return kInvalidListener;
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

        void init(SDL_Window* window);

        // Register an event for this window, works for 3rd party events too
        template<typename T>
        void registerEvent(const T& event) {
            static_assert(
                std::is_base_of_v<Event, T>,
                "T must inherit from Event"
            );

            registerEventImpl(event);
        }

        template<typename T, typename Callback>
        ListenerId registerEvent(const T& event, Callback callback) {
            static_assert(
                std::is_base_of_v<Event, T>,
                "T must inherit from Event"
            );

            registerEventImpl(event);
            return addEventListener<T>(std::move(callback));
        }

        template<typename T, typename Callback>
        ListenerId addEventListener(Callback callback) {
            static_assert(
                std::is_base_of_v<Event, T>,
                "T must inherit from Event"
            );

            return addEventListener(
                T::TYPE,
                [callback = std::move(callback)](Event& event) {
                    if (auto* typedEvent = dynamic_cast<T*>(&event)) {
                        callback(*typedEvent);
                    }
                }
            );
        }

        ListenerId addEventListener(EventType type, EventCallback callback);
        void removeListener(ListenerId id);
        void removeListeners(EventType type);
        void processEvent(const SDL_Event& event);

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

        // Variables
        SDL_Window* window_ = nullptr;
        Uint32 windowID_ = 0;
        ListenerId nextListenerId_ = 1;
        std::unordered_map<Uint32, std::vector<EventRegistration>> registrations_;
        std::unordered_map<EventType, std::vector<Listener>> listeners_;
        std::unordered_map<ListenerId, EventType> listenerTypes_;
    };

} // emgui

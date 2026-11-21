//
// Created by epax on 21.11.26.
//

//
// base class for every gui event, holds the type and the factory used by the EventManager
//

#pragma once

#include <SDL.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace emgui {

    using EventType = uint32_t;

    class Event;

    //factory that creates a gui event out of a raw SDL event ->
    using EventFactory = std::function<
        std::unique_ptr<Event>(const SDL_Event&)
    >;

    class Event {
    public:
        //default type, derived events hide this with their own TYPE ->
        static constexpr EventType TYPE = 0;

        virtual ~Event() = default;

        //some functions here ->
        virtual EventType getType() const = 0;
        virtual std::vector<Uint32> getSDLEventTypes() const = 0;
        virtual EventFactory getFactory() const = 0;

        //a listener can set this to stop the remaining listeners ->
        bool handled = false;
    };

} // emgui

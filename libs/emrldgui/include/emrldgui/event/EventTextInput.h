//
// Created by epax on 22.11.26.
//

#pragma once

#include "Event.h"

#include <SDL.h>

#include <string>

namespace emgui {

    class EventTextInput : public Event {
    public:
        static constexpr EventType TYPE = 3;

        explicit EventTextInput(SDL_Window* window);
        EventTextInput(const SDL_Event& event, SDL_Window* window);

        EventType getType() const override;
        std::vector<Uint32> getSDLEventTypes() const override;
        EventFactory getFactory() const override;

        Uint32 getWindowID() const;
        SDL_Window* getWindow() const;
        const std::string& getText() const;

    private:
        SDL_Window* window_ = nullptr;
        Uint32 windowID_ = 0;
        std::string text_;
    };

} // emgui

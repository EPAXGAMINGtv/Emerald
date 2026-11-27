// Created by epax on 27.11.26.

#pragma once

#include "emrldgui/event/Event.h"

namespace emerald {

    // App-level event built from SDL_WINDOWEVENT (RESIZED / SIZE_CHANGED).
    // Event type 4 is free: 0=Event, 1=Keyboard, 2=Mouse, 3=TextInput.
    class EventWindowResize : public emgui::Event {
    public:
        static constexpr emgui::EventType TYPE = 4;

        explicit EventWindowResize(SDL_Window* window);
        EventWindowResize(const SDL_Event& event, SDL_Window* window);

        emgui::EventType getType() const override;
        std::vector<Uint32> getSDLEventTypes() const override;
        emgui::EventFactory getFactory() const override;

        bool isResized() const;
        Uint32 getWindowID() const;
        int getWidth() const;
        int getHeight() const;

    private:
        SDL_Window* window_ = nullptr;
        Uint32 windowID_ = 0;
        int width_ = 0;
        int height_ = 0;
        bool resized_ = false;
    };

} // namespace emerald

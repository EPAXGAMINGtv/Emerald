//
// Created by epax on 21.11.26.
//

#include "../include/emrldgui/Window.h"
#include "../include/emrldgui/event/EventKeyboard.h"

#include <SDL.h>


#include "spdlog/spdlog.h"

#include <vector>

namespace emgui {

    Window::Window(std::string  title,int width,int heigh,SDL_WindowFlags flags) :  width_(width), height_(heigh),title_(title), flags_(flags) {
        spdlog::info("Creating window {}",title_);
        window_ = SDL_CreateWindow(title_.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,width_,height_,flags);
        if (window_ == nullptr) {
            throw std::runtime_error(
            std::string("SDL_CreateWindow failed: ") + SDL_GetError()
            );
        }

        //each window initializes its own event manager here ->
        eventManager_ = EventManager::create(window_);

        //keyboard events are registered for every window by default ->
        EventManager::registerEvent(window_, EventKeyboard(window_));

        spdlog::info("Created window {}",title_);
    }

    Window::~Window() {
        spdlog::info("Window destructor called closing SDL2 Window!: {}",title_);
        closeWindow();
    }

    //some  functions here->
    void Window::updateWindow() {
        SDL_Event event;

        //events of other windows are collected and pushed back for their own manager ->
        std::vector<SDL_Event> otherWindowEvents;
        const Uint32 windowID = SDL_GetWindowID(window_);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                shouldClose_ = true;
                continue;
            }

            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == windowID) {
                shouldClose_ = true;
            }

            const Uint32 eventWindowID = EventManager::getWindowID(event);

            if (eventWindowID == 0 || eventWindowID == windowID) {
                eventManager_->processEvent(event);
            } else {
                otherWindowEvents.push_back(event);
            }
        }

        for (SDL_Event& otherEvent : otherWindowEvents) {
            SDL_PushEvent(&otherEvent);
        }

        if (window_ == nullptr) {
            return;
        }

        SDL_Surface* surface = SDL_GetWindowSurface(window_);

        if (surface == nullptr) {
            spdlog::error("Could not get window surface: {}", SDL_GetError());
            return;
        }

        if (SDL_UpdateWindowSurface(window_) != 0) {
            spdlog::error("Window update failed: {}", SDL_GetError());
        }
    }

    void Window::setTitle(std::string  title) {
        //sets new titlé and updates the title_ variable
        if (!title.empty() && window_ != nullptr) {
            title_ = title;
            SDL_SetWindowTitle(window_, title_.c_str());
            spdlog::info("Setted title {}",title_);
        }else {
            spdlog::error("Window title cannot be empty!");
        }
    }

    void Window::setHeight(int height) {
        //sets the window height and updates the height_ variable
        if (height >0  && window_ != nullptr) {
            height_ = height;
            SDL_SetWindowSize(window_, width_, height_);
            spdlog::info("Setted height {}",height_);
        }else {
            spdlog::error("Window height must be greater than 0");
        }
    }

    void Window::setWidth(int width) {
        //same for width
        if (width >0 && window_ != nullptr) {
            width_ = width;
            SDL_SetWindowSize(window_, width_, height_);
            spdlog::info("Setted width {}",width_);
        }else {
            spdlog::error("Window width must be greater than 0");
        }
    }

    void Window::closeWindow() {
        //closes window
        spdlog::info("Closing window {}",title_);
        if (window_ != nullptr) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
    }


} // emgui

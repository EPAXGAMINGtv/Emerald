//
// Created by epax on 21.11.26.
//

//
// file for managing windows simply central brick of the libemrldgui
//

#pragma once

#include "event/EventManager.h"
#include "render/RenderContext.h"
#include "render/RendererOpts.h"

#include <SDL_video.h>

#include <memory>
#include <string>

namespace emgui {
    class Window {
    public:
        //Constructor and Destructor here ->
        Window(std::string  title,int width=1920,int height=1080,SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE));
        ~Window();

        //some  functions here->
        void updateWindow() ;
        void setTitle(std::string title) ;
        void setHeight(int height);
        void setWidth(int width);
        void closeWindow();

        //some getters here ->
        bool shouldClose() const {
            return shouldClose_;
        }

        SDL_Window* getWindow() const {
            return window_;
        }

        //every window owns its own event manager, use this to register new events ->
        EventManager& getEventManager() {
            return *eventManager_;
        }

        const EventManager& getEventManager() const {
            return *eventManager_;
        }

        //the window creates its own render context, use this for RendererOpts ->
        RenderContext& getRendererContext() {
            return *rendererContext_;
        }

        const RenderContext& getRendererContext() const {
            return *rendererContext_;
        }

        //the window owns the renderer too, no need to create RendererOpts manually ->
        RendererOpts& getRendererOpts() {
            return *rendererOpts_;
        }

        const RendererOpts& getRendererOpts() const {
            return *rendererOpts_;
        }

        std::string getTitle()  const {
            return title_;
        }
        int getHeight()  const {
            return height_;
        }
        int getWidth() const {
            return width_;
        }
        SDL_WindowFlags getFlags()  const {
            return flags_;
        }

    private:

        //variables
        bool shouldClose_ = false;
        int width_,height_;
        std::string  title_;
        SDL_WindowFlags flags_;
        SDL_Window* window_ = nullptr;
        std::unique_ptr<EventManager> eventManager_;
        std::unique_ptr<RenderContext> rendererContext_;
        std::unique_ptr<RendererOpts> rendererOpts_;
    };
}

// Created by epax on 21.11.26.

// File for managing windows simply central brick of the libemrldgui

#pragma once

#include "event/EventManager.h"
#include "render/RenderContext.h"
#include "render/RendererOpts.h"
#include "ui_component/UIComponentManager.h"
#include "ui_component/UiStyle.h"

#include <SDL_video.h>

#include <memory>
#include <string>

namespace emgui {
    class Window {
    public:
        Window(std::string  title,int width=1920,int height=1080,SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE));
        ~Window();

        // A window owns SDL resources, so it is neither copyable nor movable
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        // Static quit state shared by all windows
        static bool isGlobalQuitRequested();
        static void requestGlobalQuit();
        static void resetGlobalQuit();

        void shutdown();
        void updateWindow() ;
        void renderUI();

        // Re-read res/style/Window.css after a CSS reload
        void refreshBackdrop();

        void setTitle(std::string title) ;
        void setHeight(int height);
        void setWidth(int width);
        void closeWindow();

        bool shouldClose() const {
            return shouldClose_ || isGlobalQuitRequested();
        }

        SDL_Window* getWindow() const {
            return window_;
        }

        // Every window owns its own event manager, use this to register new events
        EventManager& getEventManager() {
            return *eventManager_;
        }

        const EventManager& getEventManager() const {
            return *eventManager_;
        }

        // The window creates its own render context, use this for RendererOpts
        RenderContext& getRendererContext() {
            return *rendererContext_;
        }

        const RenderContext& getRendererContext() const {
            return *rendererContext_;
        }

        // The window owns the renderer too, no need to create RendererOpts manually
        RendererOpts& getRendererOpts() {
            return *rendererOpts_;
        }

        const RendererOpts& getRendererOpts() const {
            return *rendererOpts_;
        }

        // Every window owns its UI manager, register with addComponent(&component)
        UIComponentManager& getUIComponentManager() {
            return *uiComponentManager_;
        }

        const UIComponentManager& getUIComponentManager() const {
            return *uiComponentManager_;
        }

        // Shortcut to res/style/*.css loading and reload()
        StyleManager& getStyleManager() {
            return uiComponentManager_->getStyleManager();
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

        // Variables
        bool shouldClose_ = false;
        int width_,height_;
        std::string  title_;
        SDL_WindowFlags flags_;
        SDL_Window* window_ = nullptr;
        std::unique_ptr<EventManager> eventManager_;
        std::unique_ptr<RenderContext> rendererContext_;
        std::unique_ptr<RendererOpts> rendererOpts_;
        std::unique_ptr<UIComponentManager> uiComponentManager_;
        BackdropStyle backdrop_;
    };
}

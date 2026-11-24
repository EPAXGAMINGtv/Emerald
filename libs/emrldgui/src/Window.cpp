// Created by epax on 21.11.26.

#include "emrldgui/Window.h"
#include "emrldgui/EmeraldGUI.h"
#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventMouse.h"

#include <SDL.h>


#include "spdlog/spdlog.h"

#include <vector>

namespace emgui {

    namespace {
        // One global SDL_QUIT closes every window, not just the one polling it
        bool g_globalQuitRequested = false;
    } // namespace

    Window::Window(std::string  title,int width,int heigh,SDL_WindowFlags flags) :  width_(width), height_(heigh),title_(title), flags_(flags) {
        spdlog::info("Creating window {}",title_);
        window_ = SDL_CreateWindow(title_.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,width_,height_,flags);
        if (window_ == nullptr) {
            throw std::runtime_error(
            std::string("SDL_CreateWindow failed: ") + SDL_GetError()
            );
        }

        eventManager_ = EventManager::create(window_);

        rendererContext_ = std::make_unique<RenderContext>(window_);

        rendererOpts_ = std::make_unique<RendererOpts>(*rendererContext_);

        uiComponentManager_ = std::make_unique<UIComponentManager>();
        uiComponentManager_->setWindow(this);
        refreshBackdrop();

        EventManager::registerEvent(window_, EventKeyboard(window_));
        EventManager::registerEvent(window_, EventMouse(window_));

        if (auto* gui = emrldgui::EmeraldGUI::active()) {
            gui->registerWindow(this);
        }

        spdlog::info("Created window {}",title_);
    }

    Window::~Window() {
        spdlog::info("Destroying window {}", title_);

        shutdown();

        if (auto* gui = emrldgui::EmeraldGUI::active()) {
            gui->unregisterWindow(this);
        }
    }

    void Window::shutdown() {
        uiComponentManager_.reset();
        rendererOpts_.reset();
        eventManager_.reset();
        rendererContext_.reset();

        closeWindow();
    }

    bool Window::isGlobalQuitRequested() {
        return g_globalQuitRequested;
    }

    void Window::requestGlobalQuit() {
        g_globalQuitRequested = true;
    }

    void Window::resetGlobalQuit() {
        g_globalQuitRequested = false;
    }

    void Window::updateWindow() {
        SDL_Event event;

        std::vector<SDL_Event> otherWindowEvents;
        const Uint32 windowID = SDL_GetWindowID(window_);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                requestGlobalQuit();
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

        if (uiComponentManager_ != nullptr) {
            uiComponentManager_->update();
        }

        if (window_ == nullptr) {
            return;
        }

        if (SDL_GetRenderer(window_) != nullptr) {
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

    void Window::renderUI() {
        if (rendererOpts_ != nullptr && uiComponentManager_ != nullptr) {
            int logicalWidth = 0;
            int logicalHeight = 0;
            SDL_RenderGetLogicalSize(
                rendererOpts_->getRenderer(),
                &logicalWidth,
                &logicalHeight
            );

            if (logicalWidth <= 0 || logicalHeight <= 0) {
                logicalWidth = rendererContext_->getWindowWidth();
                logicalHeight = rendererContext_->getWindowHeight();
            }

            backdrop_.draw(*rendererOpts_, {0, 0, logicalWidth, logicalHeight});
            uiComponentManager_->render(*rendererOpts_);
        }
    }

    void Window::refreshBackdrop() {
        if (uiComponentManager_ != nullptr) {
            backdrop_.apply(
                uiComponentManager_->getStyleManager().resolveNamed("Window")
            );
        }
    }

    void Window::setTitle(std::string  title) {
        
        if (!title.empty() && window_ != nullptr) {
            title_ = title;
            SDL_SetWindowTitle(window_, title_.c_str());
            spdlog::info("Window title set to {}", title_);
        }else {
            spdlog::error("Window title cannot be empty!");
        }
    }

    void Window::setHeight(int height) {
        
        if (height >0  && window_ != nullptr) {
            height_ = height;
            SDL_SetWindowSize(window_, width_, height_);
            spdlog::info("Window height set to {}", height_);
        }else {
            spdlog::error("Window height must be greater than 0");
        }
    }

    void Window::setWidth(int width) {
        
        if (width >0 && window_ != nullptr) {
            width_ = width;
            SDL_SetWindowSize(window_, width_, height_);
            spdlog::info("Window width set to {}", width_);
        }else {
            spdlog::error("Window width must be greater than 0");
        }
    }

    void Window::closeWindow() {
        
        if (window_ != nullptr) {
            spdlog::info("Closing window {}", title_);
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
    }


} // emgui

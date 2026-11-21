//
// Created by epax on 21.11.26.
//

//
// file for managing windows simply central brick of the libemrldgui
//

#pragma once
#include <SDL_video.h>
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
        bool shouldClose() {
            return shouldClose_;
        }

        SDL_Window* getWindow() {
            return window_;
        }
        std::string getTitle() {
            return title_;
        }
        int getHeight() {
            return height_;
        }
        int getWidth() {
            return width_;
        }
        SDL_WindowFlags getFlags() {
            return flags_;
        }

    private:

        //variables
        bool shouldClose_ = false;
        int width_,height_;
        std::string  title_;
        SDL_WindowFlags flags_;
        SDL_Window* window_ = nullptr;
    };
}


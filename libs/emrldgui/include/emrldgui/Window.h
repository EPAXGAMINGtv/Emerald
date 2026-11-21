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
        Window(std::string title= "SDL Window",int width=1920,int height=1080,SDL_WindowFlags flags=SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE);
        ~Window();

        //some  functions here->
        void updateWindow();
        void setTitle(std::string title);
        void setHeight(int height);
        void setWidth(int width);
        void setFlags(SDL_WindowFlags flags);
        void closeWindow();

        //some getters here ->
        SDL_Window* getWindow();
        std::string getTitle();
        int getHeight();
        int getWidth();
        SDL_WindowFlags getFlags();

    private:
        void terminateSDL();

        //variables
        int width_,height_;
        std::string title_;
        SDL_WindowFlags flags_;
        SDL_Window* window_;
    };
}


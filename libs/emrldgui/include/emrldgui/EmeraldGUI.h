//
// Created by epax on 21.11.26.
//

#pragma once

#include <vector>

namespace emgui {
    class Window;
} // emgui

namespace emrldgui {
    class EmeraldGUI {
    public:
        EmeraldGUI();
        ~EmeraldGUI();

        EmeraldGUI(const EmeraldGUI&) = delete;
        EmeraldGUI& operator=(const EmeraldGUI&) = delete;
        EmeraldGUI(EmeraldGUI&&) = delete;
        EmeraldGUI& operator=(EmeraldGUI&&) = delete;

        //the newest active GUI instance owns the SDL/TTF lifetime ->
        static EmeraldGUI* active();

        void registerWindow(emgui::Window* window);
        void unregisterWindow(emgui::Window* window);

    private:
        std::vector<emgui::Window*> windows_;
        bool videoInitialized_ = false;
        bool ttfInitialized_ = false;
    };
} // emrldgui

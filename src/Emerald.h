// Created by epax on 27.11.26.

#pragma once

#include "cfg/Config.hpp"

#include <memory>

namespace emgui {
    class Window;
    class Keyboard;
    class Mouse;
} // emgui

namespace emrldgui {
    class EmeraldGUI;
} // emrldgui

class Emerald {
public:
    Emerald();
    ~Emerald();

    Emerald(const Emerald&) = delete;
    Emerald& operator=(const Emerald&) = delete;
    Emerald(Emerald&&) = delete;
    Emerald& operator=(Emerald&&) = delete;

    void init();
    void run();

private:
    void shutdown();

    cfg::Config config_{"emerald.yaml"};

    std::unique_ptr<emrldgui::EmeraldGUI> gui_;
    std::unique_ptr<emgui::Window> window_;
    std::unique_ptr<emgui::Keyboard> keyboard_;
    std::unique_ptr<emgui::Mouse> mouse_;
};

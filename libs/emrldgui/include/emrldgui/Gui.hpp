#pragma once

namespace emrldgui {

class Gui {
public:
    Gui() = default;

    bool init();
    void run();
    void shutdown();

private:
    bool initialized_ = false;
};

} // namespace emrldgui

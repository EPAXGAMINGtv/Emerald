//
// Created by epax on 21.11.26.
//

//
// small test for the per window event system
//

#include "cfg/Config.hpp"
#include "eemerge/Emerge.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "emrldgui/EmeraldGUI.h"
#include "emrldgui/Window.h"
#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/input/Keyboard.h"
#include "emrldgui/input/Mouse.h"
#include "emrldgui/render/RendererOpts.h"
#include "emrldgui/ui_component/UIComponentManager.h"
#include "emrldgui/ui_component/components/Button.h"
#include "emrldgui/ui_component/components/CheckBox.h"
#include "emrldgui/ui_component/components/ComboBox.h"
#include "emrldgui/ui_component/components/Panel.h"
#include "emrldgui/ui_component/components/ProgressBar.h"
#include "emrldgui/ui_component/components/RadioButton.h"
#include "emrldgui/ui_component/components/Slider.h"
#include "emrldgui/ui_component/components/TextField.h"
#include "emrldgui/ui_component/components/Text.h"
#include "emrldgui/ui_component/components/ToggleButton.h"


int main() {
    auto logger = spdlog::stdout_color_mt("emerald");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

    spdlog::info("Emerald starting");

    cfg::Config config("emerald.yaml");
    config.set_string("app.name", "Emerald");
    config.set_string("logging.level", "info");
    config.set("gui.enabled", true);

    if (config.load_or_create()) {
        spdlog::info("Config ready: {} (app.name={})", config.path().string(),
                     config.get_string("app.name", "Emerald"));
    } else {
        spdlog::error("Config could not be loaded or created: {}", config.path().string());
    }

    emgr::Emerge emerge;
    emerge.EmergeSync();

    {
        emrldgui::EmeraldGUI emeraldGUI;
        emgui::Window window = emgui::Window("test", 1920, 1080,
                                             static_cast<SDL_WindowFlags>(
                                                 SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN));

        emgui::Keyboard keyboard(window);

        emgui::Mouse mouse(window);

        auto& rendererOpts = window.getRendererOpts();

        emgui::Text helloWorldTextComponent("Hello, World!", 10, 10);
        helloWorldTextComponent.setStyleClass("title");

        emgui::Text helloWorldTextComponent2("Hello, World22!", 50, 60);
        helloWorldTextComponent2.setStyleId("mainTitle");

        window.getUIComponentManager().addComponent(&helloWorldTextComponent);
        window.getUIComponentManager().addComponent(&helloWorldTextComponent2);

        int clickCount = 0;

        emgui::Button clickButton("Click me", 50, 120, 220, 60);
        clickButton.setStyleClass("primary");
        clickButton.onPressed([](emgui::Button&) {
            spdlog::debug("Button press animation started");
        });
        clickButton.onReleased([](emgui::Button&) {
            spdlog::debug("Button press animation finished");
        });
        clickButton.onClick([&clickCount](emgui::Button& button) {
            spdlog::info("Clicked '{}' (total: {})", button.getText(), ++clickCount);
        });
        window.getUIComponentManager().addComponent(&clickButton);

        emgui::Button startButton("Start", 50, 200, 220, 64);
        startButton.setStyleId("dangerButton");
        startButton.onClick([&clickCount](emgui::Button&) {
            spdlog::info("Start button clicked (total: {})", ++clickCount);
        });
        window.getUIComponentManager().addComponent(&startButton);

        emgui::Panel controlsPanel(10, 300, 560, 660, "Controls");
        controlsPanel.setStyleClass("accent");
        window.getUIComponentManager().addComponent(&controlsPanel);

        emgui::TextField nameField("Enter your name", 40, 360, 300, 48);
        nameField.onTextChanged([](emgui::TextField& field) {
            spdlog::debug("Text changed: {}", field.getText());
        });
        nameField.onEnter([](emgui::TextField& field) {
            spdlog::info("Submitted: {}", field.getText());
        });
        window.getUIComponentManager().addComponent(&nameField);

        emgui::CheckBox agreeBox("I agree", false, 40, 430);
        agreeBox.onCheckedChanged([](emgui::CheckBox& box) {
            spdlog::info("Agreement: {}", box.isChecked() ? "yes" : "no");
        });
        window.getUIComponentManager().addComponent(&agreeBox);

        emgui::Slider volumeSlider(0.0f, 100.0f, 35.0f, 40, 490, 300, 56);
        volumeSlider.onValueChanged([](emgui::Slider& slider) {
            spdlog::debug("Volume: {:.0f}", slider.getValue());
        });
        window.getUIComponentManager().addComponent(&volumeSlider);

        emgui::ToggleButton autoSaveToggle("Auto save", 40, 570, 220, 52);
        autoSaveToggle.onCheckedChanged([](emgui::ToggleButton& button) {
            spdlog::info("Auto save: {}", button.isChecked() ? "on" : "off");
        });
        window.getUIComponentManager().addComponent(&autoSaveToggle);

        emgui::ComboBox themeBox(
            {"Emerald", "Sapphire", "Ruby"}, 0, 40, 650, 300, 48
        );
        themeBox.onSelectionChanged([](emgui::ComboBox& box) {
            spdlog::info("Theme: {}", box.getSelectedText());
        });
        window.getUIComponentManager().addComponent(&themeBox);

        emgui::RadioButton radioLeft("Left", "alignment", true, 40, 720);
        emgui::RadioButton radioRight("Right", "alignment", false, 200, 720);
        radioLeft.onCheckedChanged([](emgui::RadioButton& radio) {
            spdlog::info("Alignment left: {}", radio.isChecked() ? "yes" : "no");
        });
        window.getUIComponentManager().addComponent(&radioLeft);
        window.getUIComponentManager().addComponent(&radioRight);

        emgui::ProgressBar progressBar(0.0f, 100.0f, 65.0f, 40, 780, 400, 32);
        window.getUIComponentManager().addComponent(&progressBar);

        auto& saveButton = window.getUIComponentManager().addComponent<emgui::Button>(
            "Save", 600, 330, 160, 52
        );
        saveButton.setButtonType(emgui::Button::ButtonType::Success);
        saveButton.onClick([&clickCount](emgui::Button&) {
            spdlog::info("Save clicked (total: {})", ++clickCount);
        });

        auto& cancelButton = window.getUIComponentManager().addComponent<emgui::Button>(
            "Cancel", 600, 400, 160, 52
        );
        cancelButton.setButtonType(emgui::Button::ButtonType::Ghost);
        cancelButton.onClick([&clickCount](emgui::Button&) {
            spdlog::info("Cancel clicked (total: {})", ++clickCount);
        });

        while (!window.shouldClose()) {
            window.updateWindow();

            auto frame = rendererOpts.beginFrame();
            window.renderUI();


            if (keyboard.isPressed(SDLK_ESCAPE)) {
                spdlog::info("Escape pressed, closing window");
                window.closeWindow();
            }

            if (mouse.isButtonPressed(SDL_BUTTON_LEFT)) {
                spdlog::debug("Left mouse button pressed at {} {}", mouse.getX(), mouse.getY());
            }

            spdlog::debug("Mouse pos x:{} and y:{}", mouse.getX(), mouse.getY());
            keyboard.clearFrameState();
            mouse.clearFrameState();
        }
    }

    spdlog::info("Emerald exiting");
    spdlog::shutdown();
    return 0;
}

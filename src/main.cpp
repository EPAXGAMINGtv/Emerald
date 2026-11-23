// Created by epax on 21.11.26.

#include "cfg/Config.hpp"
#include "eemerge/Emerge.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <vector>

#include "emrldgui/EmeraldGUI.h"
#include "emrldgui/Window.h"
#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/input/Keyboard.h"
#include "emrldgui/input/Mouse.h"
#include "emrldgui/render/RendererOpts.h"
#include "emrldgui/resource/ResourceManager.h"
#include "emrldgui/ui_component/UIComponentManager.h"
#include "emrldgui/ui_component/components/Button.h"
#include "emrldgui/ui_component/components/CheckBox.h"
#include "emrldgui/ui_component/components/Layout.h"
#include "emrldgui/ui_component/components/ComboBox.h"
#include "emrldgui/ui_component/components/Panel.h"
#include "emrldgui/ui_component/components/ProgressBar.h"
#include "emrldgui/ui_component/components/RadioButton.h"
#include "emrldgui/ui_component/components/Slider.h"
#include "emrldgui/ui_component/components/Text.h"
#include "emrldgui/ui_component/components/TextField.h"
#include "emrldgui/ui_component/components/Tabs.h"
#include "emrldgui/ui_component/components/ToggleButton.h"

namespace {
    // Temporary theme switch: set to true to test the alternate res2/ theme.
    constexpr bool useAlternateTheme = false;
}

int main() {
    auto logger = spdlog::stdout_color_mt("emerald");
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

    spdlog::info("Emerald starting");

#ifdef EMERLDGUI_RES2_DIR
    if (useAlternateTheme) {
        emgui::ResourceManager::setRoot(EMERLDGUI_RES2_DIR);
    }
#endif

    cfg::Config config("emerald.yaml");
    config.set_string("app.name", "Emerald");
    config.set_string("logging.level", "info");
    config.set("gui.enabled", true);

    if (config.load_or_create()) {
        spdlog::info(
            "Config ready: {} (app.name={})",
            config.path().string(),
            config.get_string("app.name", "Emerald")
        );
    } else {
        spdlog::error(
            "Config could not be loaded or created: {}",
            config.path().string()
        );
    }

    emgr::Emerge emerge;
    emerge.EmergeSync();

    {
        emrldgui::EmeraldGUI emeraldGUI;

        // Use the biggest display as the window size instead of hard-coded 1920x1080
        SDL_DisplayMode displayMode{};
        int displayWidth = 1600;
        int displayHeight = 900;

        if (SDL_GetCurrentDisplayMode(0, &displayMode) == 0 &&
            displayMode.w > 0 && displayMode.h > 0) {
            displayWidth = displayMode.w;
            displayHeight = displayMode.h;
        }

        spdlog::info("Window size follows display: {}x{}", displayWidth, displayHeight);

        emgui::Window window(
            "Emerald Playground",
            displayWidth,
            displayHeight,
            static_cast<SDL_WindowFlags>(
                SDL_WINDOW_MAXIMIZED | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
            )
        );

        emgui::Keyboard keyboard(window);
        emgui::Mouse mouse(window);

        auto& rendererOpts = window.getRendererOpts();

        // The UI is authored in a fixed 1920x1080 design space. SDL scales the
        // render output and mouse events to the real window size automatically.
        constexpr int designWidth = 1920;
        constexpr int designHeight = 1080;

        if (SDL_RenderSetLogicalSize(
                rendererOpts.getRenderer(), designWidth, designHeight
            ) != 0) {
            spdlog::warn("Logical size scaling unavailable: {}", SDL_GetError());
        }

        rendererOpts.setClearColor(
            useAlternateTheme ? SDL_Color{246, 238, 222, 255}
                              : SDL_Color{8, 12, 26, 255}
        );

        auto& ui = window.getUIComponentManager();

        int clickCount = 0;

        auto& headerPanel = ui.addComponent<emgui::Panel>(
            24, 24, 1872, 88
        );
        headerPanel.setStyleClass("hero");
        headerPanel.setTitle("");

        auto& controlsPanel = ui.addComponent<emgui::Panel>(
            24, 136, 520, 760, "Controls"
        );
        controlsPanel.setStyleClass("accent");

        ui.addComponent<emgui::Panel>(
            568, 136, 1328, 760, "Showcase"
        );

        auto& footerPanel = ui.addComponent<emgui::Panel>(
            24, 912, 1872, 144
        );
        footerPanel.setStyleClass("hero");
        footerPanel.setTitle("");

        auto& appTitle = ui.addComponent<emgui::Text>(
            "Emerald UI", 52, 46
        );
        appTitle.setStyleClass("hero");

        auto& appSubtitle = ui.addComponent<emgui::Text>(
            "Component playground", 320, 58
        );
        appSubtitle.setStyleClass("muted");

        auto& launchButton = ui.addComponent<emgui::Button>(
            "Launch", 1560, 42, 240, 52
        );
        launchButton.setStyleId("startButton");
        launchButton.onClick([&clickCount](emgui::Button&) {
            spdlog::info("Launch clicked (total: {})", ++clickCount);
        });

        auto& inputLabel = ui.addComponent<emgui::Text>(
            "Input", 48, 190
        );
        inputLabel.setStyleClass("section");

        auto& nameField = ui.addComponent<emgui::TextField>(
            "Type something...", 48, 226, 456, 52
        );
        nameField.onTextChanged([](emgui::TextField& field) {
            spdlog::debug("Text changed: {}", field.getText());
        });
        nameField.onEnter([](emgui::TextField& field) {
            spdlog::info("Submitted: {}", field.getText());
        });

        auto& optionsLabel = ui.addComponent<emgui::Text>(
            "Preferences", 48, 304
        );
        optionsLabel.setStyleClass("section");

        auto& agreeBox = ui.addComponent<emgui::CheckBox>(
            "I agree to the terms", false, 48, 340
        );
        agreeBox.onCheckedChanged([](emgui::CheckBox& box) {
            spdlog::info("Agreement: {}", box.isChecked() ? "yes" : "no");
        });

        auto& autoSaveToggle = ui.addComponent<emgui::ToggleButton>(
            "Auto save", 48, 388, 200, 44
        );
        autoSaveToggle.onCheckedChanged([](emgui::ToggleButton& button) {
            spdlog::info("Auto save: {}", button.isChecked() ? "on" : "off");
        });

        auto& alignLeft = ui.addComponent<emgui::RadioButton>(
            "Left", "alignment", true, 48, 448
        );
        ui.addComponent<emgui::RadioButton>(
            "Right", "alignment", false, 210, 448
        );
        alignLeft.onCheckedChanged([](emgui::RadioButton& radio) {
            spdlog::info("Alignment: {}", radio.isChecked() ? "left" : "right");
        });

        auto& volumeSlider = ui.addComponent<emgui::Slider>(
            0.0f, 100.0f, 35.0f, 48, 512, 456, 56
        );
        volumeSlider.onValueChanged([](emgui::Slider& slider) {
            spdlog::debug("Volume: {:.0f}", slider.getValue());
        });

        auto& progressLabel = ui.addComponent<emgui::Text>(
            "Progress", 48, 676
        );
        progressLabel.setStyleClass("section");

        ui.addComponent<emgui::ProgressBar>(
            0.0f, 100.0f, 65.0f, 48, 714, 456, 28
        );

        auto& applyButton = ui.addComponent<emgui::Button>(
            "Apply", 48, 786, 160, 48
        );
        applyButton.setButtonType(emgui::Button::ButtonType::Primary);
        applyButton.onClick([&clickCount](emgui::Button&) {
            spdlog::info("Apply clicked (total: {})", ++clickCount);
        });

        auto& resetButton = ui.addComponent<emgui::Button>(
            "Reset", 196, 786, 160, 48
        );
        resetButton.setButtonType(emgui::Button::ButtonType::Ghost);
        resetButton.onClick([&clickCount](emgui::Button&) {
            spdlog::info("Reset clicked (total: {})", ++clickCount);
        });

        ui.addComponent<emgui::ComboBox>(
            std::vector<std::string>{"Emerald", "Sapphire", "Ruby"}, 0, 48, 596, 456, 52
        ).onSelectionChanged([](emgui::ComboBox& box) {
            spdlog::info("Theme: {}", box.getSelectedText());
        });

        auto& buttonsLabel = ui.addComponent<emgui::Text>(
            "Button variants", 596, 190
        );
        buttonsLabel.setStyleClass("section");

        auto& primaryButton = ui.addComponent<emgui::Button>(
            "Primary", 596, 228, 140, 48
        );
        primaryButton.setButtonType(emgui::Button::ButtonType::Primary);

        auto& successButton = ui.addComponent<emgui::Button>(
            "Success", 752, 228, 140, 48
        );
        successButton.setButtonType(emgui::Button::ButtonType::Success);

        auto& warningButton = ui.addComponent<emgui::Button>(
            "Warning", 908, 228, 140, 48
        );
        warningButton.setButtonType(emgui::Button::ButtonType::Warning);

        auto& dangerButton = ui.addComponent<emgui::Button>(
            "Danger", 1064, 228, 140, 48
        );
        dangerButton.setButtonType(emgui::Button::ButtonType::Danger);

        auto& ghostButton = ui.addComponent<emgui::Button>(
            "Ghost", 1220, 228, 140, 48
        );
        ghostButton.setButtonType(emgui::Button::ButtonType::Ghost);

        auto& fieldsLabel = ui.addComponent<emgui::Text>(
            "Fields", 596, 312
        );
        fieldsLabel.setStyleClass("section");

        ui.addComponent<emgui::TextField>(
            "Search components", 596, 348, 420, 52
        );

        ui.addComponent<emgui::ComboBox>(
            std::vector<std::string>{"Compact", "Comfortable", "Spacious"}, 1, 1040, 348, 180, 52
        );

        auto& feedbackLabel = ui.addComponent<emgui::Text>(
            "Feedback", 596, 432
        );
        feedbackLabel.setStyleClass("section");

        ui.addComponent<emgui::ProgressBar>(
            0.0f, 100.0f, 42.0f, 596, 468, 620, 28
        );

        ui.addComponent<emgui::Slider>(
            0.0f, 100.0f, 72.0f, 596, 528, 500, 56
        );

        auto& cardPanel = ui.addComponent<emgui::Panel>(
            1220, 300, 620, 320, "Card"
        );
        cardPanel.setStyleClass("accent");

        auto& cardTitle = ui.addComponent<emgui::Text>(
            "Glass surfaces", 1252, 356
        );
        cardTitle.setStyleClass("section");

        auto& cardBody = ui.addComponent<emgui::Text>(
            "Soft gradients, subtle borders and calm spacing.",
            1252, 400
        );
        cardBody.setStyleClass("muted");

        auto& cardButton = ui.addComponent<emgui::Button>(
            "Learn more", 1252, 468, 180, 48
        );
        cardButton.setButtonType(emgui::Button::ButtonType::Primary);

        auto& layoutLabel = ui.addComponent<emgui::Text>(
            "Layouts & Tabs", 596, 642
        );
        layoutLabel.setStyleClass("section");

        auto& tabs = ui.addComponent<emgui::Tabs>(
            596, 678, 600, 196
        );
        tabs.setStyleClass("accent");

        auto& overviewPage = ui.addComponent<emgui::Layout>(
            596, 716, 600, 152
        );
        overviewPage.setStyleClass("page");
        overviewPage.setDirection(emgui::Layout::Direction::Column);
        overviewPage.setGap(10);

        auto& overviewText = ui.addComponent<emgui::Text>(
            "CSS flex: column + gap", 0, 0
        );
        overviewText.setStyleClass("muted");

        auto& overviewButton = ui.addComponent<emgui::Button>(
            "Flex button", 0, 0, 150, 44
        );
        overviewPage.addChild(&overviewText);
        overviewPage.addChild(&overviewButton);

        auto& detailsPage = ui.addComponent<emgui::Layout>(
            596, 716, 600, 152
        );
        detailsPage.setStyleClass("page");
        detailsPage.setDirection(emgui::Layout::Direction::Column);
        detailsPage.setGap(10);

        auto& detailsText = ui.addComponent<emgui::Text>(
            "The other page is hidden by Tabs", 0, 0
        );
        detailsText.setStyleClass("muted");

        auto& detailsButton = ui.addComponent<emgui::Button>(
            "Page two", 0, 0, 150, 44
        );
        detailsPage.addChild(&detailsText);
        detailsPage.addChild(&detailsButton);

        tabs.addPage("Overview", &overviewPage);
        tabs.addPage("Details", &detailsPage);
        tabs.onSelectionChanged([](emgui::Tabs&, int index) {
            spdlog::info("Tab changed to {}", index);
        });

        auto& buttonRow = ui.addComponent<emgui::Layout>(
            1220, 640, 620, 56
        );
        buttonRow.setStyleClass("card-row");

        auto& rowPrimary = ui.addComponent<emgui::Button>(
            "Flex A", 0, 0, 120, 44
        );
        auto& rowGhost = ui.addComponent<emgui::Button>(
            "Flex B", 0, 0, 120, 44
        );
        rowGhost.setButtonType(emgui::Button::ButtonType::Ghost);
        buttonRow.addChild(&rowPrimary);
        buttonRow.addChild(&rowGhost);

        auto& statusText = ui.addComponent<emgui::Text>(
            "Interact with any control to see it react.", 48, 948
        );
        statusText.setStyleClass("muted");

        auto& hintText = ui.addComponent<emgui::Text>(
            "Ctrl+C / Ctrl+V work in text fields - ESC closes the window.",
            48, 992
        );
        hintText.setStyleClass("muted");

        auto& saveButton = ui.addComponent<emgui::Button>(
            "Save", 1330, 950, 160, 48
        );
        saveButton.setButtonType(emgui::Button::ButtonType::Success);
        saveButton.onClick([&clickCount](emgui::Button&) {
            spdlog::info("Save clicked (total: {})", ++clickCount);
        });

        auto& cancelButton = ui.addComponent<emgui::Button>(
            "Cancel", 1510, 950, 160, 48
        );
        cancelButton.setButtonType(emgui::Button::ButtonType::Ghost);
        cancelButton.onClick([&clickCount](emgui::Button&) {
            spdlog::info("Cancel clicked (total: {})", ++clickCount);
        });

        auto& quitButton = ui.addComponent<emgui::Button>(
            "Quit", 1690, 950, 160, 48
        );
        quitButton.setStyleId("dangerButton");
        quitButton.onClick([&window](emgui::Button&) {
            spdlog::info("Quit button clicked");
            window.closeWindow();
        });

        while (!window.shouldClose()) {
            window.updateWindow();

            auto frame = rendererOpts.beginFrame();
            window.renderUI();

            if (keyboard.isPressed(SDLK_ESCAPE)) {
                spdlog::info("Escape pressed, closing window");
                window.closeWindow();
            }

            keyboard.clearFrameState();
            mouse.clearFrameState();
        }
    }

    spdlog::info("Emerald exiting");
    spdlog::shutdown();
    return 0;
}

// Created by epax on 27.11.26.

#include "Emerald.h"

#include "emrldgui/EmeraldGUI.h"
#include "emrldgui/Window.h"
#include "emrldgui/input/Keyboard.h"
#include "emrldgui/input/Mouse.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <SDL.h>

#include <exception>
#include <string>

Emerald::Emerald() = default;

void Emerald::init() {
    auto logger = spdlog::get("emerald");
    if (!logger) {
        logger = spdlog::stdout_color_mt("emerald");
    }
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");

    spdlog::info("Emerald starting");
    //inits Emrldgui
    gui_ = std::make_unique<emrldgui::EmeraldGUI>();
    //gets main monitor size for default size
    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);

    //sets some default config shi
    config_.set_default("app.name", std::string("Emerald"));
    config_.set_default("app.version", std::string("0.0.1"));
    config_.set_default("app.width", DM.w);
    config_.set_default("app.height", DM.h);

    if (config_.load_or_create()) {
        spdlog::info(
            "Config ready: {} (app.name={})",
            config_.path().string(),
            config_.get_string("app.name", "Emerald")
        );
    } else {
        spdlog::error(
            "Config could not be loaded or created: {}",
            config_.path().string()
        );
    }

    try {

        const int width = config_.get_int("app.width", 1920);
        const int height = config_.get_int("app.height", 1080);
        window_ = std::make_unique<emgui::Window>(
        "Emerald - Application Store", width, height,static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE |SDL_WINDOW_ALLOW_HIGHDPI)
        );

        keyboard_ = std::make_unique<emgui::Keyboard>(*window_);
        mouse_ = std::make_unique<emgui::Mouse>(*window_);
    } catch (const std::exception& error) {
        spdlog::error("Emerald initialization failed: {}", error.what());
        shutdown();
    }
}

void Emerald::run() {
    if (!window_) {
        spdlog::error("Emerald cannot run: GUI is not initialized");
        return;
    }

    while (!window_->shouldClose()) {
        window_->updateWindow();

        [[maybe_unused]] auto frame =
            window_->getRendererOpts().beginFrame();

        window_->renderUI();

        if (keyboard_->isPressed(SDLK_ESCAPE)) {
            spdlog::info("Escape pressed, closing window");
            window_->closeWindow();
            break;
        }

        keyboard_->clearFrameState();
        mouse_->clearFrameState();
    }
}

void Emerald::shutdown() {
    mouse_.reset();
    keyboard_.reset();
    window_.reset();
    gui_.reset();
}

Emerald::~Emerald() {
    shutdown();
    spdlog::info("Emerald exiting");
    spdlog::shutdown();
}

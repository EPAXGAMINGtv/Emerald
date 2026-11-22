//
// Created by epax on 22.11.26.
//

#pragma once

#include "emrldgui/render/RendererOpts.h"

#include <functional>
#include <string>
#include <utility>

namespace emgui {
    class StyleValues;
    class Window;

    class UIComponent {
    public:
        using StyleChangeCallback = std::function<void(UIComponent&)>;

        UIComponent() = default;
        UIComponent(int x, int y) : x_(x), y_(y) {}

        virtual ~UIComponent() = default;

        virtual void render(RendererOpts& renderer) = 0;
        virtual void update() {}

        //used by the CSS resolver, override for concrete components like Text
        virtual std::string getComponentName() const {
            return "UIComponent";
        }

        void setStyleClass(std::string styleClass) {
            styleClass_ = std::move(styleClass);
            notifyStyleChanged();
        }

        const std::string& getStyleClass() const {
            return styleClass_;
        }

        void setStyleId(std::string id) {
            styleId_ = std::move(id);
            notifyStyleChanged();
        }

        const std::string& getStyleId() const {
            return styleId_;
        }

        //called by UIComponentManager after CSS changes
        virtual void applyStyle(const StyleValues& values) {
            (void)values;
        }

        //the manager installs this so setStyleClass()/setStyleId() re-resolve CSS immediately
        void setStyleChangeCallback(StyleChangeCallback callback) {
            styleChangeCallback_ = std::move(callback);
        }

        //the UIComponentManager injects its window automatically -> no window
        //argument is needed when creating a component
        void setWindow(Window& window) {
            window_ = &window;
            onWindowAttached(window);
        }

        Window* getWindow() const {
            return window_;
        }

        //components override this to register their event listeners
        virtual void onWindowAttached(Window& window) {
            (void)window;
        }

        void setPosition(int x, int y) {
            x_ = x;
            y_ = y;
        }

        SDL_Point getPosition() const {
            return SDL_Point{x_, y_};
        }

    protected:
        Window* window_ = nullptr;

        void notifyStyleChanged() {
            if (styleChangeCallback_) {
                styleChangeCallback_(*this);
            }
        }

        int x_ = 0;
        int y_ = 0;

    private:
        std::string styleClass_;
        std::string styleId_;
        StyleChangeCallback styleChangeCallback_;
    };
} // emgui

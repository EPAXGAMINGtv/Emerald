// Created by epax on 22.11.26.

#pragma once

#include "emrldgui/render/RendererOpts.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

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

        // Second pass for popups/tooltips that must draw above every widget.
        virtual void renderOverlay(RendererOpts& renderer) {
            (void)renderer;
        }

        virtual void update() {}

        // Layout parents call this before update/render to arrange their children
        virtual void layout() {}

        // Common CSS properties every component understands (flex, visibility, states)
        void applyCommonStyle(const StyleValues& values);

        // Used by the CSS resolver, override for concrete components like Text
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

        // Called by UIComponentManager after CSS changes
        virtual void applyStyle(const StyleValues& values) {
            (void)values;
        }

        // The manager installs this so setStyleClass()/setStyleId() re-resolve CSS immediately
        void setStyleChangeCallback(StyleChangeCallback callback) {
            styleChangeCallback_ = std::move(callback);
        }

        // The UIComponentManager injects its window automatically.
        // No window argument is needed when creating a component
        void setWindow(Window& window) {
            window_ = &window;
            onWindowAttached(window);
        }

        Window* getWindow() const {
            return window_;
        }

        // Components override this to register their event listeners
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

        void setVisible(bool visible) {
            visible_ = visible;
        }

        bool isVisible() const {
            return visible_;
        }

        // Containers like ScrollArea draw their children themselves so they
        // can clip them. The manager still handles styling, layout and events.
        void setRenderedByParent(bool value) {
            renderedByParent_ = value;
        }

        bool isRenderedByParent() const {
            return renderedByParent_;
        }

        // Containers override this so hiding a page also hides its descendants
        virtual void setSubtreeVisible(bool visible) {
            setVisible(visible);
        }

        // Flex hints used by emgui::Layout containers
        void setFlex(float grow) {
            flex_ = grow > 0.0f ? grow : 0.0f;
        }

        float getFlex() const {
            return flex_;
        }

        void setAlignSelf(std::string align) {
            alignSelf_ = std::move(align);
        }

        const std::string& getAlignSelf() const {
            return alignSelf_;
        }

        // Pseudo-class state used by the CSS resolver (:hover, :active, ...)
        void setHovered(bool value) {
            setState(state_.hovered, value);
        }
        bool isHovered() const {
            return state_.hovered;
        }
        void setActive(bool value) {
            setState(state_.active, value);
        }
        void setFocused(bool value) {
            setState(state_.focused, value);
        }
        bool isFocused() const {
            return state_.focused;
        }
        void setSelected(bool value) {
            setState(state_.selected, value);
        }
        void setDisabled(bool value) {
            setState(state_.disabled, value);
        }

        bool hasState(const std::string& state) const;

        // Components that participate in layouts report their natural size here
        virtual SDL_Point getLayoutSize() const {
            return {0, 0};
        }

        virtual void setLayoutRect(const SDL_Rect& rect) {
            x_ = rect.x;
            y_ = rect.y;
        }

    protected:
        Window* window_ = nullptr;

        void notifyStyleChanged() {
            if (styleChangeCallback_) {
                styleChangeCallback_(*this);
            }
        }

        void setState(bool& flag, bool value) {
            if (flag == value) {
                return;
            }
            flag = value;
            notifyStyleChanged();
        }

        int x_ = 0;
        int y_ = 0;

        struct StateFlags {
            bool hovered = false;
            bool active = false;
            bool focused = false;
            bool selected = false;
            bool disabled = false;
        };

    private:
        std::string styleClass_;
        std::string styleId_;
        StyleChangeCallback styleChangeCallback_;
        std::string alignSelf_;
        StateFlags state_;
        bool visible_ = true;
        bool renderedByParent_ = false;
        float flex_ = 0.0f;
    };
} // emgui

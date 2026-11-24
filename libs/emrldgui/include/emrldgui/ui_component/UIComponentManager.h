// Created by epax on 22.11.26.

#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "UIComponent.h"
#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventManager.h"
#include "emrldgui/style/StyleManager.h"

namespace emgui {

    class Window;

    class UIComponentManager {
    private:
        // Owned components are destroyed with the manager, external ones are only referenced
        struct ComponentEntry {
            std::unique_ptr<UIComponent> owned;
            UIComponent* external = nullptr;

            UIComponent* get() const {
                return owned != nullptr ? owned.get() : external;
            }
        };

    public:
        // Manager owns the component, usage: auto& text = addComponent<Text>("Hi", 10, 10)
        template<typename T, typename... Args>
        T& addComponent(Args&&... args) {
            static_assert(
                std::is_base_of_v<UIComponent, T>,
                "T must inherit from UIComponent"
            );

            auto component = std::make_unique<T>(std::forward<Args>(args)...);
            T& reference = *component;

            components_.push_back(ComponentEntry{std::move(component), nullptr});
            finalizeRegistration(&reference);

            return reference;
        }

        ~UIComponentManager();

        // Non-owning registration, usage: addComponent(&myStackComponent)
        void addComponent(UIComponent* component);

        void removeComponent(UIComponent* component);

        // True while the manager tears its components down. Container-owned
        // children use this to skip unregistering from a dying manager.
        bool isShuttingDown() const {
            return shuttingDown_;
        }

        // Tab / Shift+Tab focus navigation. Only widgets that opted in with
        // UIComponent::setFocusable(true) participate.
        void setFocusedComponent(UIComponent* component);
        UIComponent* getFocusedComponent() const {
            return focusedComponent_;
        }
        void focusNext();
        void focusPrevious();

        // While a modal is open, focus navigation stays inside it.
        void setModalComponent(UIComponent* component) {
            modalComponent_ = component;
        }
        UIComponent* getModalComponent() const {
            return modalComponent_;
        }

        void update();
        void render(RendererOpts& renderer);
        void clear();

        // The owning window injects itself here so every component can use it
        void setWindow(Window* window);

        Window* getWindow() const {
            return window_;
        }

        // CSS helpers
        StyleManager& getStyleManager() {
            return styleManager_;
        }

        void applyStylesTo(UIComponent* component);
        void reloadStyles();

        std::size_t getComponentCount() const;
        bool contains(UIComponent* component) const;

    private:
        void finalizeRegistration(UIComponent* component);
        void detachComponent(UIComponent* component);
        void onKeyEvent(EventKeyboard& event);
        void moveFocus(int direction);

        std::vector<ComponentEntry> components_;
        StyleManager styleManager_;
        Window* window_ = nullptr;
        EventManager::ListenerId keyListener_ = EventManager::kInvalidListener;
        UIComponent* focusedComponent_ = nullptr;
        UIComponent* modalComponent_ = nullptr;
        bool shuttingDown_ = false;
    };

} // emgui

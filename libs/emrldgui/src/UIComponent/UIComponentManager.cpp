// Created by epax on 22.11.26.

#include "emrldgui/ui_component/UIComponentManager.h"
#include "emrldgui/Window.h"
#include "emrldgui/animation/Animation.h"

#include <algorithm>

namespace emgui {

    UIComponentManager::~UIComponentManager() {
        if (window_ != nullptr && window_->getWindow() != nullptr) {
            if (auto* events = EventManager::getForWindow(window_->getWindow())) {
                if (keyListener_ != EventManager::kInvalidListener) {
                    events->removeListener(keyListener_);
                }
            }
        }

        clear();
    }

    void UIComponentManager::setWindow(Window* window) {
        window_ = window;

        if (window == nullptr || keyListener_ != EventManager::kInvalidListener) {
            return;
        }

        SDL_Window* sdlWindow = window->getWindow();

        if (sdlWindow == nullptr) {
            return;
        }

        keyListener_ = EventManager::registerEvent(
            sdlWindow,
            EventKeyboard(sdlWindow),
            [this](EventKeyboard& event) {
                onKeyEvent(event);
            }
        );
    }

    void UIComponentManager::addComponent(UIComponent* component) {
        if (component == nullptr || contains(component)) {
            return;
        }

        components_.push_back(ComponentEntry{nullptr, component});
        finalizeRegistration(component);
    }

    void UIComponentManager::finalizeRegistration(UIComponent* component) {
        if (component == nullptr) {
            return;
        }

        if (window_ != nullptr) {
            component->setWindow(*window_);
        }

        styleManager_.loadComponent(component->getComponentName());
        component->setStyleChangeCallback(
            [this](UIComponent& changed) {
                applyStylesTo(&changed);
            }
        );
        styleManager_.applyTo(*component);
    }

    void UIComponentManager::detachComponent(UIComponent* component) {
        if (component != nullptr) {
            component->setStyleChangeCallback({});
        }
    }

    void UIComponentManager::applyStylesTo(UIComponent* component) {
        if (component != nullptr && contains(component)) {
            styleManager_.applyTo(*component);
        }
    }

    void UIComponentManager::reloadStyles() {
        styleManager_.reload();

        for (const ComponentEntry& entry : components_) {
            if (UIComponent* component = entry.get()) {
                styleManager_.applyTo(*component);
            }
        }

        if (window_ != nullptr) {
            window_->refreshBackdrop();
        }
    }

    void UIComponentManager::removeComponent(UIComponent* component) {
        if (component == nullptr || shuttingDown_) {
            return;
        }

        detachComponent(component);

        if (focusedComponent_ == component) {
            focusedComponent_ = nullptr;
        }
        if (modalComponent_ == component) {
            modalComponent_ = nullptr;
        }

        components_.erase(
            std::remove_if(
                components_.begin(),
                components_.end(),
                [component](const ComponentEntry& current) {
                    return current.get() == component;
                }
            ),
            components_.end()
        );
    }

    void UIComponentManager::setFocusedComponent(UIComponent* component) {
        if (component != nullptr &&
            (!component->isVisible() || component->isDisabled())) {
            return;
        }

        if (focusedComponent_ == component) {
            if (component != nullptr) {
                component->setFocused(true);
            }
            return;
        }

        if (focusedComponent_ != nullptr) {
            focusedComponent_->setFocused(false);
        }

        focusedComponent_ = component;

        if (focusedComponent_ != nullptr) {
            focusedComponent_->setFocused(true);
        }
    }

    void UIComponentManager::focusNext() {
        moveFocus(1);
    }

    void UIComponentManager::focusPrevious() {
        moveFocus(-1);
    }

    void UIComponentManager::moveFocus(int direction) {
        std::vector<UIComponent*> focusables;

        if (modalComponent_ != nullptr) {
            modalComponent_->collectFocusable(focusables);
        } else {
            for (const ComponentEntry& entry : components_) {
                if (UIComponent* component = entry.get()) {
                    component->collectFocusable(focusables);
                }
            }
        }

        // Containers list their children themselves, so a directly registered
        // child can appear more than once in the collected list
        focusables.erase(
            std::unique(focusables.begin(), focusables.end()),
            focusables.end()
        );

        if (focusables.empty()) {
            return;
        }

        const std::size_t count = focusables.size();
        std::size_t index = direction > 0 ? 0 : count - 1;

        if (focusedComponent_ != nullptr) {
            const auto found = std::find(
                focusables.begin(),
                focusables.end(),
                focusedComponent_
            );

            if (found != focusables.end()) {
                const auto distance = std::distance(focusables.begin(), found);
                const auto current = static_cast<std::size_t>(distance);

                if (direction > 0) {
                    index = (current + 1) % count;
                } else {
                    index = (current + count - 1) % count;
                }
            }
        }

        setFocusedComponent(focusables[index]);
    }

    void UIComponentManager::onKeyEvent(EventKeyboard& event) {
        if (!event.isPressed() || event.isRepeat() ||
            event.getKey() != SDLK_TAB) {
            return;
        }

        // A modal dialog traps Tab and moves focus between its own buttons
        if (modalComponent_ != nullptr) {
            return;
        }

        if ((event.getModifiers() & KMOD_SHIFT) != KMOD_NONE) {
            focusPrevious();
        } else {
            focusNext();
        }

        event.handled = true;
    }

    void UIComponentManager::update() {
        Animation::tick(SDL_GetTicks());

        // Layout containers arrange their children before any widget updates
        for (const ComponentEntry& entry : components_) {
            if (UIComponent* component = entry.get()) {
                component->layout();
            }
        }

        for (const ComponentEntry& entry : components_) {
            if (UIComponent* component = entry.get()) {
                if (component->isVisible()) {
                    component->update();
                }
            }
        }
    }

    void UIComponentManager::render(RendererOpts& renderer) {
        // Normal widgets first, then popups on top of all of them.
        for (const ComponentEntry& entry : components_) {
            if (UIComponent* component = entry.get()) {
                if (component->isVisible() && !component->isRenderedByParent()) {
                    component->render(renderer);
                }
            }
        }

        for (const ComponentEntry& entry : components_) {
            if (UIComponent* component = entry.get()) {
                if (component->isVisible() && !component->isRenderedByParent()) {
                    component->renderOverlay(renderer);
                }
            }
        }
    }

    void UIComponentManager::clear() {
        shuttingDown_ = true;
        focusedComponent_ = nullptr;
        modalComponent_ = nullptr;

        // Detach callbacks before owned components are destroyed
        for (const ComponentEntry& entry : components_) {
            detachComponent(entry.get());
        }

        components_.clear();
    }

    std::size_t UIComponentManager::getComponentCount() const {
        return components_.size();
    }

    bool UIComponentManager::contains(UIComponent* component) const {
        if (component == nullptr) {
            return false;
        }

        return std::any_of(
            components_.begin(),
            components_.end(),
            [component](const ComponentEntry& current) {
                return current.get() == component;
            }
        );
    }

} // emgui

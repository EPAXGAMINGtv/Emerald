// Created by epax on 22.11.26.

#include "emrldgui/ui_component/UIComponentManager.h"

#include <algorithm>

namespace emgui {

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
    }

    void UIComponentManager::removeComponent(UIComponent* component) {
        if (component == nullptr) {
            return;
        }

        detachComponent(component);

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

    void UIComponentManager::update() {
        for (const ComponentEntry& entry : components_) {
            if (UIComponent* component = entry.get()) {
                component->update();
            }
        }
    }

    void UIComponentManager::render(RendererOpts& renderer) {
        for (const ComponentEntry& entry : components_) {
            if (UIComponent* component = entry.get()) {
                component->render(renderer);
            }
        }
    }

    void UIComponentManager::clear() {
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

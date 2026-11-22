//
// Created by epax on 22.11.26.
//

#include "emrldgui/ui_component/UIComponentManager.h"

#include <algorithm>

namespace emgui {

    void UIComponentManager::addComponent(UIComponent* component) {
        if (component == nullptr || contains(component)) {
            return;
        }

        components_.push_back(ComponentEntry{nullptr, component});
    }

    void UIComponentManager::removeComponent(UIComponent* component) {
        if (component == nullptr) {
            return;
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
        //only owned components are deleted, external components stay alive ->
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
